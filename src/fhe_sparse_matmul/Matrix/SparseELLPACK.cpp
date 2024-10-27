#include <fhe_sparse_matmul/Matrix/SparseELLPACK.hpp>
#include <fhe_sparse_matmul/utils.hpp>
#include <assert.h>

namespace SparseFHE {

SparseELLPACKFHE::SparseELLPACKFHE(uint64_t rows, uint64_t cols, uint64_t chunk_size, const double* const data, SealCKKSRuntimeContext& context)
{
    this->_rows = rows;
    this->_cols = cols;
    this->_chunk_size = chunk_size;

    // Determine non-zero values so we can allocate values and indices array
    this->_row_nzv.resize(this->rows());
    this->_max_nzv = 0;
    for (uint64_t row = 0; row < this->rows(); row++)
    {   
        uint64_t non_zero = 0;
        for (uint64_t col = 0; col < this->cols(); col++)
        {
            if (doubles_close(data[(row*cols) + col], 0.0) == false)
            {
                non_zero++;
            }
        }
        this->_row_nzv.at(row) = non_zero;
        this->_max_nzv = std::max(this->_max_nzv, non_zero);
    }

    // Temporary encrypted values array (not stored in encrypted structure)
    std::vector<double> values(this->rows()*this->_max_nzv, 0.0);
    this->_col_indices = std::vector<std::vector<uint64_t>>(this->rows(), std::vector<uint64_t>(this->_max_nzv, 0));
    
    // Populate column indices metadata array and temporary value array
    for (uint64_t row = 0; row < this->rows(); row++)
    {   
        uint64_t col_idx = 0;
        for (uint64_t col = 0; col < this->cols(); col++)
        {
            double value = data[(row*this->cols()) + col];
            if (doubles_close(value, 0.0) == false)
            {
                values.at((row*this->_max_nzv) + col_idx) = value;
                this->_col_indices.at(row).at(col_idx) = col;
                col_idx++;
            }
        }
    }

    // Now encrypt values and store
    size_t slot_count = context.ckks_encoder->slot_count();
    assert(chunk_size <= slot_count);
    std::vector<std::vector<double>> mat_v = chunk_values(values, chunk_size, slot_count);
    this->_enc_mat = encrypt_values(mat_v, context);
}


void SparseELLPACKFHE::compute_result_metadata(const SparseELLPACKFHE& rhs, SparseELLPACKFHE& result) const
{
    // Matrix to hold occupancy data for the result i.e. where will there be result values
    std::vector<bool> occupancy(result.rows()*result.cols(), false);
    for (uint64_t A_row = 0; A_row < result.rows(); A_row++)
    {
        for (uint64_t A_nzv_idx = 0; A_nzv_idx < this->_max_nzv; A_nzv_idx++)
        {
            // Stop if we have reached end of valid entries for this row
            if (A_nzv_idx >= this->_row_nzv.at(A_row))
            {
                break;
            }

            for (uint64_t B_nzv_idx = 0; B_nzv_idx < rhs._max_nzv; B_nzv_idx++)
            {
                uint64_t A_idx = (A_row*this->_max_nzv) + A_nzv_idx;
                uint64_t A_col = this->_col_indices.at(A_row).at(A_nzv_idx);

                // Stop if we have reached end of valid entries for this row
                if (B_nzv_idx >= rhs._row_nzv.at(A_col))
                {
                    break;
                }

                uint64_t B_idx = (A_col*rhs._max_nzv) + B_nzv_idx;
                uint64_t B_col = rhs._col_indices.at(A_col).at(B_nzv_idx);
                uint64_t R_idx = ((A_row*result.cols()) + B_col);

                occupancy.at(R_idx) = true;
            }
        }
    }

    // Generate ELLPACK metadata
    result._row_nzv = std::vector<uint64_t>(result.rows(), 0);
    result._max_nzv = 0;

    for (uint64_t row = 0; row < result.rows(); row++)
    {
        for(uint64_t col = 0; col < result.cols(); col++)
        {
            if (occupancy.at((row * result.cols()) + col) == true)
            {
                result._col_indices.at(row).push_back(col);
                result._row_nzv.at(row) += 1;
                result._max_nzv = std::max(result._max_nzv, result._row_nzv.at(row));
            }
        }
    }

    // Ensure all column index vectors are the same size
    for (auto& col_index : result._col_indices)
        col_index.resize(result._max_nzv);
}


uint64_t SparseELLPACKFHE::get_ellpack_index(const SparseELLPACKFHE& ellpack, uint64_t row, uint64_t col) const
{
    // Row is the same, all we need to do is find where the column we want is 
    for (uint64_t col_idx = 0; col_idx < ellpack._row_nzv.at(row); col_idx++)
    {
        if (ellpack._col_indices.at(row).at(col_idx) == col)
        {
            return (row*ellpack._max_nzv) + col_idx;
        }
    }

    // If we get here something has gone wrong with the ELLPACK formatting, this will trigger an exception downstream
    return std::numeric_limits<uint64_t>::max();
}


SparseELLPACKFHE SparseELLPACKFHE::fhe_matmul(const SparseELLPACKFHE& rhs, const SealCKKSRuntimeContext& context, uint64_t n_threads) const
{
    // Define intermediate ciphertexts used during computation
    seal::Ciphertext enc_zeros = encrypted_zeros(context);
    seal::Ciphertext enc_slot_zero_mask = encrypted_slot_zero_mask(context);

    // Compute what the ELLPACK structure for the result will look like (without exposing any values)
    SparseELLPACKFHE result(this->rows(), rhs.cols(), this->_chunk_size);
    this->compute_result_metadata(rhs, result);

    size_t n_result_chunks = std::ceil(static_cast<double>(result._col_indices.size() * result._max_nzv)/static_cast<double>(this->_chunk_size));
    result._enc_mat = std::vector<seal::Ciphertext>(n_result_chunks, enc_zeros);

    std::vector<std::mutex> result_chunks_mutex(n_result_chunks);
    std::vector<std::shared_ptr<std::thread>> threads(n_threads);
        
    for (uint64_t A_row = 0; A_row < this->rows(); A_row++)
    {
        for (uint64_t A_nzv_idx = 0; A_nzv_idx < this->_max_nzv; A_nzv_idx++)
        {
            // Stop if we have reached end of valid entries for this row
            if (A_nzv_idx >= this->_row_nzv.at(A_row))
            {
                break;
            }

            // Wait for thread to become available
            size_t thread_idx = ((A_row*this->_max_nzv) + A_nzv_idx) % n_threads;
            if ((threads.at(thread_idx) != nullptr) && (threads.at(thread_idx)->joinable()))
            {
                threads.at(thread_idx)->join();
            }
            

            threads.at(thread_idx) = std::shared_ptr<std::thread>(
                new std::thread([&, A_row, A_nzv_idx] {

                    for (uint64_t B_nzv_idx = 0; B_nzv_idx < rhs._max_nzv; B_nzv_idx++)
                    {
                        uint64_t A_idx = (A_row*this->_max_nzv) + A_nzv_idx;
                        uint64_t A_col = this->_col_indices.at(A_row).at(A_nzv_idx);
                        
                        // Stop if we have reached end of valid entries for this row
                        if (B_nzv_idx >= rhs._row_nzv.at(A_col))
                        {
                            break;
                        }

                        // Compute index into result ELLPACK values matrix for current location
                        uint64_t B_idx = (A_col*rhs._max_nzv) + B_nzv_idx;
                        uint64_t B_col = rhs._col_indices.at(A_col).at(B_nzv_idx);
                        uint64_t R_idx = this->get_ellpack_index(result, A_row, B_col);

                        // Determine which chunk and where in the chunk we need to access for both operands
                        size_t selected_A_chunk = A_idx / this->_chunk_size;
                        size_t A_data_offset = A_idx % this->_chunk_size;
                        size_t selected_B_chunk = B_idx / rhs._chunk_size;
                        size_t B_data_offset = B_idx % rhs._chunk_size;
                        size_t selected_R_chunk = R_idx / result._chunk_size;
                        size_t R_data_offset = R_idx % result._chunk_size;

                        
                        seal::Ciphertext enc_rot_a, enc_rot_b;
                        
                        // Rotate A and B so the desired elements line up in slot zero
                        context.evaluator->rotate_vector(this->_enc_mat.at(selected_A_chunk), A_data_offset, *context.galois_keys, enc_rot_a);
                        context.evaluator->rotate_vector(rhs._enc_mat.at(selected_B_chunk), B_data_offset, *context.galois_keys, enc_rot_b);
                        
                        // Multiply
                        context.evaluator->multiply_inplace(enc_rot_a, enc_rot_b);
                        context.evaluator->relinearize_inplace(enc_rot_a, *context.relin_keys);
                        context.evaluator->rescale_to_next_inplace(enc_rot_a);

                        // Mask out slot zero
                        context.evaluator->mod_switch_to(enc_slot_zero_mask, enc_rot_a.parms_id(), enc_rot_b);
                        context.evaluator->multiply_inplace(enc_rot_a, enc_rot_b);
                        context.evaluator->relinearize_inplace(enc_rot_a, *context.relin_keys);
                        context.evaluator->rescale_to_next_inplace(enc_rot_a);

                        // Rotate result to slot within chunk
                        context.evaluator->rotate_vector_inplace(enc_rot_a, -R_data_offset, *context.galois_keys);

                        // Critical section, acquire lock
                        std::lock_guard<std::mutex> chunk_lock(result_chunks_mutex.at(selected_R_chunk));
                        context.evaluator->mod_switch_to_inplace(result._enc_mat.at(selected_R_chunk), enc_rot_a.parms_id());
                        result._enc_mat.at(selected_R_chunk).scale() = enc_rot_a.scale();
                        context.evaluator->add_inplace(result._enc_mat.at(selected_R_chunk), enc_rot_a);
                    }
                })
            );
        }
    }

    // Wait for all row threads to join 
    for (size_t idx = 0; idx < n_threads; idx++) {
        if ((threads.at(idx) != nullptr) && (threads.at(idx)->joinable()))
        {
            threads.at(idx)->join();
        }
    }

    return result;
}


void SparseELLPACKFHE::decrypt(const SealCKKSSecretContext& context, double* const output) const
{
    // Zero input matrix entries
    zero_matrix(output, this->rows()*this->cols());

    // Chunk overflow is how much of the last chunk is occupied with values
    size_t chunk_overflow = (this->rows() * this->cols()) % this->_chunk_size;
    for (uint64_t chunk = 0; chunk < this->_enc_mat.size(); chunk++)
    { 
        seal::Plaintext plain_result;
        std::vector<double> result_values;
        context.decryptor->decrypt(this->_enc_mat.at(chunk), plain_result);
        context.ckks_encoder->decode(plain_result, result_values);

        // When the matrix does not fit perfectly into chunks, we need to only iterate part-way through
        size_t chunk_size_limit = this->_chunk_size;
        if ((chunk == (this->_enc_mat.size() - 1)) && (chunk_overflow != 0)) {
            chunk_size_limit = chunk_overflow;
        }

        // Place the chunk values into the output array
        for (uint64_t elem = 0; elem < chunk_size_limit; elem++)
        {
            uint64_t result_idx = (chunk * this->_chunk_size) + elem;
            uint64_t row = result_idx / this->_max_nzv;
            uint64_t col_idx = result_idx % this->_max_nzv;

            // Don't add to result if this index is outwith the number of elements in this row
            if (col_idx < this->_row_nzv.at(row))
            {
                uint64_t col = this->_col_indices.at(row).at(col_idx);
                output[(row * this->cols()) + col] = result_values.at(elem);
            }
        }
    }
}


double SparseELLPACKFHE::sparsity() const 
{
    // Sum of the non-zero values across all rows over the size of the matrix
    uint64_t sum = 0;
    for (auto row : this->_row_nzv)
        sum += row;
    return static_cast<double>(sum) / static_cast<double>(this->rows() * this->cols()); 
}


}; // namespace SparseFHE
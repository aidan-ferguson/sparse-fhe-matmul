#include <fhe_sparse_matmul/Matrix/SparseCSR.hpp>
#include <fhe_sparse_matmul/utils.hpp>
#include <assert.h>

namespace SparseFHE {

SparseCSRFHE::SparseCSRFHE(uint64_t rows, uint64_t cols, uint64_t chunk_size, const double *const data, SealCKKSRuntimeContext &context)
{
    this->_rows = rows;
    this->_cols = cols;
    this->_chunk_size = chunk_size;

    // Temporary non-zero values vector, not stored in encrypted structure
    std::vector<double> nzv;

    // Populate CSR metadata vectors that indicate position of elements
    this->_row_index.reserve(rows + 1);
    for(uint64_t row_idx = 0; row_idx < rows; row_idx++)
    {
        this->_row_index.push_back(nzv.size());
        for(uint64_t col_idx = 0; col_idx < cols; col_idx++)
        {
            // Store value, col idx and row idx for all non-zero values
            double v = data[col_idx + (row_idx * cols)];
            if (doubles_close(v, 0.0) == false)
            {
                nzv.push_back(v);
                this->_col_index.push_back(col_idx);
            }
        }
    }
    this->_row_index.push_back(nzv.size());

    // Encode & Encrypt all required matrices
    size_t slot_count = context.ckks_encoder->slot_count();
    assert(chunk_size <= slot_count);
    std::vector<std::vector<double>> mat_v = chunk_values(nzv, chunk_size, slot_count);
    this->_enc_mat = encrypt_values(mat_v, context);
}


void SparseCSRFHE::compute_result_metadata(const SparseCSRFHE& rhs, SparseCSRFHE& result) const
{
    for (uint64_t A_row = 0; A_row < result.rows(); A_row++)
    {
        result._row_index.push_back(result._col_index.size());

        // Select the indices that represent this row in the LHS encrypted values vector
        uint64_t A_row_start = this->_row_index.at(A_row);
        uint64_t A_row_end = this->_row_index.at(A_row + 1);

        for(uint64_t A_data_idx = A_row_start; A_data_idx < A_row_end; A_data_idx++)
        {
            // Get the column index of the current selected element in A
            uint64_t A_col = this->_col_index.at(A_data_idx);

            // Select the indices that represent this row in the RHS encrypted values vector
            uint64_t B_row_start = rhs._row_index.at(A_col);
            uint64_t B_row_end = rhs._row_index.at(A_col + 1);

            for(uint64_t B_data_idx = B_row_start; B_data_idx < B_row_end; B_data_idx++)
            {
                // Get the column of this value in RHS
                uint64_t B_col = rhs._col_index.at(B_data_idx);
                result._col_index.push_back(B_col);
            }
        }
    }
    result._row_index.push_back(result._col_index.size());
}


uint64_t SparseCSRFHE::get_csr_index(const SparseCSRFHE& csr, uint64_t row, uint64_t col) const
{
    // Get the start of the row by referring to row index array
    uint64_t row_start = csr._row_index.at(row);
    uint64_t row_end   = csr._row_index.at(row + 1);

    // Search from start of row to end for column
    for (uint64_t col_idx = row_start; col_idx < row_end; col_idx++)
    {
        if (csr._col_index.at(col_idx) == col)
        {
            return col_idx;
        }
    }

    // If we get to this point, something has gone very wrong
    // we return something that will be out of range, this will trigger std::out_of_range
    // when we try to access with .at()
    return std::numeric_limits<uint64_t>::max();
}


// TODO: standardise formatting, here we have 'SparseCSRFHE &rhs'. Just install extension and auto-format
SparseCSRFHE SparseCSRFHE::fhe_matmul(const SparseCSRFHE &rhs, const SealCKKSRuntimeContext &context, uint64_t n_threads) const
{
    // Define intermediate ciphertexts used during computation
    seal::Ciphertext enc_zeros = encrypted_zeros(context);
    seal::Ciphertext enc_slot_zero_mask = encrypted_slot_zero_mask(context);

    // Ahead of multiplication, compute what the CSR structure of the result will look like
    // We must compute this ahead of time to facilitate multi-threading
    SparseCSRFHE result(this->rows(), rhs.cols(), this->_chunk_size);
    this->compute_result_metadata(rhs, result);

    size_t n_result_chunks = std::ceil(static_cast<double>(result._col_index.size())/static_cast<double>(this->_chunk_size));
    result._enc_mat = std::vector<seal::Ciphertext>(n_result_chunks, enc_zeros);

    std::vector<std::mutex> result_chunks_mutex(n_result_chunks);
    std::vector<std::shared_ptr<std::thread>> threads(n_threads);

    for (uint64_t A_row = 0; A_row < this->_rows; A_row++)
    {
        // Select the indices that represent this row in the LHS encrypted values vector
        uint64_t A_row_start = this->_row_index.at(A_row);
        uint64_t A_row_end = this->_row_index.at(A_row + 1);

        for(uint64_t A_data_idx = A_row_start; A_data_idx < A_row_end; A_data_idx++)
        {
            // Get the column index of the current selected element in A
            uint64_t A_col = this->_col_index.at(A_data_idx);

            // Select the indices that represent this row in the RHS encrypted values vector
            uint64_t B_row_start = rhs._row_index.at(A_col);
            uint64_t B_row_end = rhs._row_index.at(A_col + 1);

            // Wait on required thread_idx joining back to main
            size_t thread_idx = ((A_row*this->_cols) + A_col) % n_threads;
            if ((threads.at(thread_idx) != nullptr) && (threads.at(thread_idx)->joinable()))
            {
                threads.at(thread_idx)->join();
            }

            threads.at(thread_idx) = std::shared_ptr<std::thread>(
                new std::thread([&, A_data_idx, B_row_start, B_row_end, A_row] {

                    for(uint64_t B_data_idx = B_row_start; B_data_idx < B_row_end; B_data_idx++)
                    {
                        // Get the column of this value in RHS
                        uint64_t B_col = rhs._col_index.at(B_data_idx);
                        uint64_t R_idx = get_csr_index(result, A_row, B_col);

                        // Determine which chunk and where in the chunk we need to access for both operands
                        size_t selected_a_chunk = A_data_idx / this->_chunk_size;
                        size_t selected_b_chunk = B_data_idx / rhs._chunk_size;
                        size_t selected_r_chunk = R_idx / result._chunk_size;
                        size_t A_data_offset = A_data_idx % this->_chunk_size;
                        size_t B_data_offset = B_data_idx % rhs._chunk_size;
                        size_t R_data_offset = R_idx % result._chunk_size;
                        
                        seal::Ciphertext enc_rot_a, enc_rot_b;

                        // Rotate A and B so the desired elements line up in slot zero
                        context.evaluator->rotate_vector(this->_enc_mat.at(selected_a_chunk), A_data_offset, *context.galois_keys, enc_rot_a);
                        context.evaluator->rotate_vector(rhs._enc_mat.at(selected_b_chunk), B_data_offset, *context.galois_keys, enc_rot_b);
                        // Multiply
                        context.evaluator->multiply_inplace(enc_rot_a, enc_rot_b);
                        context.evaluator->relinearize_inplace(enc_rot_a, *context.relin_keys);
                        context.evaluator->rescale_to_next_inplace(enc_rot_a);
                        
                        // Mask out slot zero
                        context.evaluator->mod_switch_to(enc_slot_zero_mask, enc_rot_a.parms_id(), enc_rot_b);
                        context.evaluator->multiply_inplace(enc_rot_a, enc_rot_b);
                        context.evaluator->relinearize_inplace(enc_rot_a, *context.relin_keys);
                        context.evaluator->rescale_to_next_inplace(enc_rot_a);
                        
                        // Rotate result value to offset slot within chunk
                        context.evaluator->rotate_vector_inplace(enc_rot_a, -R_data_offset, *context.galois_keys);

                        // Critical section, we lock the chunk we want to modify and add
                        std::lock_guard<std::mutex> chunk_lock(result_chunks_mutex.at(selected_r_chunk));
                        context.evaluator->mod_switch_to_inplace(result._enc_mat.at(selected_r_chunk), enc_rot_a.parms_id());
                        result._enc_mat.at(selected_r_chunk).scale() = enc_rot_a.scale();
                        context.evaluator->add_inplace(result._enc_mat.at(selected_r_chunk), enc_rot_a);
                        
                    }
            }));
        }
    }
    // Wait for all threads to finish 
    for (size_t idx = 0; idx < n_threads; idx++) {
        if ((threads.at(idx) != nullptr) && (threads.at(idx)->joinable()))
        {
            threads.at(idx)->join();
        }
    }
    
    
    return result;
}


void SparseCSRFHE::decrypt(const SealCKKSSecretContext &context, double *const output) const
{
    // Zero input matrix entries
    zero_matrix(output, this->rows()*this->cols());

    // TODO: replace size_t with uint64_t unless actually coming from something that is size_t
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
            // Index into CSR value array
            uint64_t value_idx = (chunk * this->_chunk_size) + elem;

            // Recover row and column
            uint64_t col = this->_col_index.at(value_idx);
            uint64_t row = std::numeric_limits<uint64_t>::max();
            for (size_t row_idx = 0; row_idx < this->_row_index.size() - 1; row_idx++)
            {
                if ((this->_row_index.at(row_idx) <= value_idx) && (this->_row_index.at(row_idx + 1) > value_idx))
                {
                    row = row_idx;
                    break;
                }
            }

            output[(row * this->cols()) + col] = result_values.at(elem);
        }
    }
}


double SparseCSRFHE::sparsity() const
{
    // Number of entries in `col_index` array over the size of the matrix, as it is parallel to values array
    return static_cast<double>(this->_col_index.size()) / static_cast<double>(this->rows() * this->cols()); 
}

}; // namespace SparseFHE
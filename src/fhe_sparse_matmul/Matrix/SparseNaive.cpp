#include <fhe_sparse_matmul/Matrix/SparseNaive.hpp>
#include <fhe_sparse_matmul/utils.hpp>
#include <cassert>

namespace SparseFHE {


SparseNaiveFHE::SparseNaiveFHE(uint64_t rows, uint64_t cols, uint64_t chunk_size, const double* const data, SealCKKSRuntimeContext& context)
{
    this->_rows=rows; this->_cols=cols; this->_chunk_size = chunk_size;
    size_t slot_count = context.ckks_encoder->slot_count();
    assert(chunk_size <= slot_count);

    // Populate metadata array that indicates zero positions
    this->_is_zero = std::vector<bool>(rows*cols);
    for(uint32_t p = 0; p < this->_rows * this->_cols; p++)
    {
        this->_is_zero.at(p) = (data[p] == 0);
    }

    // Encode & Encrypt all required matrices
    std::vector<double> mat_values(data, data + (this->rows() * this->cols()));
    const auto mat_v = chunk_values(mat_values, chunk_size, slot_count);
    this->_enc_mat = encrypt_values(mat_v, context);
}


void SparseNaiveFHE::decrypt(const SealCKKSSecretContext& context, double* const output) const
{
    zero_matrix(output, this->rows()*this->cols());

    uint64_t chunk_overflow = (cols() * rows()) % this->_chunk_size;
    for (uint64_t chunk = 0; chunk < this->_enc_mat.size(); chunk++)
    {
        // Decrypt the chunk
        seal::Plaintext plain_result;
        std::vector<double> result_values;
        context.decryptor->decrypt(_enc_mat.at(chunk), plain_result);
        context.ckks_encoder->decode(plain_result, result_values);

        // When the matrix does not fit perfectly into chunks, we need to only iterate part-way through the chunk
        uint64_t chunk_size_limit = this->_chunk_size;
        if ((chunk == (this->_enc_mat.size() - 1)) && (chunk_overflow != 0)) {
            chunk_size_limit = chunk_overflow;
        }

        // Place the chunk values into the output array
        for (uint64_t elem = 0; elem < chunk_size_limit; elem++)
        {
            if (this->_is_zero.at((chunk * this->_chunk_size) + elem) == false)
            {
                output[(chunk * this->_chunk_size) + elem] = result_values.at(elem);
            }
        }
    }
}
    

SparseNaiveFHE SparseNaiveFHE::fhe_matmul(const SparseNaiveFHE& rhs, const SealCKKSRuntimeContext& context, uint64_t n_threads) const
{
    assert(this->cols() == rhs.rows());

    // Define intermediate ciphertexts used during computation
    seal::Ciphertext enc_zeros = encrypted_zeros(context);
    seal::Ciphertext enc_slot_zero_mask = encrypted_slot_zero_mask(context);

    // We want to store our results in different chunks, allows us to multithread more efficiently
    SparseNaiveFHE result(this->rows(), rhs.cols(), this->_chunk_size);
    uint64_t n_result_chunks = std::ceil(static_cast<double>(result.rows()*result.cols())/static_cast<double>(this->_chunk_size));
    result._enc_mat = std::vector<seal::Ciphertext>(n_result_chunks, enc_zeros);

    std::vector<std::mutex> result_chunks_mutex(n_result_chunks);
    std::vector<std::shared_ptr<std::thread>> threads(n_threads);

    for (uint64_t A_row = 0; A_row < this->rows(); A_row++)
    {
        for (uint64_t B_col = 0; B_col < rhs.cols(); B_col++)
        {
            uint64_t thread_idx = ((A_row*rhs.cols()) + B_col) % n_threads;
            if ((threads.at(thread_idx) != nullptr) && (threads.at(thread_idx)->joinable()))
            {
                threads.at(thread_idx)->join();
            }

            threads.at(thread_idx) = std::shared_ptr<std::thread>(
                new std::thread([&, A_row, B_col] {
                    for (uint64_t data_idx = 0; data_idx < this->cols(); data_idx++)
                    {
                        uint64_t A_idx = (A_row*this->cols()) + data_idx;
                        uint64_t B_idx = (data_idx*rhs.cols()) + B_col;
                        uint64_t R_idx = ((A_row*result.cols()) + B_col);

                        if ((this->_is_zero.at(A_idx) != true) && 
                            (rhs._is_zero.at(B_idx) != true))
                        {

                            // Determine which chunk and where in the chunk we need to access for both operands
                            uint64_t selected_A_chunk = A_idx / this->_chunk_size;
                            uint64_t selected_B_chunk = B_idx / rhs._chunk_size;
                            uint64_t selected_R_chunk = R_idx / result._chunk_size;
                            uint64_t A_data_offset = A_idx % this->_chunk_size;
                            uint64_t B_data_offset = B_idx % rhs._chunk_size;
                            uint64_t R_data_offset = R_idx % result._chunk_size;

                            seal::Ciphertext enc_rot_a, enc_rot_b;

                            // Rotate A and B so the desired elements now line up in slot zero
                            context.evaluator->rotate_vector(this->_enc_mat.at(selected_A_chunk), A_data_offset, *context.galois_keys, enc_rot_a);
                            context.evaluator->rotate_vector(rhs._enc_mat.at(selected_B_chunk), B_data_offset, *context.galois_keys, enc_rot_b);
                            // Multiply
                            context.evaluator->multiply_inplace(enc_rot_a, enc_rot_b);
                            context.evaluator->relinearize_inplace(enc_rot_a, *context.relin_keys);
                            context.evaluator->rescale_to_next_inplace(enc_rot_a);
                            
                            // Mask out slot zero by multiplying with mask
                            context.evaluator->mod_switch_to(enc_slot_zero_mask, enc_rot_a.parms_id(), enc_rot_b);
                            context.evaluator->multiply_inplace(enc_rot_a, enc_rot_b);
                            context.evaluator->relinearize_inplace(enc_rot_a, *context.relin_keys);
                            context.evaluator->rescale_to_next_inplace(enc_rot_a);

                            // Rotate result value to desired slot
                            context.evaluator->rotate_vector_inplace(enc_rot_a, -R_data_offset, *context.galois_keys);

                            // Critical section, we lock the chunk we want to modify and add to result chunk
                            std::lock_guard<std::mutex> chunk_lock(result_chunks_mutex.at(selected_R_chunk));
                            context.evaluator->mod_switch_to_inplace(result._enc_mat.at(selected_R_chunk), enc_rot_a.parms_id());
                            result._enc_mat.at(selected_R_chunk).scale() = enc_rot_a.scale();
                            context.evaluator->add_inplace(result._enc_mat.at(selected_R_chunk), enc_rot_a);
                            
                            result._is_zero.at(R_idx) = false;
                        }
                    }
                })
            );
        }
    }

    // Wait for all row threads to finish 
    for (uint64_t idx = 0; idx < n_threads; idx++) {
        if ((threads.at(idx) != nullptr) && (threads.at(idx)->joinable()))
        {
            threads.at(idx)->join();
        }
    }

    return result;
}


double SparseNaiveFHE::sparsity() const
{
    // Equal to number of non-zero entries in `is_zero` array
    uint64_t nzv = 0;
    for(uint64_t p = 0; p < rows()*cols(); p++)
    {
        if (this->_is_zero.at(p) == false)
        {
            nzv++;
        }
    }

    return static_cast<double>(nzv)/static_cast<double>(rows()*cols());
}

}; // namespace SparseFHE
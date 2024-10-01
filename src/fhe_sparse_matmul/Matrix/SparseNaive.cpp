#include <fhe_sparse_matmul/Matrix/SparseNaive.hpp>
#include <fhe_sparse_matmul/utils.hpp>
#include <assert.h>

namespace SparseFHE {


SparseNaiveFHE::SparseNaiveFHE(uint64_t rows, uint64_t cols, uint64_t chunk_size, const double* const data, SealCKKSRuntimeContext& context)
{
    this->_rows=rows; this->_cols=cols; this->_chunk_size = chunk_size;
    auto plain_matrix = std::shared_ptr<double>(new double[rows*cols]);
    memcpy(plain_matrix.get(), data, sizeof(double)*rows*cols);


    this->_is_zero = std::shared_ptr<uint8_t>(new uint8_t[rows*cols]);
    for(uint32_t p = 0; p < this->_rows * this->_cols; p++)
    {
        this->_is_zero.get()[p] = (plain_matrix.get()[p] == 0);
    }

    // Encode & Encrypt all required matrices
    size_t slot_count = context.ckks_encoder->slot_count();
    assert(chunk_size <= slot_count);
    std::vector<double> mat_values;
    mat_values.assign(plain_matrix.get(), plain_matrix.get() + (this->rows() * this->cols()));
    std::vector<std::vector<double>> mat_v = chunk_values(mat_values, chunk_size, slot_count);
    this->_enc_mat = encrypt_values(mat_v, context);
}


void SparseNaiveFHE::decrypt(const SealCKKSSecretContext& context, double* const output) const
{
    // First zero all elements in output matrix
    for (uint64_t idx = 0; idx < (cols() * rows()); idx++)
    {
        output[idx] = 0.0;
    }

    size_t chunk_overflow = (cols() * rows()) % this->_chunk_size;
    for (uint64_t chunk = 0; chunk < this->_enc_mat.size(); chunk++)
    {
        seal::Plaintext plain_result;
        std::vector<double> result_values;
        context.decryptor->decrypt(_enc_mat.at(chunk), plain_result);
        context.ckks_encoder->decode(plain_result, result_values);

        // When the matrix does not fit perfectly into chunks, we need to only iterate part-way through
        // the chunk
        size_t chunk_size_limit = this->_chunk_size;
        if ((chunk == (this->_enc_mat.size() - 1)) && (chunk_overflow != 0)) {
            chunk_size_limit = chunk_overflow;
        }

        // Place the chunk values into the output array
        for (uint64_t elem = 0; elem < chunk_size_limit; elem++)
        {
            output[(chunk * this->_chunk_size) + elem] = result_values.at(elem);
        }
    }
}
    

SparseNaiveFHE SparseNaiveFHE::fhe_matmul(const SparseNaiveFHE& rhs, const SealCKKSRuntimeContext& context, uint64_t n_threads) const
{
    // Define intermediate ciphertexts and encode accumulator with zeros
    seal::Ciphertext enc_zeros;
    std::vector<double> zeros(context.ckks_encoder->slot_count(), 0);
    seal::Plaintext plain_zeros;
    context.ckks_encoder->encode(zeros, context.scale, plain_zeros);
    context.encryptor->encrypt(plain_zeros, enc_zeros);

    // We want to store our results in different chunks, allows us to multithread more efficiently
    SparseNaiveFHE result(this->rows(), rhs.cols(), this->_chunk_size);
    size_t n_result_chunks = std::ceil(static_cast<double>(result.rows()*result.cols())/static_cast<double>(this->_chunk_size));
    std::vector<seal::Ciphertext> result_chunks(n_result_chunks, enc_zeros);

    // Create index 0 mask
    seal::Ciphertext enc_index_zero_mask;
    seal::Plaintext plain_index_zero_mask;
    std::vector<double> index_zero_mask(context.ckks_encoder->slot_count(), 0);
    index_zero_mask.at(0) = 1;
    context.ckks_encoder->encode(index_zero_mask, context.scale, plain_index_zero_mask);
    context.encryptor->encrypt(plain_index_zero_mask, enc_index_zero_mask);

    std::vector<std::mutex> result_chunks_mutex(n_result_chunks);
    std::vector<std::shared_ptr<std::thread>> threads(n_threads);

    for (uint64_t A_row = 0; A_row < this->_rows; A_row++)
    {
        for (uint64_t B_col = 0; B_col < rhs.cols(); B_col++)
        {
            size_t thread_idx = ((A_row*rhs.cols()) + B_col) % n_threads;
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
                        uint64_t R_idx = ((A_row*rhs.cols()) + B_col);

                        if ((this->_is_zero.get()[A_idx] != true) && 
                            (rhs._is_zero.get()[B_idx] != true))
                        {
                            size_t selected_A_chunk = A_idx / this->_chunk_size;
                            size_t selected_B_chunk = B_idx / rhs._chunk_size;
                            size_t selected_R_chunk = R_idx / result._chunk_size;
                            size_t A_data_offset = A_idx % this->_chunk_size;
                            size_t B_data_offset = B_idx % rhs._chunk_size;
                            size_t R_data_offset = R_idx % result._chunk_size;

                            seal::Ciphertext enc_rot_a, enc_rot_b;

                            context.evaluator->rotate_vector(this->_enc_mat.at(selected_A_chunk), A_data_offset, *context.galois_keys, enc_rot_a);
                            context.evaluator->rotate_vector(rhs._enc_mat.at(selected_B_chunk), B_data_offset, *context.galois_keys, enc_rot_b);
                            // context.evaluator->multiply_inplace(enc_rot_a, enc_rot_b);
                            context.evaluator->multiply_inplace_sparse(enc_rot_a, enc_rot_b, std::vector<bool>(8192/2, 0), std::vector<bool>(8192/2, 0));
                            context.evaluator->relinearize_inplace(enc_rot_a, *context.relin_keys);
                            context.evaluator->rescale_to_next_inplace(enc_rot_a);
                            
                            context.evaluator->mod_switch_to(enc_index_zero_mask, enc_rot_a.parms_id(), enc_rot_b);
                            context.evaluator->multiply_inplace(enc_rot_a, enc_rot_b);
                            context.evaluator->relinearize_inplace(enc_rot_a, *context.relin_keys);
                            context.evaluator->rescale_to_next_inplace(enc_rot_a);

                            context.evaluator->rotate_vector_inplace(enc_rot_a, -R_data_offset, *context.galois_keys);

                            // Critical section, we lock the chunk we want to modify and add
                            std::lock_guard<std::mutex> chunk_lock(result_chunks_mutex.at(selected_R_chunk));
                            context.evaluator->mod_switch_to_inplace(result_chunks.at(selected_R_chunk), enc_rot_a.parms_id());
                            result_chunks.at(selected_R_chunk).scale() = enc_rot_a.scale();
                            context.evaluator->add_inplace(result_chunks.at(selected_R_chunk), enc_rot_a);
                            
                            result._is_zero.get()[(A_row*result.cols()) + B_col] = false;
                        }
                    }
                })
            );
        }
    }

    // Wait for all row threads to finish 
    for (size_t idx = 0; idx < n_threads; idx++) {
        if ((threads.at(idx) != nullptr) && (threads.at(idx)->joinable()))
        {
            threads.at(idx)->join();
        }
    }

    result._enc_mat = result_chunks;

    return result;
}


double SparseNaiveFHE::sparsity() const
{
    uint64_t nzv = 0;
    for(uint64_t p = 0; p < rows()*cols(); p++)
    {
        if (this->_is_zero.get()[p] == false)
        {
            nzv++;
        }
    }

    return static_cast<double>(nzv)/static_cast<double>(rows()*cols());
}

}; // namespace SparseFHE
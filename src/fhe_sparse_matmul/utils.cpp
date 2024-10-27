#include <fhe_sparse_matmul/utils.hpp>


namespace SparseFHE {


std::vector<std::vector<double>> chunk_values(const std::vector<double>& V, uint64_t chunk_sz, uint64_t slot_count)
{
    // To accommodate the matrix values within the ciphertext, we 'chuck' them into different ciphertexts
    std::vector<std::vector<double>> result;
    size_t n_mat_a_chunks = std::ceil(static_cast<double>(V.size()) / chunk_sz);
    for (size_t chunk_idx = 0; chunk_idx < n_mat_a_chunks; chunk_idx++)
    {
        auto chunk_start = V.begin() + (chunk_sz * chunk_idx);
        auto chunk_end = V.begin() + std::min((chunk_sz * (chunk_idx + 1)), V.size());
        uint64_t chunk_diff = chunk_end - chunk_start;
        std::vector<double> chunk_v(slot_count - chunk_diff, 0);
        chunk_v.insert(chunk_v.begin(), chunk_start, chunk_end);
        result.push_back(chunk_v);
    }

    // Handle the case where there are zero values, we just populate a zero value vector
    if (n_mat_a_chunks == 0)
    {
        result.push_back(std::vector<double>(slot_count, 0));
    }

    return result;
}


std::vector<seal::Ciphertext> encrypt_values(const std::vector<std::vector<double>>& values, const SealCKKSRuntimeContext& context)
{
    std::vector<seal::Ciphertext> encrypted;
    for (auto& v : values)
    {
        seal::Plaintext plain_values;
        seal::Ciphertext enc_values;
        context.ckks_encoder->encode(v, context.scale, plain_values);
        context.encryptor->encrypt(plain_values, enc_values);
        encrypted.push_back(enc_values);
    }

    return encrypted;
}


seal::Ciphertext encrypted_zeros(const SealCKKSRuntimeContext &context)
{
    seal::Ciphertext enc_zeros;
    
    std::vector<double> zeros(context.ckks_encoder->slot_count(), 0);
    seal::Plaintext plain_zeros;
    context.ckks_encoder->encode(zeros, context.scale, plain_zeros);
    context.encryptor->encrypt(plain_zeros, enc_zeros);

    return enc_zeros;
}


seal::Ciphertext encrypted_slot_zero_mask(const SealCKKSRuntimeContext& context)
{
    seal::Ciphertext enc_slot_zero_mask;

    seal::Plaintext plain_slot_zero_mask;
    std::vector<double> index_zero_mask(context.ckks_encoder->slot_count(), 0);
    index_zero_mask.at(0) = 1;
    context.ckks_encoder->encode(index_zero_mask, context.scale, plain_slot_zero_mask);
    context.encryptor->encrypt(plain_slot_zero_mask, enc_slot_zero_mask);

    return enc_slot_zero_mask;
}

}; // namespace SparseFHE
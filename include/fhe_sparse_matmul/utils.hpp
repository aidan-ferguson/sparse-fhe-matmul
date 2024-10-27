#pragma once
#ifndef __SPARSE_FHE_MATMUL__UTILS_HPP
#define __SPARSE_FHE_MATMUL__UTILS_HPP

#include <seal/seal.h>
#include <memory>
#include <vector>

#include <fhe_sparse_matmul/SealCKKSContext.hpp>

namespace SparseFHE {

/// @brief Chunk an array of values into an array of padded vectors according to the chunk size parameter
///        Each vector in the return value will have length equal to `slot_count` and contain `chunk_sz` values
/// @param V Input value vector
/// @param chunk_sz The number of values per chunk
/// @param slot_count The number of slots in the CKKS encoder
/// @return An array of vectors padded to `slot_count` size and containing `chunk_sz` values
std::vector<std::vector<double>> chunk_values(const std::vector<double>& V, uint64_t chunk_sz, uint64_t slot_count);

/// @brief Encrypt an array of chunks using an established CKKS runtime context
/// @param values The array of chunks representing the matrix to encrypt. Vectors must have length equal to the slot count of the CKKS encoder
/// @param context The public runtime CKKS context, used to encrypt the values 
/// @return An array of encrypted Ciphertext objects
std::vector<seal::Ciphertext> encrypt_values(const std::vector<std::vector<double>>& values, const SealCKKSRuntimeContext& context);

/// @brief Generate an encrypted set of zero values, typically used for initialisation of ciphertexts
/// @param context The public runtime CKKS context, used to encrypt the values  
/// @return A ciphertext containing '0.0' in all slots
seal::Ciphertext encrypted_zeros(const SealCKKSRuntimeContext &context);

/// @brief Generate a ciphertext with all slots set to zero, except slot 0. This is used for 'masking' slot 0 by multiplying
/// @param context The public runtime CKKS context, used to encrypt the values  
/// @return A ciphertext containing '0.0' in all slots, except slot 0 which will contain '1.0'
seal::Ciphertext encrypted_slot_zero_mask(const SealCKKSRuntimeContext& context);

/// @brief Zero all elements in input matrix 
/// @param m pointer to matrix to be modified
/// @param sz number of elements in matrix
inline void zero_matrix(double* m, uint64_t sz) noexcept
{
    for (uint64_t idx = 0; idx < sz; idx++)
        m[idx] = 0.0;
}

/// @brief Verify if two double precision floating point numbers are within some threshold (i.e. equal)
/// @param a left hand operand
/// @param b right hand operand
/// @param epsilon threshold in difference between a and b
/// @return true if |a-b| < epsilon, false otherwise
inline bool doubles_close(double a, double b, double epsilon = 1e-9) noexcept
{
    return std::fabs(a - b) <= epsilon;
}

}; // namespace SparseFHE


#endif // __SPARSE_FHE_MATMUL__UTILS_HPP
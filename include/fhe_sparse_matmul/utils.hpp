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
std::vector<std::vector<double>> chunk_values(std::vector<double> V, uint64_t chunk_sz, uint64_t slot_count);

/// @brief Encrypt an array of chunks using an established CKKS runtime context
/// @param values The array of chunks representing the matrix to encrypt. Vectors must have length equal to the slot count of the CKKS encoder
/// @param context The public runtime CKKS context used to encrypt the values 
/// @return An array of encrypted Ciphertext objects
std::vector<seal::Ciphertext> encrypt_values(std::vector<std::vector<double>>& values, const SealCKKSRuntimeContext& context);

}; // namespace SparseFHE


#endif // __SPARSE_FHE_MATMUL__UTILS_HPP
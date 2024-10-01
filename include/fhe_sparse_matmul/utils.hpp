#pragma once
#ifndef __SPARSE_FHE_MATMUL__UTILS_HPP
#define __SPARSE_FHE_MATMUL__UTILS_HPP

#include <seal/seal.h>
#include <memory>
#include <vector>

#include <fhe_sparse_matmul/SealCKKSContext.hpp>

namespace SparseFHE {

std::vector<std::vector<double>> chunk_values(std::vector<double> V, uint64_t chunk_sz, uint64_t slot_count);
std::vector<seal::Ciphertext> encrypt_values(std::vector<std::vector<double>>& values, const SealCKKSRuntimeContext& context);

}; // namespace SparseFHE


#endif // __SPARSE_FHE_MATMUL__UTILS_HPP
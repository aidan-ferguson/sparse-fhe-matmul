#pragma once
#ifndef __SPARSE_FHE_MATMUL__SPARSE_BASE_HPP
#define __SPARSE_FHE_MATMUL__SPARSE_BASE_HPP

#include <cstdint>

#include <fhe_sparse_matmul/SealCKKSContext.hpp>

namespace SparseFHE {


template <typename DerivedFHE>
class SparseBase {
public:

    virtual DerivedFHE fhe_matmul(const DerivedFHE& rhs, const SealCKKSRuntimeContext& context, uint64_t n_threads) const = 0;
    virtual void decrypt(const SealCKKSSecretContext& context, double* const output) const = 0;
    virtual double sparsity() const = 0;
 
    uint64_t rows() const {return _rows;}
    uint64_t cols() const {return _cols;}

protected:
    uint64_t _rows, _cols, _chunk_size;

    /// @brief Stores the encrypted value chunks
    std::vector<seal::Ciphertext> _enc_mat;
};


};

#endif // __SPARSE_FHE_MATMUL__SPARSE_BASE_HPP
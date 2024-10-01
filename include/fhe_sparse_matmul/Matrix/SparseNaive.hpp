#pragma once
#ifndef __SPARSE_FHE_MATMUL__SPARSE_NAIVE_HPP
#define __SPARSE_FHE_MATMUL__SPARSE_NAIVE_HPP

#include <fhe_sparse_matmul/Matrix/SparseBase.hpp>

namespace SparseFHE {


class SparseNaiveFHE : SparseBase<SparseNaiveFHE> {
public:
    SparseNaiveFHE(uint64_t rows, uint64_t cols, uint64_t chunk_size) {this->_rows=rows; this->_cols=cols; this->_chunk_size=chunk_size; this->_is_zero = std::shared_ptr<uint8_t>(new uint8_t[rows*cols]); };
    SparseNaiveFHE(uint64_t rows, uint64_t cols, uint64_t chunk_size, const double* const data, SealCKKSRuntimeContext& context);

    SparseNaiveFHE fhe_matmul(const SparseNaiveFHE& rhs, const SealCKKSRuntimeContext& context, uint64_t n_threads) const override;
    void decrypt(const SealCKKSSecretContext& context, double* const output) const override;
    double sparsity() const override;

protected:
    std::shared_ptr<uint8_t> _is_zero;
};

};

#endif // __SPARSE_FHE_MATMUL__SPARSE_NAIVE_HPP
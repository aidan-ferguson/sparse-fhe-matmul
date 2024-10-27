#pragma once
#ifndef __SPARSE_FHE_MATMUL__SPARSE_NAIVE_HPP
#define __SPARSE_FHE_MATMUL__SPARSE_NAIVE_HPP

#include <fhe_sparse_matmul/Matrix/SparseBase.hpp>

namespace SparseFHE {

/// @brief Class that allows for encrypted homomorphic naive sparse matrix multiplication
class SparseNaiveFHE : SparseBase<SparseNaiveFHE> {
public:

    /// @brief Create an encrypted empty SparseNaiveFHE object without any data
    /// @param rows Number of rows in the matrix
    /// @param cols Number of columns in the matrix
    /// @param chunk_size Chunk size of the encrypted matrix
    SparseNaiveFHE(uint64_t rows, uint64_t cols, uint64_t chunk_size) {this->_rows=rows; this->_cols=cols; this->_chunk_size=chunk_size; this->_is_zero = std::vector<bool>(rows*cols, true); };
    
    /// @brief Create an encrypted SparseNaiveFHE object initialised with some matrix data
    /// @param rows Number of rows in the matrix
    /// @param cols Number of columns in the matrix
    /// @param chunk_size Chunk size of the encrypted matrix
    /// @param data Pointer to row-major matrix data to encrypt
    /// @param context Public SEAL CKKS context
    SparseNaiveFHE(uint64_t rows, uint64_t cols, uint64_t chunk_size, const double* const data, SealCKKSRuntimeContext& context);

    /// @brief Perform matrix multiplication on two SparseNaiveFHE objects
    /// @param rhs Right-hand side matrix
    /// @param context Public SEAL CKKS context
    /// @param n_threads Number of threads to perform the computation with
    /// @return The encrypted resultant matrix
    SparseNaiveFHE fhe_matmul(const SparseNaiveFHE& rhs, const SealCKKSRuntimeContext& context, uint64_t n_threads) const override;
    
    /// @brief Decrypt the current matrix
    /// @param context Private SEAL CKKS context
    /// @param output Pointer to pre-allocated double buffer for result
    void decrypt(const SealCKKSSecretContext& context, double* const output) const override;
    
    /// @brief Determine the sparsity of the encrypted matrix
    /// @return Sparsity of the encrypted matrix
    double sparsity() const override;

protected:

    /// @brief Parallel matrix that indicates if the corresponding entry in the encrypted matrix is zero
    ///        we expose some information about the strucure of the matrix here.
    std::vector<bool> _is_zero;
};

};

#endif // __SPARSE_FHE_MATMUL__SPARSE_NAIVE_HPP
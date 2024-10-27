#pragma once
#ifndef __SPARSE_FHE_MATMUL__SPARSE_ELLPACK_HPP
#define __SPARSE_FHE_MATMUL__SPARSE_ELLPACK_HPP

#include <fhe_sparse_matmul/Matrix/SparseBase.hpp>

namespace SparseFHE {

/// @brief Class that allows for encrypted homomorphic ELLPACK sparse matrix multiplication
class SparseELLPACKFHE : public SparseBase<SparseELLPACKFHE> {
public:

    /// @brief Create an encrypted empty SparseELLPACKFHE object without any data
    /// @param rows Number of rows in the matrix
    /// @param cols Number of columns in the matrix
    /// @param chunk_size Chunk size of the encrypted matrix
    SparseELLPACKFHE(uint64_t rows, uint64_t cols, uint64_t chunk_size) {this->_rows=rows; this->_cols=cols; this->_chunk_size=chunk_size; this->_col_indices.resize(rows); this->_row_nzv.resize(rows);};
    
    /// @brief Create an encrypted SparseELLPACKFHE object initialised with some matrix data
    /// @param rows Number of rows in the matrix
    /// @param cols Number of columns in the matrix
    /// @param chunk_size Chunk size of the encrypted matrix
    /// @param data Pointer to row-major matrix data to encrypt
    /// @param context Public SEAL CKKS context
    SparseELLPACKFHE(uint64_t rows, uint64_t cols, uint64_t chunk_size, const double* const data, SealCKKSRuntimeContext& context);

    /// @brief Perform matrix multiplication on two SparseELLPACKFHE objects
    /// @param rhs Right-hand side matrix
    /// @param context Public SEAL CKKS context
    /// @param n_threads Number of threads to perform the computation with
    /// @return The encrypted resultant matrix
    SparseELLPACKFHE fhe_matmul(const SparseELLPACKFHE& rhs, const SealCKKSRuntimeContext& context, uint64_t n_threads) const override;
    
    /// @brief Decrypt the current matrix
    /// @param context Private SEAL CKKS context
    /// @param output Pointer to pre-allocated double buffer for result
    void decrypt(const SealCKKSSecretContext& context, double* const output) const override;
    
    /// @brief Determine the sparsity of the encrypted matrix
    /// @return Sparsity of the encrypted matrix
    double sparsity() const override;

protected:

    /// @brief When multiplying two matrices, compute what nzv attribues and column indices will be
    /// @param rhs The RHS operand
    /// @param result The result matrix, will be modified
    void compute_result_metadata(const SparseELLPACKFHE& rhs, SparseELLPACKFHE& result) const;

    /// @brief Calculates the index into the ELLPACK values matrix given a row and column index in the 'plaintext' matrix
    /// @param ellpack The ELLPACK FHE object to reference for all metadata arrays
    /// @param row The row we are querying
    /// @param col The column we are querying
    /// @return The index into the ELLPACK values matrix required to represent a value at mat[row, col]
    uint64_t get_ellpack_index(const SparseELLPACKFHE& ellpack, uint64_t row, uint64_t col) const;

    /// @brief Stores the column of the associated entries in the encrypted values array
    std::vector<std::vector<uint64_t>> _col_indices;

    /// @brief Stores how many non-zero values there are on each row. This is not part of the standard
    ///        ellpack format which relies on checking the number of non-zero values per row at multiplication
    ///        time by checking if the current value is zero. We cannot do conditional logic as our values are
    ///        encrypted, so we must store them in this format.
    std::vector<uint64_t> _row_nzv;

    /// @brief We do sometimes need the max non-zero value as it is the dimensionality of the internal matrices
    ///        so we don't have to keep recomputing it from `row_nzv` we cache it here 
    uint64_t _max_nzv;

};

}; // namespace SparseFHE

#endif // __SPARSE_FHE_MATMUL__SPARSE_ELLPACK_HPP
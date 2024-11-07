#pragma once
#ifndef __SPARSE_FHE_MATMUL__SPARSE_CSR_HPP
#define __SPARSE_FHE_MATMUL__SPARSE_CSR_HPP

#include <fhe_sparse_matmul/Matrix/SparseBase.hpp>

namespace SparseFHE {

/// @brief Class that allows for encrypted homomorphic CSR sparse matrix multiplication
class SparseCSRFHE : public SparseBase<SparseCSRFHE> {
public:

    /// @brief Create an encrypted empty SparseCSRFHE object without any data
    /// @param rows Number of rows in the matrix
    /// @param cols Number of columns in the matrix
    /// @param chunk_size Chunk size of the encrypted matrix
    SparseCSRFHE(uint64_t rows, uint64_t cols, uint64_t chunk_size) {this->_rows=rows; this->_cols=cols; this->_chunk_size=chunk_size;};
    
    /// @brief Create an encrypted SparseCSRFHE object initialised with some matrix data
    /// @param rows Number of rows in the matrix
    /// @param cols Number of columns in the matrix
    /// @param chunk_size Chunk size of the encrypted matrix
    /// @param data Pointer to row-major matrix data to encrypt
    /// @param context Public SEAL CKKS context
    SparseCSRFHE(uint64_t rows, uint64_t cols, uint64_t chunk_size, const double* const data, SealCKKSRuntimeContext& context);

    /// @brief 
    /// @param stream 
    /// @param context 
    SparseCSRFHE(std::stringstream& stream, SealCKKSRuntimeContext& context) {this->deserialize(stream, context);}

    /// @brief Perform matrix multiplication on two SparseCSRFHE objects
    /// @param rhs Right-hand side matrix
    /// @param context Public SEAL CKKS context
    /// @param n_threads Number of threads to perform the computation with
    /// @return The encrypted resultant matrix
    SparseCSRFHE fhe_matmul(const SparseCSRFHE& rhs, const SealCKKSRuntimeContext& context, uint64_t n_threads) const override;
    
    /// @brief Decrypt the current matrix
    /// @param context Private SEAL CKKS context
    /// @param output Pointer to pre-allocated double buffer for result
    void decrypt(const SealCKKSSecretContext& context, double* const output) const override;
    
    /// @brief Determine the sparsity of the encrypted matrix
    /// @return Sparsity of the encrypted matrix
    double sparsity() const override;

    /// @brief 
    /// @param stream 
    void serialize(std::stringstream& stream) const override;

    /// @brief 
    /// @param stream 
    void deserialize(std::stringstream& stream, const SealCKKSRuntimeContext& context) override;

protected:

    /// @brief When multiplying two matrices, compute what the col index and row index arrays will look like ahead of homomorphic computation
    /// @param rhs The RHS operand
    /// @param result The result matrix, will be modified with result
    void compute_result_metadata(const SparseCSRFHE& rhs, SparseCSRFHE& result) const;

    /// @brief Calculates the index into the CSR values array required given a row and column
    ///        used for inserting encrypted values into a CSR object with pre-computed column and row index arrays
    /// @param csr The CSR FHE object to reference for column and row index arrays to compute value index 
    /// @param row The row we are querying
    /// @param col The column we are querying
    /// @return The index into the CSR values array required to represent a value at mat[row, col]
    uint64_t get_csr_index(const SparseCSRFHE& csr, uint64_t row, uint64_t col) const;
    
    /// @brief Vector to store column indices of encrypted values 
    std::vector<uint64_t> _col_indices;
    
    /// @brief Vector to store row indices of encrypted values
    std::vector<uint64_t> _row_indices;

};

}; // namespace SparseFHE

#endif // __SPARSE_FHE_MATMUL__SPARSE_CSR_HPP
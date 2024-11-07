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
    /// @brief 
    /// @param stream 
    virtual void serialize(std::stringstream& stream) const
    {
        // Write rows, columns, chunk size and vector size
        size_t vector_size = _enc_mat.size();
        stream.write(reinterpret_cast<const char*>(&this->_rows), sizeof(uint64_t));
        stream.write(reinterpret_cast<const char*>(&this->_cols), sizeof(uint64_t));
        stream.write(reinterpret_cast<const char*>(&this->_chunk_size), sizeof(uint64_t));
        stream.write(reinterpret_cast<const char*>(&vector_size), sizeof(size_t));

        // Now write ciphertexts
        for (const auto& enc : this->_enc_mat) {
            enc.save(stream);
        }
    }

    
    virtual void deserialize(std::stringstream& stream, const SealCKKSRuntimeContext& context)
    {
        // In same order, read in rows, columns, etc...
        size_t vector_size;
        stream.read(reinterpret_cast<char*>(&this->_rows), sizeof(uint64_t));
        stream.read(reinterpret_cast<char*>(&this->_cols), sizeof(uint64_t));
        stream.read(reinterpret_cast<char*>(&this->_chunk_size), sizeof(uint64_t));
        stream.read(reinterpret_cast<char*>(&vector_size), sizeof(size_t));

        this->_enc_mat.resize(vector_size);
        for (auto& cipher : this->_enc_mat) {
            cipher.load(*context.context, stream);  // Each `load` reads from the stream
        }
    }

    uint64_t _rows, _cols, _chunk_size;

    /// @brief Stores the encrypted value chunks
    std::vector<seal::Ciphertext> _enc_mat;
};


};

#endif // __SPARSE_FHE_MATMUL__SPARSE_BASE_HPP
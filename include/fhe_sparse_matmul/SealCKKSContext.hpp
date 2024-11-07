#pragma once
#ifndef __SPARSE_FHE_MATMUL__SEAL_CKKS_CONTEXT_HPP
#define __SPARSE_FHE_MATMUL__SEAL_CKKS_CONTEXT_HPP

#include <seal/seal.h>
#include <memory>

namespace SparseFHE {


/// @brief Structure which contains SEAL objects that are public, that is they can be
///        shared without exposing sensitive information.
struct SealCKKSRuntimeContext {
public:
    std::shared_ptr<seal::SEALContext> context;
    std::shared_ptr<seal::EncryptionParameters> params;
    std::shared_ptr<seal::PublicKey>    public_key;
    std::shared_ptr<seal::Encryptor>    encryptor;
    std::shared_ptr<seal::Evaluator>    evaluator;
    std::shared_ptr<seal::RelinKeys>    relin_keys;
    std::shared_ptr<seal::GaloisKeys>   galois_keys;
    std::shared_ptr<seal::CKKSEncoder>  ckks_encoder;
    double scale;

    // Attributes are set by SealCKKSContext
    SealCKKSRuntimeContext() = default;
    SealCKKSRuntimeContext(std::stringstream& stream) {this->deserialize(stream);}

    void serialize(std::stringstream& stream) const;
    void deserialize(std::stringstream& stream);
};


/// @brief Structure which contains SEAL objects that are private, this object should not be
///        shared out-with the client.
struct SealCKKSSecretContext {
public:
    std::shared_ptr<seal::Decryptor>   decryptor;
    std::shared_ptr<seal::CKKSEncoder> ckks_encoder;

    // Attributes are set by SealCKKSContext
    SealCKKSSecretContext() = default;
};


/// @brief Struture which stores the entire SEAL CKKS context used for encryption, matrix multiplication and decryption
struct SealCKKSContext {
    public:
        SealCKKSSecretContext secret;
        SealCKKSRuntimeContext runtime;

    /// @brief Construct a SEAL CKKS context with a specified polynomial modulus degree
    /// @param poly_modulus_degree The polynomial modulus degree to use for the CKKS context
    SealCKKSContext(size_t poly_modulus_degree);
};


}; // namespace SparseFHE


#endif // __SPARSE_FHE_MATMUL__SEAL_CKKS_CONTEXT_HPP
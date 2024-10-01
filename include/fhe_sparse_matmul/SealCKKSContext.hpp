#pragma once
#ifndef __SPARSE_FHE_MATMUL__SEAL_CKKS_CONTEXT_HPP
#define __SPARSE_FHE_MATMUL__SEAL_CKKS_CONTEXT_HPP

#include <seal/seal.h>
#include <memory>

namespace SparseFHE {

struct SealCKKSRuntimeContext {
public:
    std::shared_ptr<seal::Encryptor>    encryptor;
    std::shared_ptr<seal::Evaluator>    evaluator;
    std::shared_ptr<seal::RelinKeys>    relin_keys;
    std::shared_ptr<seal::GaloisKeys>   galois_keys;
    std::shared_ptr<seal::CKKSEncoder>  ckks_encoder;
    double scale;

    // Attributes are set by SealCKKSContext
    SealCKKSRuntimeContext() = default;
};


struct SealCKKSSecretContext {
public:
    std::shared_ptr<seal::Decryptor>   decryptor;
    std::shared_ptr<seal::CKKSEncoder> ckks_encoder;

    // Attributes are set by SealCKKSContext
    SealCKKSSecretContext() = default;
};

struct SealCKKSContext {
    public:
        std::shared_ptr<seal::SEALContext> context;
        SealCKKSSecretContext secret;
        SealCKKSRuntimeContext runtime;

    SealCKKSContext(size_t poly_modulus_degree);
};


};


#endif // __SPARSE_FHE_MATMUL__SEAL_CKKS_CONTEXT_HPP
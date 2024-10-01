#include <fhe_sparse_matmul/SealCKKSContext.hpp>

SparseFHE::SealCKKSContext::SealCKKSContext(size_t poly_modulus_degree)
{
    seal::EncryptionParameters params(seal::scheme_type::ckks);

    params.set_poly_modulus_degree(poly_modulus_degree);
    params.set_coeff_modulus(seal::CoeffModulus::Create(poly_modulus_degree, { 50, 40, 40, 40, 40 }));

    this->runtime.scale = pow(2.0, 40);

    this->context = std::make_shared<seal::SEALContext>(params);
    seal::KeyGenerator keygen(*this->context);
    seal::SecretKey secret_key = keygen.secret_key();
    
    seal::PublicKey public_key;
    keygen.create_public_key(public_key);

    this->runtime.relin_keys = std::make_shared<seal::RelinKeys>();
    keygen.create_relin_keys(*this->runtime.relin_keys);

    this->runtime.galois_keys = std::make_shared<seal::GaloisKeys>();
    keygen.create_galois_keys(*this->runtime.galois_keys);

    this->runtime.encryptor = std::make_shared<seal::Encryptor>(*context, public_key);
    this->runtime.evaluator = std::make_shared<seal::Evaluator>(*context);
    this->secret.decryptor = std::make_shared<seal::Decryptor>(*context, secret_key);

    this->runtime.ckks_encoder = std::make_shared<seal::CKKSEncoder>(*context);
    this->secret.ckks_encoder = this->runtime.ckks_encoder;
}
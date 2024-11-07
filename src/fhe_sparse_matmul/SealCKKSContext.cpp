#include <fhe_sparse_matmul/SealCKKSContext.hpp>

SparseFHE::SealCKKSContext::SealCKKSContext(size_t poly_modulus_degree)
{
    this->runtime.params = std::make_shared<seal::EncryptionParameters>(seal::scheme_type::ckks);

    this->runtime.params->set_poly_modulus_degree(poly_modulus_degree);
    this->runtime.params->set_coeff_modulus(seal::CoeffModulus::Create(poly_modulus_degree, { 50, 40, 40, 40, 40 }));

    this->runtime.scale = pow(2.0, 40);

    this->runtime.context = std::make_shared<seal::SEALContext>(*this->runtime.params);
    seal::KeyGenerator keygen(*this->runtime.context);
    seal::SecretKey secret_key = keygen.secret_key();
    
    seal::PublicKey public_key;
    keygen.create_public_key(public_key);

    this->runtime.relin_keys = std::make_shared<seal::RelinKeys>();
    keygen.create_relin_keys(*this->runtime.relin_keys);

    this->runtime.galois_keys = std::make_shared<seal::GaloisKeys>();
    keygen.create_galois_keys(*this->runtime.galois_keys);

    this->runtime.encryptor = std::make_shared<seal::Encryptor>(*runtime.context, public_key);
    this->runtime.evaluator = std::make_shared<seal::Evaluator>(*runtime.context);
    this->secret.decryptor = std::make_shared<seal::Decryptor>(*runtime.context, secret_key);

    this->runtime.ckks_encoder = std::make_shared<seal::CKKSEncoder>(*runtime.context);
    this->secret.ckks_encoder = this->runtime.ckks_encoder;
}
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
    
    this->runtime.public_key = std::make_shared<seal::PublicKey>();
    keygen.create_public_key(*runtime.public_key);

    this->runtime.relin_keys = std::make_shared<seal::RelinKeys>();
    keygen.create_relin_keys(*this->runtime.relin_keys);

    this->runtime.galois_keys = std::make_shared<seal::GaloisKeys>();
    keygen.create_galois_keys(*this->runtime.galois_keys);

    this->runtime.encryptor = std::make_shared<seal::Encryptor>(*runtime.context, *runtime.public_key);
    this->runtime.evaluator = std::make_shared<seal::Evaluator>(*runtime.context);
    this->secret.decryptor = std::make_shared<seal::Decryptor>(*runtime.context, secret_key);

    this->runtime.ckks_encoder = std::make_shared<seal::CKKSEncoder>(*runtime.context);
    this->secret.ckks_encoder = this->runtime.ckks_encoder;
}


void SparseFHE::SealCKKSRuntimeContext::serialize(std::stringstream& stream) const
{
    // Note, not yet using Serializable<T> interface in SEAL, so these may be quite large
    this->params->save(stream);
    this->public_key->save(stream);
    this->relin_keys->save(stream);
    this->galois_keys->save(stream);
    stream.write(reinterpret_cast<const char*>(&this->scale), sizeof(double));
}


void SparseFHE::SealCKKSRuntimeContext::deserialize(std::stringstream& stream)
{
    this->params = std::make_shared<seal::EncryptionParameters>();
    this->params->load(stream);
    this->context = std::make_shared<seal::SEALContext>(*this->params);

    this->public_key = std::make_shared<seal::PublicKey>();
    this->public_key->load(*this->context, stream);
    this->encryptor = std::make_shared<seal::Encryptor>(*context, *public_key);
    this->evaluator = std::make_shared<seal::Evaluator>(*context);

    this->ckks_encoder = std::make_shared<seal::CKKSEncoder>(*context);

    this->relin_keys = std::make_shared<seal::RelinKeys>();
    this->relin_keys->load(*context, stream);
    this->galois_keys = std::make_shared<seal::GaloisKeys>();
    this->galois_keys->load(*context, stream);
    stream.read(reinterpret_cast<char*>(&this->scale), sizeof(double));
}
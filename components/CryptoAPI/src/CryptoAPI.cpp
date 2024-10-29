#include "CryptoAPI.h"
#include "MbedtlsModule.h"
#include "WolfsslModule.h"
#include "MicroeccModule.h"

static const char *TAG = "CryptoAPI";

CryptoAPI::CryptoAPI()
{
  mbedtls_module = new MbedtlsModule(commons);
  wolfssl_module = new WolfsslModule(commons);
  microecc_module = new MicroeccModule(commons);
}

CryptoAPI::~CryptoAPI()
{
  delete mbedtls_module;
  delete wolfssl_module;
  delete microecc_module;
}

int CryptoAPI::init(Libraries library, Algorithms algorithm, Hashes hash, size_t length_of_shake256)
{
  this->print_init_configuration(library, algorithm, hash, length_of_shake256);
  this->chosen_library = library;

  if (library == Libraries::MBEDTLS_LIB)
  {
    delete wolfssl_module;
    delete microecc_module;
  }

  if (this->chosen_library == Libraries::MBEDTLS_LIB)
  {
    return mbedtls_module->init(algorithm, hash);
  }

  if (this->chosen_library == Libraries::WOLFSSL_LIB)
  {
    return wolfssl_module->init(algorithm, hash, length_of_shake256);
  }

  return microecc_module->init(hash);
}

int CryptoAPI::get_signature_size()
{
  if (this->chosen_library == Libraries::MBEDTLS_LIB)
  {
    return mbedtls_module->get_signature_size();
  }

  if (this->chosen_library == Libraries::WOLFSSL_LIB)
  {
    return wolfssl_module->get_signature_size();
  }

  return 64;
}

int CryptoAPI::gen_rsa_keys(int rsa_key_size, int rsa_exponent)
{
  if (this->chosen_library == Libraries::MBEDTLS_LIB)
  {
    return mbedtls_module->gen_rsa_keys(rsa_key_size, rsa_exponent);
  }

  if (this->chosen_library == Libraries::WOLFSSL_LIB)
  {
    return wolfssl_module->gen_rsa_keys(rsa_key_size, rsa_exponent);
  }

  return -1;
}

int CryptoAPI::gen_keys()
{
  if (this->chosen_library == Libraries::WOLFSSL_LIB)
  {
    return wolfssl_module->gen_keys();
  }

  if (this->chosen_library == Libraries::MBEDTLS_LIB)
  {
    return mbedtls_module->gen_keys();
  }

  return microecc_module->gen_keys();
}

int CryptoAPI::sign(const unsigned char *message, size_t message_length, unsigned char *signature, size_t *signature_length)
{
  if (this->chosen_library == Libraries::MBEDTLS_LIB)
  {
    return mbedtls_module->sign(message, message_length, signature, signature_length);
  }

  if (this->chosen_library == Libraries::WOLFSSL_LIB)
  {
    return wolfssl_module->sign(message, message_length, signature, signature_length);
  }

  return microecc_module->sign(message, message_length, signature);
}

int CryptoAPI::sign(const unsigned char *message, size_t message_length, unsigned char *signature)
{
  if (this->chosen_library != Libraries::MICROECC_LIB)
  {
    return -1000;
  }

  return this->sign(message, message_length, signature, 0);
}

int CryptoAPI::verify(const unsigned char *message, size_t message_length, unsigned char *signature, size_t signature_length)
{
  if (this->chosen_library == Libraries::MBEDTLS_LIB)
  {
    return mbedtls_module->verify(message, message_length, signature, signature_length);
  }

  if (this->chosen_library == Libraries::WOLFSSL_LIB)
  {
    return wolfssl_module->verify(message, message_length, signature, signature_length);
  }

  return microecc_module->verify(message, message_length, signature);
}

int CryptoAPI::verify(const unsigned char *message, size_t message_length, unsigned char *signature)
{
  if (this->chosen_library != Libraries::MICROECC_LIB)
  {
    return -1000;
  }

  return this->verify(message, message_length, signature, 0);
}

void CryptoAPI::close()
{
  if (this->chosen_library == Libraries::MBEDTLS_LIB)
  {
    mbedtls_module->close();
    return;
  }

  if (this->chosen_library == Libraries::WOLFSSL_LIB)
  {
    wolfssl_module->close();
    return;
  }

  microecc_module->close();
}

Algorithms CryptoAPI::get_chosen_algorithm()
{
  return commons.get_chosen_algorithm();
}

Libraries CryptoAPI::get_chosen_library()
{
  return this->chosen_library;
}

void CryptoAPI::print_init_configuration(Libraries library, Algorithms algorithm, Hashes hash, size_t length_of_shake256)
{
  const char *library_str;
  switch (library)
  {
  case MBEDTLS_LIB:
    library_str = "MBEDTLS";
    break;
  case WOLFSSL_LIB:
    library_str = "WOLFSSL";
    break;
  case MICROECC_LIB:
    library_str = "MICROECC";
    break;
  default:
    library_str = "UNKNOWN";
    break;
  }

  const char *algorithm_str;
  switch (algorithm)
  {
  case ECDSA_BP256R1:
    algorithm_str = "ECDSA_BP256R1";
    break;
  case ECDSA_BP512R1:
    algorithm_str = "ECDSA_BP512R1";
    break;
  case ECDSA_SECP256R1:
    algorithm_str = "ECDSA_SECP256R1";
    break;
  case ECDSA_SECP521R1:
    algorithm_str = "ECDSA_SECP521R1";
    break;
  case EDDSA_25519:
    algorithm_str = "EDDSA_25519";
    break;
  case EDDSA_448:
    algorithm_str = "EDDSA_448";
    break;
  case RSA:
    algorithm_str = "RSA";
    break;
  default:
    algorithm_str = "UNKNOWN";
    break;
  }

  const char *hash_str;
  switch (hash)
  {
  case MY_SHA_256:
    hash_str = "SHA_256";
    break;
  case MY_SHA_512:
    hash_str = "SHA_512";
    break;
  case MY_SHA3_256:
    hash_str = "SHA3_256";
    break;
  case MY_SHAKE_256:
    hash_str = "SHAKE_256";
    break;
  default:
    hash_str = "UNKNOWN";
    break;
  }

  ESP_LOGI(TAG, "> INITIALIZED LIBRARY [ %s ] WITH ALGORITHM [ %s ] AND HASH [ %s ]", library_str, algorithm_str, hash_str);

  if (hash == Hashes::MY_SHAKE_256)
  {
    ESP_LOGI(TAG, "SHAKE_256 LENGTH [ %zu ]", length_of_shake256);
  }
}
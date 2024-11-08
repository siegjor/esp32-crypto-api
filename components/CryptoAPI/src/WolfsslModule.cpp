#include "WolfsslModule.h"

static const char *TAG = "WolfsslModule";

WolfsslModule::WolfsslModule(CryptoApiCommons &commons) : commons(commons) {}

int WolfsslModule::init(Algorithms algorithm, Hashes hash, size_t length_of_shake256)
{
  commons.set_chosen_algorithm(algorithm);
  commons.set_chosen_hash(hash);
  commons.set_shake256_hash_length(length_of_shake256);

  int initial_memory = esp_get_free_heap_size();
  unsigned long start_time = esp_timer_get_time() / 1000;

  wolfCrypt_Init();

  rng = (WC_RNG *)malloc(sizeof(WC_RNG));
  int ret = wc_InitRng(rng);
  if (ret != 0)
  {
    commons.log_error("wc_InitRng");
    return ret;
  }

  switch (commons.get_chosen_algorithm())
  {
  case EDDSA_25519:
    wolf_ed25519_key = (ed25519_key *)malloc(sizeof(ed25519_key));
    ret = wc_ed25519_init(wolf_ed25519_key);
    if (ret != 0)
    {
      commons.log_error("wc_ed25519_init");
      return ret;
    }
    break;
  case RSA:
    wolf_rsa_key = (RsaKey *)malloc(sizeof(RsaKey));
    ret = wc_InitRsaKey(wolf_rsa_key, NULL);
    if (ret != 0)
    {
      commons.log_error("wc_InitRsaKey");
      return ret;
    }
    break;
  case ECDSA_BP256R1:
  case ECDSA_BP512R1:
  case ECDSA_SECP256R1:
  case ECDSA_SECP521R1:
    wolf_ecc_key = (ecc_key *)malloc(sizeof(ecc_key));
    ret = wc_ecc_init(wolf_ecc_key);
    if (ret != 0)
    {
      commons.log_error("wc_ecc_init");
      return ret;
    }
    break;
  case EDDSA_448:
    wolf_ed448_key = (ed448_key *)malloc(sizeof(ed448_key));
    ret = wc_ed448_init(wolf_ed448_key);
    if (ret != 0)
    {
      commons.log_error("wc_ed448_init");
      return ret;
    }
    break;
  }

  unsigned long end_time = esp_timer_get_time() / 1000;
  int final_memory = esp_get_free_heap_size();

  commons.print_elapsed_time(start_time, end_time, "init");
  commons.print_used_memory(initial_memory, final_memory, "init");

  commons.log_success("init");
  return 0;
}

int WolfsslModule::gen_keys()
{
  int ret;
  int curve_id = get_ecc_curve_id();
  int key_size = get_key_size(curve_id);

  int initial_memory = esp_get_free_heap_size();
  unsigned long start_time = esp_timer_get_time() / 1000;

  switch (commons.get_chosen_algorithm())
  {
  case EDDSA_25519:
    ret = wc_ed25519_make_key(rng, key_size, wolf_ed25519_key);
    if (ret != 0)
    {
      commons.log_error("wc_ed25519_make_key");
      return ret;
    }
    break;
  case ECDSA_BP256R1:
  case ECDSA_BP512R1:
  case ECDSA_SECP256R1:
  case ECDSA_SECP521R1:
  default:
    ret = wc_ecc_make_key_ex(rng, key_size, wolf_ecc_key, curve_id);
    if (ret != 0)
    {
      commons.log_error("wc_ecc_make_key_ex");
      return ret;
    }
    break;
  case EDDSA_448:
    ret = wc_ed448_make_key(rng, key_size, wolf_ed448_key);
    if (ret != 0)
    {
      commons.log_error("wc_ed448_make_key");
      return ret;
    }
    break;
  }

  unsigned long end_time = esp_timer_get_time() / 1000;
  int final_memory = esp_get_free_heap_size();

  commons.print_elapsed_time(start_time, end_time, "gen_keys");
  commons.print_used_memory(initial_memory, final_memory, "gen_keys");

  commons.log_success("gen_keys");
  return 0;
}

int WolfsslModule::gen_rsa_keys(unsigned int rsa_key_size, int rsa_exponent)
{
  int initial_memory = esp_get_free_heap_size();
  unsigned long start_time = esp_timer_get_time() / 1000;

  this->rsa_key_size = rsa_key_size;

  int ret = wc_MakeRsaKey(wolf_rsa_key, rsa_key_size, rsa_exponent, rng);
  if (ret != 0)
  {
    commons.log_error("wc_MakeRsaKey");
    return ret;
  }

  unsigned long end_time = esp_timer_get_time() / 1000;
  int final_memory = esp_get_free_heap_size();

  commons.print_elapsed_time(start_time, end_time, "gen_keys");
  commons.print_used_memory(initial_memory, final_memory, "gen_keys");

  commons.log_success("gen_keys");
  return 0;
}

int WolfsslModule::sign(const unsigned char *message, size_t message_length, unsigned char *signature, size_t *signature_length)
{
  int initial_memory = esp_get_free_heap_size();
  unsigned long start_time = esp_timer_get_time() / 1000;

  size_t hash_length = commons.get_hash_length();
  byte *hash = (byte *)malloc(hash_length * sizeof(byte));

  int ret = hash_message(message, message_length, hash);
  if (ret != 0)
  {
    commons.log_error("hash_message");
    return ret;
  }

  switch (commons.get_chosen_algorithm())
  {
  case EDDSA_25519:
    ret = wc_ed25519ph_sign_hash(hash, hash_length, signature, signature_length, wolf_ed25519_key, NULL, 0);
    if (ret != 0)
    {
      commons.log_error("wc_ed25519ph_sign_hash");
      return ret;
    }
    break;
  case RSA:
    ret = wc_RsaSSL_Sign(hash, hash_length, signature, *signature_length, wolf_rsa_key, rng);
    if (ret != *signature_length)
    {
      commons.log_error("wc_RsaSSL_Sign");
      return ret;
    }
    break;
  case ECDSA_BP256R1:
  case ECDSA_BP512R1:
  case ECDSA_SECP256R1:
  case ECDSA_SECP521R1:
    ret = wc_ecc_sign_hash(hash, hash_length, signature, signature_length, rng, wolf_ecc_key);
    if (ret != 0)
    {
      commons.log_error("wc_ecc_sign_hash");
      return ret;
    }
    break;
  case EDDSA_448:
    ret = wc_ed448ph_sign_hash(hash, hash_length, signature, signature_length, wolf_ed448_key, NULL, 0);
    if (ret != 0)
    {
      commons.log_error("wc_ed448ph_sign_hash");
      return ret;
    }
    break;
  }

  unsigned long end_time = esp_timer_get_time() / 1000;
  int final_memory = esp_get_free_heap_size();

  commons.print_elapsed_time(start_time, end_time, "sign");
  commons.print_used_memory(initial_memory, final_memory, "sign");

  free(hash);

  commons.log_success("sign");
  return 0;
}

int WolfsslModule::verify(const unsigned char *message, size_t message_length, unsigned char *signature, size_t signature_length)
{
  int initial_memory = esp_get_free_heap_size();
  unsigned long start_time = esp_timer_get_time() / 1000;

  size_t hash_length = commons.get_hash_length();
  byte *hash = (byte *)malloc(hash_length * sizeof(byte));

  int ret = hash_message(message, message_length, hash);
  if (ret != 0)
  {
    commons.log_error("hash_message");
    return ret;
  }

  // byte decrypted_signature[hash_length];
  byte *decrypted_signature = (byte *)malloc(hash_length * sizeof(byte));

  int verify_status = 0;
  switch (commons.get_chosen_algorithm())
  {
  case EDDSA_25519:
    ret = wc_ed25519ph_verify_hash(signature, signature_length, hash, hash_length, &verify_status, wolf_ed25519_key, NULL, 0);
    if (ret != 0)
    {
      commons.log_error("wc_ed25519ph_verify_hash");
      return ret;
    }

    if (verify_status != 1)
    {
      ESP_LOGE(TAG, "> Signature not valid.");
    }
    break;
  case RSA:
    ret = wc_RsaSSL_Verify(signature, signature_length, decrypted_signature, hash_length, wolf_rsa_key);
    if (ret != hash_length)
    {
      commons.log_error("wc_RsaSSL_Verify");
      return ret;
    }

    verify_status = memcmp(hash, decrypted_signature, hash_length);
    if (verify_status != 0)
    {
      ESP_LOGE(TAG, "> Signature not valid.");
    }
    break;
  case ECDSA_BP256R1:
  case ECDSA_BP512R1:
  case ECDSA_SECP256R1:
  case ECDSA_SECP521R1:
    ret = wc_ecc_verify_hash(signature, signature_length, hash, hash_length, &verify_status, wolf_ecc_key);
    if (ret != 0)
    {
      commons.log_error("wc_ecc_verify_hash");
      return ret;
    }

    if (verify_status != 1)
    {
      ESP_LOGE(TAG, "> Signature not valid.");
    }
    break;
  case EDDSA_448:
    ret = wc_ed448ph_verify_hash(signature, signature_length, hash, hash_length, &verify_status, wolf_ed448_key, NULL, 0);
    if (ret != 0)
    {
      commons.log_error("wc_ed448ph_verify_hash");
      return ret;
    }

    if (verify_status != 1)
    {
      ESP_LOGE(TAG, "> Signature not valid.");
    }
    break;
  }

  unsigned long end_time = esp_timer_get_time() / 1000;
  int final_memory = esp_get_free_heap_size();

  commons.print_elapsed_time(start_time, end_time, "verify");
  commons.print_used_memory(initial_memory, final_memory, "verify");

  free(hash);
  free(decrypted_signature);

  commons.log_success("verify");
  return 0;
}

void WolfsslModule::close()
{
  wolfCrypt_Cleanup();
  wc_FreeRng(rng);
  if (commons.get_chosen_algorithm() == Algorithms::RSA)
  {
    wc_FreeRsaKey(wolf_rsa_key);
  }
  else if (commons.get_chosen_algorithm() == Algorithms::EDDSA_25519)
  {
    wc_ed25519_free(wolf_ed25519_key);
  }
  else if (commons.get_chosen_algorithm() == Algorithms::EDDSA_448)
  {
    wc_ed448_free(wolf_ed448_key);
  }
  else
  {
    wc_ecc_free(wolf_ecc_key);
  }

  ESP_LOGI(TAG, "> wolfssl closed.");
}

int WolfsslModule::get_key_size(int curve_id)
{
  if (commons.get_chosen_algorithm() == Algorithms::EDDSA_25519)
  {
    return ED25519_PUB_KEY_SIZE;
  }
  else if (commons.get_chosen_algorithm() == Algorithms::EDDSA_448)
  {
    return ED448_PUB_KEY_SIZE;
  }
  else if (commons.get_chosen_algorithm() == Algorithms::RSA)
  {
    return rsa_key_size / 8;
  }
  else
  {
    return wc_ecc_get_curve_size_from_id(curve_id);
  }
}
int WolfsslModule::get_ecc_curve_id()
{
  if (commons.get_chosen_algorithm() == Algorithms::ECDSA_SECP256R1)
  {
    return ECC_SECP256R1;
  }
  else if (commons.get_chosen_algorithm() == Algorithms::ECDSA_SECP521R1)
  {
    return ECC_SECP521R1;
  }
  else if (commons.get_chosen_algorithm() == Algorithms::ECDSA_BP256R1)
  {
    return ECC_BRAINPOOLP256R1;
  }
  else
  {
    return ECC_BRAINPOOLP512R1;
  }
}

int WolfsslModule::hash_message(const unsigned char *message, size_t message_len, unsigned char *hash)
{
  switch (commons.get_chosen_hash())
  {
  case Hashes::MY_SHA_256:
    return wc_Sha256Hash(message, message_len, hash);
  case Hashes::MY_SHA_512:
    return wc_Sha512Hash(message, message_len, hash);
  case Hashes::MY_SHA3_256:
    return wc_Sha3_256Hash(message, message_len, hash);
  case Hashes::MY_SHAKE_256:
    return wc_Shake256Hash(message, message_len, hash, commons.get_hash_length());
  default:
    return wc_Sha256Hash(message, message_len, hash);
  }
}

size_t WolfsslModule::get_public_key_size()
{
  return get_key_size(get_ecc_curve_id());
}

int WolfsslModule::get_signature_size()
{
  if (commons.get_chosen_algorithm() == EDDSA_25519)
  {
    return ED25519_SIG_SIZE;
  }
  else if (commons.get_chosen_algorithm() == EDDSA_448)
  {
    return ED448_SIG_SIZE;
  }
  else if (commons.get_chosen_algorithm() == RSA)
  {
    return rsa_key_size / 8;
  }

  return ECC_MAX_SIG_SIZE;
}

int WolfsslModule::get_public_key_pem(unsigned char *public_key_pem)
{
  int ret;

  int initial_memory = esp_get_free_heap_size();
  unsigned long start_time = esp_timer_get_time() / 1000;

  size_t der_pub_key_size = get_public_key_der_size();
  unsigned char *der_pub_key = (unsigned char *)malloc(der_pub_key_size * sizeof(unsigned char));
  CertType cert_type;

  switch (commons.get_chosen_algorithm())
  {
  case EDDSA_25519:
    ret = wc_ed25519_export_public(wolf_ed25519_key, der_pub_key, &der_pub_key_size);
    cert_type = PUBLICKEY_TYPE;
    if (ret != 0)
    {
      commons.log_error("wc_ed25519_export_public");
      return ret;
    }
    break;
  case RSA:
    ret = wc_RsaKeyToPublicDer(wolf_rsa_key, der_pub_key, der_pub_key_size);
    cert_type = RSA_PUBLICKEY_TYPE;

    if (ret < 0)
    {
      commons.log_error("wc_RsaKeyToPublicDer");
      return ret;
    }
    else
    {
      der_pub_key_size = ret;
    }
    break;
  case ECDSA_BP256R1:
  case ECDSA_BP512R1:
  case ECDSA_SECP256R1:
  case ECDSA_SECP521R1:
  default:
    ret = wc_EccPublicKeyToDer(wolf_ecc_key, der_pub_key, der_pub_key_size, 0);
    cert_type = ECC_PUBLICKEY_TYPE;
    if (ret < 0)
    {
      commons.log_error("wc_EccPublicKeyToDer");
      return ret;
    }
    else
    {
      der_pub_key_size = ret;
    }
    break;
  case EDDSA_448:
    ret = wc_ed448_export_public(wolf_ed448_key, der_pub_key, &der_pub_key_size);
    cert_type = PUBLICKEY_TYPE;
    if (ret != 0)
    {
      commons.log_error("wc_ed448_export_public");
      return ret;
    }
    break;
  }

  ret = wc_DerToPem(der_pub_key, der_pub_key_size, public_key_pem, get_public_key_pem_size(), cert_type);
  if (ret < 0)
  {
    commons.log_error("wc_DerToPem");
    return ret;
  }

  public_key_pem[get_public_key_pem_size()] = '\0';

  unsigned long end_time = esp_timer_get_time() / 1000;
  int final_memory = esp_get_free_heap_size();

  commons.print_elapsed_time(start_time, end_time, "get_pub_key");
  commons.print_used_memory(initial_memory, final_memory, "get_pub_key");

  // ESP_LOGI(TAG, "Public key in PEM format: \n%s", public_key_pem);
  return 0;
}

size_t WolfsslModule::get_public_key_pem_size()
{
  if (commons.get_chosen_algorithm() == Algorithms::EDDSA_25519)
  {
    return 97;
  }
  else if (commons.get_chosen_algorithm() == Algorithms::EDDSA_448)
  {
    return 130;
  }
  else if (commons.get_chosen_algorithm() == Algorithms::ECDSA_SECP256R1 || commons.get_chosen_algorithm() == Algorithms::ECDSA_BP256R1)
  {
    return 142;
  }
  else if (commons.get_chosen_algorithm() == Algorithms::ECDSA_SECP521R1 || commons.get_chosen_algorithm() == Algorithms::ECDSA_BP512R1)
  {
    return 235;
  }
  else if (rsa_key_size == 2048)
  {
    return 459;
  }
  else
  {
    return 808;
  }
}

size_t WolfsslModule::get_public_key_der_size()
{
  if (commons.get_chosen_algorithm() == Algorithms::EDDSA_25519)
  {
    return 32;
  }
  else if (commons.get_chosen_algorithm() == Algorithms::EDDSA_448)
  {
    return 57;
  }
  else if (commons.get_chosen_algorithm() == Algorithms::ECDSA_SECP256R1 || commons.get_chosen_algorithm() == Algorithms::ECDSA_BP256R1)
  {
    return 65;
  }
  else if (commons.get_chosen_algorithm() == Algorithms::ECDSA_SECP521R1 || commons.get_chosen_algorithm() == Algorithms::ECDSA_BP512R1)
  {
    return 133;
  }
  else if (rsa_key_size == 2048)
  {
    return 294;
  }
  else
  {
    return 550;
  }
}
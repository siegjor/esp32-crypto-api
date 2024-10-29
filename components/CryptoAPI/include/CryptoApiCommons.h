#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_log.h"
// #include <string>

#ifndef CRYPTO_API_COMMONS
#define CRYPTO_API_COMMONS

enum Algorithms
{
  ECDSA_BP256R1,
  ECDSA_BP512R1,
  ECDSA_SECP256R1,
  ECDSA_SECP521R1,
  EDDSA_25519,
  EDDSA_448,
  RSA,
};

enum Hashes
{
  MY_SHA_256,
  MY_SHA_512,
  MY_SHA3_256,
  MY_SHAKE_256,
};

class CryptoApiCommons
{
public:
  CryptoApiCommons();
  Algorithms get_chosen_algorithm();
  void set_chosen_algorithm(Algorithms algorithm);
  Hashes get_chosen_hash();
  void set_chosen_hash(Hashes hash);
  void set_shake256_hash_length(size_t length);
  void save_pub_key(const char *pubkey_filename, char *public_key_pem, const int buffer_length);
  // std::string load_pub_key(const char *pubkey_filename);
  void print_hex(const uint8_t *data, size_t length);
  void log_success(const char *msg);
  void log_error(const char *msg);
  void print_elapsed_time(unsigned long start, unsigned long end, const char *label);
  void print_used_memory(unsigned long initial, unsigned long final, const char *label);
  size_t get_hash_length();

private:
  Algorithms chosen_algorithm;
  Hashes chosen_hash;
  size_t shake256_hash_length;
};

#endif
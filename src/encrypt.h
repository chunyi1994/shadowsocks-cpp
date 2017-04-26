#ifndef ENCRYPY_H
#define ENCRYPY_H
#include <string>
#include <algorithm>

#include <cryptopp/cryptlib.h>
#include <cryptopp/aes.h>

#include <cryptopp/modes.h> // CFB_Mode
#include <cryptopp/osrng.h>  //AutoSeededRandomPool
#include <cryptopp/hex.h>
#include <cryptopp/files.h>
#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
#include <cryptopp/md5.h>


//#include <cryptopp/aes.h>
//#include <cryptopp/blowfish.h>
//#include <cryptopp/cast.h>
//#include <cryptopp/chacha.h>
//#include <cryptopp/cryptlib.h>
//#include <cryptopp/des.h>
//#include <cryptopp/filters.h>
//#include <cryptopp/modes.h>
//#include <cryptopp/salsa.h>

namespace shadowsocks {

// the length of AES :  16（AES-128）、24（AES-192）、32（AES-256）Bytes
const int kAES_128_KEY_LEN = 16;
const int kAES_192_KEY_LEN = 24;
const int kAES_256_KEY_LEN = 32;

//MD5 128 bits == 16bytes
const int kMD5_128_BITS_LEN = 16;

//functions
std::string md5_string(const std::string & message);
std::vector<byte> md5_sum(const byte* bytes, std::size_t len);
std::vector<byte> md5_sum(const std::string& input);
std::vector<byte> evp_bytes_to_key(const std::string& password, std::size_t key_len);

//------------Encrypter-----------------

template<class T>
class Encrypter {
    typedef typename CryptoPP::CFB_Mode<T>::Encryption Encryption;

public:
    Encrypter(const std::string& password, std::size_t keylen, std::size_t ivlen);

    std::string encrypt(const std::string& plaintext);

    std::size_t iv_len() const       { return iv_len_; }

    std::string iv() const             { return iv_; }

private:
    std::string iv_;
    std::size_t key_len_;
    std::size_t iv_len_;
    Encryption encryption_;
};

template<class T>
inline Encrypter<T>::Encrypter(const std::string &password,
                               std::size_t keylen, std::size_t ivlen) :
    iv_(),
    key_len_(keylen),
    iv_len_(ivlen),
    encryption_()
{
    //init the iv_
    iv_.resize(ivlen);
    byte iv[ivlen];
    CryptoPP::AutoSeededRandomPool random_pool;
    random_pool.GenerateBlock(iv, ivlen);
    std::copy(iv, iv + ivlen, iv_.begin());
    std::vector<byte> key = evp_bytes_to_key(password, keylen);
    const byte * iv_bytes = reinterpret_cast<const byte *>(iv_.c_str());
    encryption_.SetKeyWithIV(key.data(), key.size(), iv_bytes, iv_.length());
}

template<class T>
std::string Encrypter<T>::encrypt(const std::string &plaintext) {
    std::string ciphertext;
    //don't need to worry about the memory leak
    //because StringSouce will delete the pointer (the third param)
    CryptoPP::StringSource(plaintext, true,
                           new CryptoPP::StreamTransformationFilter(
                               encryption_, new CryptoPP::StringSink(ciphertext)));
    return ciphertext;
}


//---------------------Decrypter--------------------

template<class T>
class Decrypter {
    typedef typename CryptoPP::CFB_Mode<T>::Decryption Decryption;
    Decrypter(const std::string& password,  std::size_t keylen, std::size_t ivlen);

    void set_key_with_iv(const std::string &iv);

    std::size_t iv_len() override { return iv_len_; }

    std::string decrypt(const std::string &ciphertext);

public:

private:
    std::size_t key_len_;
    std::size_t iv_len_;
    std::vector<byte> key_;
    Decryption decryption_;
};

template<class T>
inline Decrypter<T>::Decrypter(const std::string &password, std::size_t keylen, std::size_t ivlen) :
    key_len_(keylen),
    iv_len_(ivlen),
    key_(evp_bytes_to_key(password, keylen))
{}

template<class T>
inline std::string Decrypter<T>::decrypt(const std::string &ciphertext) {
    std::string plaintext;
    CryptoPP::StringSource(ciphertext, true, new CryptoPP::StreamTransformationFilter(
                               decryption_, new CryptoPP::StringSink(plaintext)));
    return plaintext;
}

template<class T>
inline void Decrypter<T>::set_key_with_iv(const std::string &iv) {
    decryption_.SetKeyWithIV(key_.data(),key_.size(),
                             reinterpret_cast<const byte *>(iv.c_str()), iv.length());
}


} //namespace

#endif // ENCRYPY_H

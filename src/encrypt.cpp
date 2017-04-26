#include "encrypt.h"

namespace shadowsocks {

/*
 * Description: to calculate the hash of the message, and return it(string).
 * Input:  message: need to calculate its hash value
 * Output: return the hash value(string) of the message.
 */
std::string md5_string(const std::string & message) {
    std::string digest;
    CryptoPP::Weak::MD5 md5;
    CryptoPP::StringSource(message, true,
                           new CryptoPP::HashFilter(md5, new CryptoPP::HexEncoder(
                                                        new CryptoPP::StringSink(digest))));
    return digest;
}

std::vector<byte> md5_sum(const byte* bytes, std::size_t len) {
    std::vector<byte> digests(CryptoPP::Weak::MD5::DIGESTSIZE);
    CryptoPP::Weak::MD5 hash;
    hash.CalculateDigest(digests.data(), bytes, len);
    return digests;
}

std::vector<byte> md5_sum(const std::string& input) {
    const byte* bytes =  reinterpret_cast<const byte*>(input.c_str());
    return md5_sum(bytes, input.size());
}

std::vector<byte> evp_bytes_to_key(const std::string& password, std::size_t key_len) {
    const std::size_t md5_len = kMD5_128_BITS_LEN;
    std::size_t count = (key_len - 1) / md5_len + 1;
    std::vector<byte> result (count * md5_len, '\0');
    std::vector<byte> md5_result = md5_sum(password);
    std::copy(md5_result.begin(), md5_result.end(), result.begin());
    // the vector md5_and_password is (MD5 STRING + PASSWORD STRING).
    //so length of the md5_and_password equals to md5_len +  password.size()
    std::vector<byte> md5_and_password (md5_len + password.size(), 0);
    std::size_t start = 0;
    for (std::size_t i = 1; i < count; ++i) {
        // Repeatedly call md5 until bytes generated is enough.
        // Each call to md5 uses data: prev md5 sum + password.
        start += md5_len;
        std::copy_n(result.begin() + start - md5_len, md5_len, md5_and_password.begin());
        std::copy(password.begin(), password.end(), md5_and_password.begin() + md5_len);
        md5_result = md5_sum(md5_and_password.data(), md5_and_password.size());
        std::copy(md5_result.begin(), md5_result.end(), result.begin() + start);
    }
    result.erase(result.begin() + key_len, result.end());
    return result;
}





} //namespace

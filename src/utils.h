#ifndef UTILS
#define UTILS
#include <boost/asio.hpp>
#include <functional>
#include <string>
namespace shadowsocks {

using boost::system::error_code;
using boost::asio::io_service;

template <class T>
inline std::string to_str(T value) {
    std::stringstream ss;
    ss << value;
    return ss.str();
}

}
#endif // UTILS


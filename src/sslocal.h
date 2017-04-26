#ifndef SSLOCAL_H
#define SSLOCAL_H

#include <boost/asio.hpp>
#include <memory>
#include <vector>
#include <functional>
#include <set>

#include "local_conn.h"
namespace shadowsocks {

using boost::asio::io_service;
using boost::system::error_code;

class SSLocal {
public:
    SSLocal(io_service& ioservice, std::size_t port);
    void start();
    void do_accept();
private:
    void start_timer();
private:
    io_service &ioservice_;
    boost::asio::ip::tcp::acceptor acceptor_;
    std::set<LocalConn::WeakPointer, std::owner_less<LocalConn::WeakPointer>> conns_;
    boost::asio::deadline_timer timer_;
};

}
#endif // SSLOCAL_H

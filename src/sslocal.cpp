#include "sslocal.h"

#include <iostream>
#include "local_conn.h"
#include "logger.h"

namespace shadowsocks {

SSLocal::SSLocal(io_service &ioservice, std::size_t port) :
    ioservice_(ioservice),
    acceptor_(ioservice, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))
  ,conns_(), timer_(ioservice)
{

}

void SSLocal::start() {
    do_accept();
    //start_timer();

}

void SSLocal::do_accept() {
    LocalConn::Pointer conn = LocalConn::create(ioservice_);
    acceptor_.async_accept(conn->socket(), [this, conn] (const error_code &ec) {
        if (!ec) {
            conns_.insert(conn);
            conn->handshake();
        } else {
            //handle error
            LOG_INFO<<"accept error:"<<ec.message();
        }
        do_accept();
    });
}

//这个函数用来调试程序，看看还有多少个链接没有被关闭。
void SSLocal::start_timer() {
    timer_.expires_from_now(boost::posix_time::seconds(3));
    timer_.async_wait([this](const boost::system::error_code& err) {
        if (err)  {
            //说明定时取消
            return;
        }
        int conn_count = 0;
        for (auto iter = conns_.begin(); iter != conns_.end();) {
            LocalConn::Pointer conn = iter->lock();
            if (conn) {
                ++iter;
                ++conn_count;
            } else {
                iter = conns_.erase(iter);
            }
        }
        LOG_INFO<<"现在还有"<<conn_count<<"个连接没有断开";
        start_timer();
    });
}

}//namespace

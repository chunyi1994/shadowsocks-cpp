#include "local_conn.h"

#include <assert.h>
#include <string>
#include "logger.h"
#include "utils.h"
namespace shadowsocks {

LocalConn::LocalConn(io_service &ioservice) :
    socket_(ioservice),
    buffer_(kDEFAULT_BUFFER_SIZE),
    timer_(ioservice),
    status_(eRUNNING)
{
}

void LocalConn::handshake() {
    async_read(0, kFIRST_HANDSHAKE_MSG_LEN,
               [this] (error_code err, std::size_t bytes) {
        handle_handshake1(err, bytes);
    });
}

void LocalConn::run() {
    handshake();
}

LocalConn::~LocalConn() {
    //std::cout<<"~LocalConn"<<std::endl;
}

boost::asio::ip::tcp::socket &LocalConn::socket() {
    return socket_;
}

std::vector<char> &LocalConn::buffer() {
    return buffer_;
}

LocalConn::Pointer LocalConn::create(boost::asio::io_service &ioservice) {
    return std::make_shared<LocalConn>(ioservice);
}

void LocalConn::recv(LocalConn::RecvCallback cb) {
    Pointer self = shared_from_this();
    socket_.async_read_some(boost::asio::buffer(buffer_),
                            [this,self, cb] (error_code err, std::size_t bytes) {
        if (!cb) {
            return;
        }
        cb(std::move(err), bytes, self);
    });
}

void LocalConn::on_close(LocalConn::CloseCallback cb) {
    close_callback_ = std::move(cb);
}

void LocalConn::close() {
    socket_.close();
    handle_close();
}

boost::asio::io_service &LocalConn::ioservice() {
    return socket_.get_io_service();
}

void LocalConn::handle_handshake1(error_code err, std::size_t bytes) {
    status_ = eHANDSHAKE;
    const std::string kFIRST_MSG = { 0x05, 0x01, 0x00 };
    if (bytes < kFIRST_HANDSHAKE_MSG_LEN) {
        LOG_DEBUG<<"handshake1 error1";
        return;
    }
    std::string recv(buffer_.data(), bytes);
    if (recv != kFIRST_MSG) {
        LOG_DEBUG<<"handshake1 error2";
        return;
    }
    const std::string kOK = { 0x05, 0x00 };
    async_write(kOK.data(), kOK.size(), [this] (error_code err, std::size_t) {
        if (err) {
            LOG_DEBUG<<"send ok error: "<<err.message();
            close();
            return;
        }
        async_read(0, kHOST_LEN + 1, [this] (error_code err, std::size_t bytes) {
            parse_addr(err, bytes);
        });
    });
}

void LocalConn::parse_addr(error_code err, std::size_t bytes) {
    if (bytes < kADDR_TYPE + 1) {
        LOG_DEBUG<<"parse_addr error1";
        if (err) {
            LOG_DEBUG<<"parse_addr error :"<<err.message();
        }
        close();
        return;
    }
    const int kTYPE_IPV4 = 0;
    const int kTYPE_URL  = 3;
    const int kTYPE_IPV6 = 4;
    int type = buffer_[kADDR_TYPE];
    switch (type) {
    case kTYPE_IPV4:
        get_addr_ipv4(bytes);
        break;

    case kTYPE_URL:
        get_addr_domain(bytes);
        break;

    case kTYPE_IPV6:
        get_addr_ipv6(bytes);
        break;
    }
}

void LocalConn::get_addr_ipv4(std::size_t bytes) {
    LOG_TRACE<<"get_ipv4";
}

void LocalConn::get_addr_ipv6(std::size_t bytes) {
    LOG_TRACE<<"get_ipv6";
}

unsigned short get_port(const char* buf) {
    assert(sizeof(unsigned short) == 2);
    union Port {
        unsigned short port;
        char s[2];
    };
    Port uport;
    uport.s[0] = buf[0];
    uport.s[1] = buf[1];
    using namespace boost::asio::detail::socket_ops;
    return network_to_host_short(uport.port);
}

void LocalConn::get_addr_domain(std::size_t bytes) {
    std::size_t url_len = buffer_[kHOST_LEN];
    if (url_len == 0) {
        LOG_DEBUG<<"bad request: domain length must >= 1.";
        return;
    }
    const std::size_t need_read = url_len + kPORT_LEN;
    async_read(kHOST_LEN + 1,
               need_read,
               [this, need_read, url_len] (error_code, std::size_t bytes) {
        if (bytes < need_read) {
            LOG_DEBUG<<"get domain error";
            close();
            return;
        }
        std::string host(buffer_.data() + kHOST, url_len);
        std::size_t pos_port = kHOST + url_len;
        unsigned short port = get_port(buffer_.data() + pos_port);
        connect_and_iocopy(std::move(host), port);
    });
}

void LocalConn::connect(
        std::string host,
        std::size_t port,
        LocalConn::ConnectCallback cb) {
    LocalConn::Pointer conn = std::make_shared<LocalConn>(ioservice());
    conn->set_status(eCONNECT);
    LocalConn::WeakPointer weak_conn = conn;
    conn->timer_.expires_from_now(boost::posix_time::seconds(5));
    conn->timer_.async_wait([this, weak_conn](const boost::system::error_code& err) {
        if (err)  return;
        LocalConn::Pointer conn = weak_conn.lock();
        if (!conn) {
            return;
        }
        conn->set_status(eDEAD);
        conn->socket().close();
    });
    boost::asio::ip::tcp::resolver resolver(ioservice());
    boost::asio::ip::tcp::resolver::query query(host, to_str(port));
    typedef boost::asio::ip::tcp::resolver::iterator Iterator;
    Iterator endpoint_iterator = resolver.resolve(query);
    boost::asio::async_connect(conn->socket(),
                               endpoint_iterator, [conn, cb] (error_code err, Iterator) {
        conn->timer_.cancel();
        if (err) {
            cb(err, nullptr);
            return;
        }
        cb(err, conn);
    });
}

void LocalConn::connect_and_iocopy(std::string host, std::size_t port) {
    Pointer self = shared_from_this();
    this->connect(host, port, [this, self] (error_code err, LocalConn::Pointer peer) {
        if (err) {
            LOG_DEBUG<<"connect error:" <<err.message();
            return;
        }
        std::vector<char> ok{0x05, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        self->send(ok.data(), ok.size(), [peer] (error_code err, std::size_t, Pointer self) {
            if (err) {
                LOG_DEBUG<<"send ok error:"<<err.message();
                return;
            }
            LocalConn::iocopy(self, peer);
            LocalConn::iocopy(peer, self);
        });
    });
}

void LocalConn::iocopy(LocalConn::Pointer src, LocalConn::Pointer dest) {
    src->set_status(eIOCOPY);
    src->recv([dest] (error_code err, std::size_t bytes, LocalConn::Pointer src) {
        if (bytes == 0) {
            dest->socket().cancel(err);
            return;
        }
        dest->send(src->buffer().data(),  bytes,
                   [src, err] (error_code send_err, std::size_t, Pointer dest) {
            if (err || send_err) {
                return;
            }
            LocalConn::iocopy(src, dest);
        });
    });
}

void LocalConn::send(const char *msg, std::size_t len, SendCallback cb) {
    async_write(msg, len , [cb, this] (error_code err, std::size_t bytes) {
        if (!cb) {
            return;
        }
        cb(err, bytes, shared_from_this());
    });
}


void LocalConn::async_read(std::size_t startpos,
                           std::size_t read_bytes,
                           LocalConn::ReadCallback cb) {
    assert(read_bytes < buffer_.size());
    LocalConn::Pointer self = shared_from_this();
    boost::asio::async_read(
                socket_,
                boost::asio::buffer(buffer_.data() + startpos, read_bytes),
                [this, cb, self](error_code code, std::size_t bytes)  mutable {
        cb(code, bytes);
    });
}

void LocalConn::handle_close()
{

}

void LocalConn::async_write(const char *msg, std::size_t len, LocalConn::WriteCallback cb) {
    LocalConn::Pointer self = shared_from_this();
    boost::asio::async_write(
                socket_,
                boost::asio::buffer(msg, len),
                [this, cb,self] (error_code err, std::size_t bytes) {
        cb(err, bytes);
    });
}


} //namespace

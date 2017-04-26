#ifndef LOCALCONN_H
#define LOCALCONN_H
#include <boost/asio.hpp>
#include <memory>
namespace shadowsocks {
using boost::system::error_code;
using boost::asio::io_service;

class LocalConn : public std::enable_shared_from_this<LocalConn> {


public:
    typedef std::shared_ptr<LocalConn> Pointer;
    typedef std::weak_ptr<LocalConn> WeakPointer;
    typedef std::function<void(error_code, std::size_t bytes, LocalConn::Pointer) > RecvCallback;
    typedef std::function<void(error_code, std::size_t bytes, LocalConn::Pointer) > SendCallback;
    typedef std::function<void(LocalConn::Pointer)> CloseCallback;

    enum Status {
        eRUNNING,
        eDEAD,
        eHANDSHAKE,
        eCONNECT,
        eIOCOPY
    };

private:
    typedef std::function<void(error_code, std::size_t bytes)> ReadCallback;
    typedef std::function<void(error_code, std::size_t bytes)> WriteCallback;
    typedef std::function<void(error_code, LocalConn::Pointer)> ConnectCallback;

public:
    void handshake();
    void run();
    LocalConn(io_service& ioservice);
    ~LocalConn();
    boost::asio::ip::tcp::socket& socket();
    std::vector<char>& buffer();
    static Pointer create(io_service& ioservice);
    static void iocopy(Pointer conn1, Pointer conn2);

    void send(const char* msg, std::size_t len, SendCallback cb);

    void send_with_encryption(const char* msg, std::size_t len,  WriteCallback cb);

    void recv(RecvCallback cb);

    void on_close(CloseCallback cb);

    void close();

    Status status() const { return status_; }
    void set_status(Status s) { status_ = s; }

private:
    io_service& ioservice();
    void handle_handshake1(error_code, std::size_t bytes);
    void parse_addr(error_code, std::size_t bytes);
    void get_addr_ipv4(std::size_t bytes);
    void get_addr_ipv6(std::size_t bytes);
    void get_addr_domain(std::size_t bytes);
    void connect(std::string host, std::size_t port, ConnectCallback);
    void connect_and_iocopy(std::string host, std::size_t port);


    template<class Container>
    void async_write(const Container& msg, WriteCallback cb);
    void async_write(const char* msg, std::size_t len, WriteCallback cb);
    void async_read(std::size_t startpos, std::size_t read_bytes, ReadCallback);

    void handle_close();

private:
    static const int kDEFAULT_BUFFER_SIZE = 1024 * 3;
    static const int kFIRST_HANDSHAKE_MSG_LEN = 3;
    static const int kADDR_TYPE = 3;
    static const int kHOST_LEN = kADDR_TYPE + 1;
    static const int kHOST = kHOST_LEN + 1;
    static const int kPORT_LEN = 2;

private:
    boost::asio::ip::tcp::socket socket_;
    std::vector<char> buffer_;
    CloseCallback close_callback_;
    boost::asio::deadline_timer timer_;
    Status status_;
};

template <class Container>
inline void LocalConn::async_write(const Container& msg, WriteCallback cb) {
    async_write(msg.data(), msg.size(), cb);
}

}
#endif // LOCALCONN_H

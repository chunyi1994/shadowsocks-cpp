#include <iostream>
#include "sslocal.h"
#include "encrypt.h"
using namespace std;
using namespace shadowsocks;

void run_server() {
    boost::asio::io_service ioservice;
    SSLocal local_server(ioservice, 9999);
    local_server.start();
    ioservice.run();
}
#include <stdio.h>
int main()
{
    std::string result1 = md5_string("md5_string");
    std::vector<byte> result2 = md5_sum("md5_string");

    cout<<result1<<"\n";
    //cout<<result2<<"\n";
    cout<<result2.size()<<"\n";
    for (std::size_t i = 0; i < result2.size(); ++i) {
        unsigned char c = result2[i];
        printf("%x", c);
    }
    return 0;
}


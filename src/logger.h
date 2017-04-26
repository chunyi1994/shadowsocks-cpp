#ifndef LOGGER_H
#define LOGGER_H
#include <iostream>
#include <sstream>
namespace shadowsocks {
class SimpleLogger {
public:
    SimpleLogger() :ss_() { }

    ~SimpleLogger() {
        std::cout<<ss_.str()<<std::endl;
    }

    std::stringstream& stream() {
        return ss_;
    }
private:
      std::stringstream ss_;
};

#define LOG_INFO SimpleLogger().stream()<<"[INFO]"
#define LOG_TRACE SimpleLogger().stream()<<"[TRACE]"<<__FILE__<<"->"<< __func__<<"->"<< __LINE__<<":"
#define LOG_DEBUG SimpleLogger().stream()<<"[DEBUG]"<<__FILE__<<"->"<< __func__<<"->"<< __LINE__<<":"

}
#endif // LOGGER_H


#ifndef LOGGER_H_
#define LOGGER_H_

#include <fstream>

class Logger{
 public:
    Logger();
    void log(unsigned long long, char*, size_t);
    void finish();
    void init();
    void recoverTree(std::fstream &f);

 private:
    size_t num, pos;
    std::fstream file;
};

#endif

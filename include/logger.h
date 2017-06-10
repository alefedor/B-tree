#ifndef LOGGER_H_
#define LOGGER_H_

#include <fstream>

class Logger{
 public:
    Logger();
    void log(unsigned long long, char*, size_t, bool is_value);
    void finish();
    void init();
    void recoverTree(std::fstream &f, std::fstream &f_vals);

 private:
    size_t num, pos;
    std::fstream file;
};

#endif

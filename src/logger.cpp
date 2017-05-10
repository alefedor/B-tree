#include <exception>
#include "logger.h"

Logger::Logger(){
    file.open("btree.log", std::ios::in | std::ios::out | std::ios::binary);
    if (!file.good()){
        file.close();
        throw std::runtime_error("Error in file for logger");
    }
}

void Logger::init(){
    pos = 1;
    num = 0;
    file.seekp(0, std::ios_base::beg);
    char c = 0;
    file.write(&c, 1);
    file.flush();
}

void Logger::log(unsigned long long offset, char* bin, size_t sz){
    file.seekp(pos, std::ios_base::beg);
    file.write((char*)&offset, sizeof(unsigned long long));
    pos += sizeof(unsigned long long);
    file.write((char*)&sz, sizeof(size_t));
    pos += sizeof(size_t);
    file.write(bin, sz);
    pos += sz;

    num++;
    file.seekp(0, std::ios_base::beg);
    file.write((char*)&num, 1);
    file.flush();

    if (!file.good())
        throw std::runtime_error("Error in file for logger");
}

void Logger::finish(){
    pos = 0;
    file.seekp(0, std::ios_base::beg);
    char c = 0;
    file.write(&c, 1);
    file.flush();
    if (!file.good())
        throw std::runtime_error("Error in file for logger");

}

void Logger::recoverTree(std::fstream &f){
    file.seekg(0, std::ios_base::end);
    if (file.tellg() == 0)
        return;
    file.seekg(0, std::ios_base::beg);
    char cnt;
    file.read(&cnt, 1);
    while (cnt != 0){
        cnt--;
        unsigned long long offset;
        size_t sz;
        file.read((char*)&offset, sizeof(unsigned long long));
        file.read((char*)&sz, sizeof(size_t));
        char buf[sz];
        file.read(buf, sz);
        f.seekp(offset, std::ios_base::beg);
        f.write(buf, sz);
    }
    if (!file.good())
        throw std::runtime_error("Error in file for logger");
}


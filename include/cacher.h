#ifndef CACHER_H_
#define CACHER_H_

#include <map>

class Cacher{
 public:
    Cacher(size_t sz);
    ~Cacher();
    void update(unsigned long long offset, char* bin);
    char* get(unsigned long long offset);

 private:
    Cacher(const Cacher &c);
    void operator =(const Cacher &c);

    std::map<unsigned long long, char*> store;
    size_t sz;
    const size_t max_size = (1<<25); //32 MB
};

#endif

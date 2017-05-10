#include <cstring>
#include "cacher.h"

using namespace std;

Cacher::Cacher(size_t sz):sz(sz){}

Cacher::~Cacher(){
    while (!store.empty()){
        delete [] store.begin() -> second;
        store.erase(store.begin());
    }
}

void Cacher::update(unsigned long long offset, char* bin){
    char *buf = new char[sz];
    memcpy(buf, bin, sz);
    if (store.count(offset) != 0){
        delete []store[offset];
    }

    store[offset] = buf;
    if (store.size() * sz > max_size){
        std::map<unsigned long long, char*>::iterator it = store.upper_bound(offset);
        if (it == store.end())
            it = store.find(store.rbegin() -> first);
        delete [] it -> second;
        store.erase(it);
    }
}


char* Cacher::get(unsigned long long offset){
    if (store.count(offset) != 0)
        return store[offset];
    else
        return NULL;
}

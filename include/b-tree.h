#ifndef BTREE_H_
#define BTREE_H_

#include <fstream>
#include <vector>
#include <algorithm>
#include <cstring>
#include <utility>
#include <iostream>
#include <exception>
#include "cacher.h"
#include "logger.h"

template <typename Key, typename Value, unsigned int min_deg> //min_deg-1 ... 2min_deg-2 keys in node
class Btree{
 public:
    static_assert(min_deg >= 2, "Should be at least two children");

    Btree();
    ~Btree(){}

    void addElem(const Key &k, const Value &v);
    void delElem(const Key &k);
    bool findElem(const Key &k, Value *v);
    void getElems(const Key &l, const Key &r, std::vector<std::pair<Key, Value> > &res);

 private:
    class Node{
     public:
        Node(std::fstream &f, unsigned long long offset, Cacher &cache);
        Node(){};

        void writeNode(std::fstream &f, Logger &logger, Cacher &cache);
        void delNode(std::fstream &f, Logger &logger, Cacher &cache, unsigned long long &nxt_space);
        void swap(Node &n);
        void insertInLeaf(const Key &k, const Value &v);
        void eraseInLeaf(size_t pos);
        bool isLeaf();
        void replaceKey(size_t pos, const Key &k, const Value &v);
        void insert(const Key &k, const Value &v, unsigned long long son_offset);

        const static size_t size = (2 * min_deg - 2) * sizeof(Value) + (2 * min_deg - 2) * sizeof(Key) + (2 * min_deg - 1) * sizeof(unsigned long long);
        bool changed;
        unsigned long long offset;
        std::vector<Key> keys; //keys in order as in file
        std::vector<Value> vals;
        std::vector<unsigned long long> refs; //references to next nodes (offsets)

     private:
        char* getBinary();
        char old[size];
    };

    void add(unsigned long long offset, const Key &k, const Value &v, Node *par);
    void del(unsigned long long offset, const Key &k, Node *par, size_t pos);
    bool find(unsigned long long offset, const Key &k, Value *v);
    void get(unsigned long long offset, const Key &l, const Key &r, std::vector<std::pair<Key, Value> > &res);
    std::pair<Key, Value> delNext(unsigned long long offset, Node *par, size_t pos, const Key &k);
    void fix(Node &n, Node *par, size_t pos);

    unsigned long long getNextSpace(std::fstream &f);
    void changeOffset(unsigned long long offset, std::fstream &f);

    Btree(const Btree &b);
    void operator= (const Btree &b);

    unsigned long long nxt_space;
    const size_t root = sizeof(unsigned long long);
    Logger logger;
    std::fstream file;//, file_vals;
    Cacher cache;
};

template <typename Key, typename Value, unsigned int t>
Btree<Key, Value, t>::Btree():cache(Node::size){
    file.open("btree.main", std::ios::in | std::ios::out | std::ios::binary); //remove
    //file_vals.open("btree.vals", std::ios::in | std::ios::out | std::ios::binary);
    logger.recoverTree(file);
    nxt_space = 0;
    if (getNextSpace(file) < Node::size + root){
        file.seekp(0, std::ios_base::beg);
        char buf[Node::size + root];
        memset(buf, 0, Node::size + root);
        file.write(buf, Node::size + root);
    }else{
        file.seekg(0, std::ios_base::beg);
        file.read((char*)&nxt_space, sizeof(unsigned long long));
    }
    if (!file.good()){
        file.close();
        throw std::runtime_error("Error on opening file");
    }
}

template <typename Key, typename Value, unsigned int t>
void Btree<Key, Value, t>::changeOffset(unsigned long long offset, std::fstream &f){
    f.seekg(offset, std::ios_base::beg);
    unsigned long long pos;
    f.read((char*)&pos, sizeof(unsigned long long));

    char buf[sizeof(unsigned long long)];
    memcpy(buf, &offset, sizeof(unsigned long long));
    logger.log(0, buf, sizeof(unsigned long long));
    f.seekp(0, std::ios_base::beg);
    f.write((char*)&pos, sizeof(unsigned long long));
    nxt_space = pos;
}


template <typename Key, typename Value, unsigned int t>
unsigned long long Btree<Key, Value, t>::getNextSpace(std::fstream &f){
    if (nxt_space != 0){
        unsigned long long offset = nxt_space;
        changeOffset(nxt_space, f);
        return offset;
    }

    f.seekg(0, std::ios_base::end);
    return f.tellg();
}

template <typename Key, typename Value, unsigned int t>
bool Btree<Key, Value, t>::findElem(const Key &k, Value *v){
    logger.init();
    bool res = find(root, k, v);
    if (!file.good())
        throw std::runtime_error("Error with file while findElem");
    logger.finish();
    return res;
}

template <typename Key, typename Value, unsigned int t>
bool Btree<Key, Value, t>::find(unsigned long long offset, const Key &k, Value *v){
    Node n(file, offset, cache);
    typename std::vector<Key>::iterator it = lower_bound(n.keys.begin(), n.keys.end(), k);
    size_t pos = it - n.keys.begin();
    if (it != n.keys.end() && *it == k){
        *v = n.vals[pos];
        return true;
    }else
    if (!n.isLeaf()){
        return find(n.refs[pos], k, v);
    }
    return false;
}

template <typename Key, typename Value, unsigned int t>
void Btree<Key, Value, t>::getElems(const Key &l, const Key &r, std::vector<std::pair<Key, Value> > &res){
    logger.init();
    get(root, l, r, res);
    if (!file.good())
        throw std::runtime_error("Error with file while getElems");
    logger.finish();
}

template <typename Key, typename Value, unsigned int t>
void Btree<Key, Value, t>::get(unsigned long long offset, const Key &l, const Key &r, std::vector<std::pair<Key, Value> > &res){
    Node n(file, offset, cache);
    typename std::vector<Key>::iterator lit = lower_bound(n.keys.begin(), n.keys.end(), l);
    typename std::vector<Key>::iterator rit = upper_bound(n.keys.begin(), n.keys.end(), r);
    size_t lpos = lit - n.keys.begin(), rpos = rit - n.keys.begin();
    if (!n.isLeaf())
    for (size_t i = lpos; i <= rpos; i++)
        get(n.refs[i], l, r, res);
    for (size_t i = lpos; i < rpos; i++)
        res.emplace_back(n.keys[i], n.vals[i]);
}

template <typename Key, typename Value, unsigned int min_deg>
void Btree<Key, Value, min_deg>::addElem(const Key &k, const Value &v){
    logger.init();
    add(root, k, v, NULL); //for root parent = NULL
    if (!file.good())
        throw std::runtime_error("Error with file while addElem");
    logger.finish();
}

template <typename Key, typename Value, unsigned int min_deg>
void Btree<Key, Value, min_deg>::add(unsigned long long offset, const Key &k, const Value &v, Node *par){
    Node n(file, offset, cache);
    //std::cout << n.offset << " " << n.keys.size() << " " << n.refs.size() << std::endl;
    typename std::vector<Key>::iterator it = lower_bound(n.keys.begin(), n.keys.end(), k);
    size_t pos = it - n.keys.begin();
    if (it != n.keys.end() && *it == k){
        //n.update(pos, v);
        n.changed = true;
        n.vals[pos] = v;
    }else{
        if (n.isLeaf()) //leaf
            n.insertInLeaf(k, v);
        else{ //not leaf
            add(n.refs[pos], k, v, &n);
        }
    }
    if (n.keys.size() == 2 * min_deg - 1){
        Node new_node;
        size_t mid = min_deg;
        n.changed = new_node.changed = true;
        new_node.offset = getNextSpace(file);
        new_node.vals.insert(new_node.vals.begin(), n.vals.begin() + mid, n.vals.end());
        new_node.keys.insert(new_node.keys.begin(), n.keys.begin() + mid, n.keys.end());
        if (!n.isLeaf())
            new_node.refs.insert(new_node.refs.begin(), n.refs.begin() + mid, n.refs.end());

        new_node.writeNode(file, logger, cache);

        if (par == NULL){
            Node new_root;
            new_root.offset = getNextSpace(file);
            new_root.changed = true;
            new_root.swap(n);
            new_root.refs.emplace_back(n.offset);
            new_root.insert(n.keys[mid - 1], n.vals[mid - 1], new_node.offset);
            new_root.writeNode(file, logger, cache);
        }else{
            par -> insert(n.keys[mid - 1], n.vals[mid - 1], new_node.offset);
        }

        n.vals.resize(mid - 1);
        n.keys.resize(mid - 1);
        if (!n.isLeaf())
            n.refs.resize(mid);
    }
    n.writeNode(file, logger, cache);
}


template <typename Key, typename Value, unsigned int min_deg>
void Btree<Key, Value, min_deg>::fix(Node &n, Node *par, size_t pos){
    if (par == NULL){ //root
        if (n.keys.size() == 0 && n.refs.size() != 0){
            Node new_root(file, n.refs[0], cache);
            n.changed = new_root.changed = true;
            std::swap(n.keys, new_root.keys);
            std::swap(n.vals, new_root.vals);
            std::swap(n.refs, new_root.refs);
            new_root.delNode(file, logger, cache, nxt_space);
        } // else all is fine

        return;
    }

    if (pos != 0){ //there is left brother
        Node left(file, par -> refs[pos - 1], cache);

        if (left.keys.size() > min_deg - 1){
            n.changed = left.changed = par -> changed = true;
            n.keys.insert(n.keys.begin(), par -> keys[pos - 1]);
            n.vals.insert(n.vals.begin(), par -> vals[pos - 1]);
            if (!n.isLeaf())
                n.refs.insert(n.refs.begin(), left.refs.back());
            par -> keys[pos - 1] = left.keys.back();
            par -> vals[pos - 1] = left.vals.back();
            left.keys.pop_back();
            left.vals.pop_back();
            if (!n.isLeaf())
                left.refs.pop_back();

            left.writeNode(file, logger, cache);
        }else{
            n.changed = left.changed = par -> changed = true;
            n.keys.insert(n.keys.begin(), par -> keys[pos - 1]);
            n.vals.insert(n.vals.begin(), par -> vals[pos - 1]);
            n.keys.insert(n.keys.begin(), left.keys.begin(), left.keys.end());
            n.vals.insert(n.vals.begin(), left.vals.begin(), left.vals.end());
            n.refs.insert(n.refs.begin(), left.refs.begin(), left.refs.end());
            par -> keys.erase(par -> keys.begin() + pos - 1);
            par -> vals.erase(par -> vals.begin() + pos - 1);
            par -> refs.erase(par -> refs.begin() + pos - 1);

            left.delNode(file, logger, cache, nxt_space);
        }
        return;
    }

    if (pos != par -> refs.size() - 1){ // there is right brother
        Node right(file, par -> refs[pos + 1], cache);

        if (right.keys.size() > min_deg - 1){
            n.changed = right.changed = par -> changed = true;
            n.keys.insert(n.keys.end(), par -> keys[pos]);
            n.vals.insert(n.vals.end(), par -> vals[pos]);
            if (!n.isLeaf())
                n.refs.insert(n.refs.end(), right.refs.front());
            par -> keys[pos] = right.keys.front();
            par -> vals[pos] = right.vals.front();
            right.keys.erase(right.keys.begin());
            right.vals.erase(right.vals.begin());
            if (!n.isLeaf())
                right.refs.erase(right.refs.begin());

            right.writeNode(file, logger, cache);
        }else{

            n.changed = right.changed = par -> changed = true;
            n.keys.insert(n.keys.end(), par -> keys[pos]);
            n.vals.insert(n.vals.end(), par -> vals[pos]);
            n.keys.insert(n.keys.end(), right.keys.begin(), right.keys.end());
            n.vals.insert(n.vals.end(), right.vals.begin(), right.vals.end());
            n.refs.insert(n.refs.end(), right.refs.begin(), right.refs.end());
            par -> keys.erase(par -> keys.begin() + pos);
            par -> vals.erase(par -> vals.begin() + pos);
            par -> refs.erase(par -> refs.begin() + pos + 1);

            right.delNode(file, logger, cache, nxt_space);
        }
        return;
    }
}

template <typename Key, typename Value, unsigned int min_deg>
void Btree<Key, Value, min_deg>::delElem(const Key &k){
    logger.init();
    del(root, k, NULL, 0);
    if (!file.good())
        throw std::runtime_error("Error with file while delElem");
    logger.finish();
}

template <typename Key, typename Value, unsigned int min_deg>
void Btree<Key, Value, min_deg>::del(unsigned long long offset, const Key &k, Node *par, size_t from){
    Node n(file, offset, cache);
    typename std::vector<Key>::iterator it = lower_bound(n.keys.begin(), n.keys.end(), k);
    size_t pos = it - n.keys.begin();
    if (n.isLeaf()){
        if (it == n.keys.end() || *it != k){
            return;
        }
        n.eraseInLeaf(pos);
    }else{
        if (it != n.keys.end() && *it == k){
            std::pair<Key, Value> next_key = delNext(n.refs[pos + 1], &n, pos + 1, k);
            it = std::find(n.keys.begin(), n.keys.end(), k);
            if (it != n.keys.end())
                n.replaceKey(it - n.keys.begin(), next_key.first, next_key.second);
            //n.replace_key(pos, next_key.first, next_key.second);
        }else
            del(n.refs[pos], k, &n, pos);
    }
    if (n.keys.size() < min_deg - 1)
        fix(n, par, from);
    n.writeNode(file, logger, cache);
}


template <typename Key, typename Value, unsigned int min_deg>
std::pair<Key, Value> Btree<Key, Value, min_deg>::delNext(unsigned long long offset, Node *par, size_t from, const Key &k){
    Node n(file, offset, cache);
    std::pair<Key, Value> res;
    if (n.isLeaf()){
        res = std::make_pair(n.keys[0], n.vals[0]);
        n.eraseInLeaf(0);
    }else{
        res = delNext(n.refs[0], &n, 0, k);
    }

    if (n.keys.size() < min_deg - 1)
        fix(n, par, from);

    typename std::vector<Key>::iterator it = std::find(n.keys.begin(), n.keys.end(), k);
    if (it != n.keys.end())
        n.replaceKey(it - n.keys.begin(), res.first, res.second);
    n.writeNode(file, logger, cache);
    return res;
}

template <typename Key, typename Value, unsigned int min_deg>
void Btree<Key, Value, min_deg>::Node::eraseInLeaf(size_t pos){
    changed = true;
    keys.erase(keys.begin() + pos);
    vals.erase(vals.begin() + pos); // do
}

template <typename Key, typename Value, unsigned int min_deg>
void Btree<Key, Value, min_deg>::Node::replaceKey(size_t pos, const Key &k, const Value &v){
    changed = true;
    keys[pos] = k;
    vals[pos] = v; // to do
}

template <typename Key, typename Value, unsigned int min_deg>
bool Btree<Key, Value, min_deg>::Node::isLeaf(){
    return (refs.size() == 0);
}

template <typename Key, typename Value, unsigned int min_deg>
char* Btree<Key, Value, min_deg>::Node::getBinary(){
    char* buf = new char[size];
    size_t pos = 0;

    if (refs.size() == 0){
        unsigned long long one = 1;
        for (size_t i = 0; i <= keys.size(); i++){
            memcpy(buf + pos, &one, sizeof(unsigned long long));
            pos += sizeof(unsigned long long);
        }
    }else{
        for (size_t i = 0; i < refs.size(); i++){
            memcpy(buf + pos, &refs[i], sizeof(unsigned long long));
            pos += sizeof(unsigned long long);
        }
    }

    if ((2 * min_deg - 1) * sizeof(unsigned long long) - pos != 0)
        memset(buf + pos, 0, (2 * min_deg - 1) * sizeof(unsigned long long) - pos);
    pos = (2 * min_deg - 1) * sizeof(unsigned long long);

    for (size_t i = 0; i < (2 * min_deg - 2); i++)
        if (i < keys.size()){
            memcpy(buf + pos, &keys[i], sizeof(Key));
            pos += sizeof(Key);
        }else{
            memset(buf + pos, 0, sizeof(Key));
            pos += sizeof(Key);
        }

    for (size_t i = 0; i < (2 * min_deg - 2); i++)
        if (i < vals.size()){
            memcpy(buf + pos, &vals[i], sizeof(Value));
            pos += sizeof(Value);
        }else{
            memset(buf + pos, 0, sizeof(Value));
            pos += sizeof(Value);
        }
    return buf;
}

template <typename Key, typename Value, unsigned int min_deg>
void Btree<Key, Value, min_deg>::Node::writeNode(std::fstream &f, Logger &logger, Cacher &cache){
    if (!changed)
        return;
    char *bin = getBinary();
    cache.update(offset, bin);
    logger.log(offset, old, size);
    f.seekp(offset, std::ios_base::beg);
    f.write(bin, size);
    delete [] bin;
}

template <typename Key, typename Value, unsigned int min_deg>
void Btree<Key, Value, min_deg>::Node::swap(Node &n){
    changed = n.changed = true;
    std::swap(offset, n.offset);
}

template <typename Key, typename Value, unsigned int min_deg>
void Btree<Key, Value, min_deg>::Node::insertInLeaf(const Key &k, const Value &v){
    changed = true;
    typename std::vector<Key>::iterator it = lower_bound(keys.begin(), keys.end(), k);
    vals.insert(vals.begin() + (it - keys.begin()), v);
    keys.insert(it, k);
}

template <typename Key, typename Value, unsigned int min_deg>
void Btree<Key, Value, min_deg>::Node::insert(const Key &k, const Value &v, unsigned long long son_offset){
    changed = true;
    typename std::vector<Key>::iterator it = lower_bound(keys.begin(), keys.end(), k);
    vals.insert(vals.begin() + (it - keys.begin()), v);
    refs.insert(refs.begin() + (it - keys.begin() + 1), son_offset);
    keys.insert(it, k);
}


template <typename Key, typename Value, unsigned int min_deg>
void Btree<Key, Value, min_deg>::Node::delNode(std::fstream &f, Logger &logger, Cacher &cache, unsigned long long &nxt_space){
    char buf[size];
    memset(buf, 0, size);
    memcpy(buf, &nxt_space, sizeof(unsigned long long));
    cache.update(offset, buf);
    logger.log(offset, old, size);
    f.seekp(offset, std::ios_base::beg);
    f.write(buf, size);

    char tmp[sizeof(unsigned long long)];
    memcpy(tmp, &nxt_space, sizeof(unsigned long long));
    logger.log(0, tmp, sizeof(unsigned long long));
    memcpy(tmp, &offset, sizeof(unsigned long long));
    f.seekp(0, std::ios_base::beg);
    f.write(tmp, sizeof(unsigned long long));
    nxt_space = offset;
}

template <typename Key, typename Value, unsigned int min_deg>
Btree<Key, Value, min_deg>::Node::Node(std::fstream &f, unsigned long long ps, Cacher &cache):changed(false), offset(ps){
    char *res = cache.get(offset);
    if (!res){
        f.seekg(offset, std::ios_base::beg);
        f.read(old, size);
    }else{
        memcpy(old, res, size);
    }
    size_t mx = 2 * min_deg - 1;
    size_t pos = 0;
    unsigned long long r;
    for (size_t i = 0; i < mx; i++){
        memcpy(&r, old + pos, sizeof(unsigned long long));
        pos += sizeof(unsigned long long);
        if (r != 0)
            refs.emplace_back(r);
    }
    Key k;
    for (size_t i = 1; i < mx; i++){
        memcpy(&k, old + pos, sizeof(Key));
        pos += sizeof(Key);
        if (i < refs.size())
            keys.emplace_back(k);
    }
    Value v;
    for (size_t i = 1; i < mx; i++){
        memcpy(&v, old + pos, sizeof(Value));
        pos += sizeof(Value);
        if (i < refs.size())
            vals.emplace_back(v);
    }
    if (!refs.empty() && refs[0] == 1)
        refs.clear();
}

//template <typename Key, typename Value, unsigned int min_deg>
//Btree<Key, Value, min_deg>::Node::Node(){}
#endif

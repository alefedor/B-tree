#include <cstddef>
#include <cstdlib>
#include <iostream>

#include <cassert>
#include <map>

#include "b-tree.h"

using namespace std;

void clear_tree(){
    fstream file("btree.main", std::fstream::out | ios_base::trunc);
    fstream l("btree.log", std::fstream::out | ios_base::trunc);
    fstream file_vals("btree.vals", std::fstream::out | ios_base::trunc);
    file_vals.close();
    l.close();
    file.close();
}

void test_add(){
    clear_tree();
    Btree<pair<int, int>, pair<long long, long long>, 25> b;
    for (size_t i = 0; i < 1000; i++)
        b.addElem(make_pair(rand(), rand()), make_pair(5, 4)); // testing that this doesn't break down
}

void test_find(){
    clear_tree();
    Btree<int, int, 35> b;
    map<int, int> mp;
    int a;
    int val;
    for (size_t i = 0; i < 500; i++){
        a = rand() % 750;
        mp[a] = val = i;
        b.addElem(a, val);
    }

    bool bad = false, res;
    int vv;
    int *v = &vv;
    for (size_t i = 0; i < 750; i++){
        res = b.findElem(i, v);
        if (res != mp.count(i) || (res && mp[i] != *v))
            bad = true;

    }
    if (bad)
        cerr << "Find test failed" << endl;
    while (!mp.empty()){
        b.delElem(mp.begin() -> first);
        mp.erase(mp.begin());
    }
}

void test_get(){
    clear_tree();
    Btree<int, long long, 35> b;
    map<int, long long> mp;
    int a;
    long long val;
    for (size_t i = 0; i < 500; i++){
        a = rand() % 750;
        mp[a] = val = rand();
        b.addElem(a, val);
    }

    bool bad = false;
    vector<pair<int, long long> > v;
    for (size_t i = 0; i < 500; i++){
        a = i + rand() % 100;
        v.clear();
        b.getElems(i, i + a, v);
        map<int, long long>::iterator it = mp.lower_bound(i);
        size_t num = 0;
        while (it != mp.end() && it -> first <= (int)i + a){
            num++;
            it++;
        }
        if (v.size() != num)
            bad = true;
        for (pair<int, long long> &p:v)
            if (!mp.count(p.first) || mp[p.first] != p.second)
                bad = true;
    }
    if (bad)
        cerr << "Get in range test failed" << endl;

}

void test_del(){
    clear_tree();
    Btree<int, long long, 35> b;
    int arr[100];
    for (size_t i = 0; i < 100; i++)
        arr[i] = i;
    random_shuffle(arr, arr + 100);
    for (size_t i = 0; i < 100; i++)
        b.addElem(arr[i], 5);
    random_shuffle(arr, arr + 100);
    for (size_t i = 0; i < 100; i++)
        b.delElem(arr[i]);
    long long vv;
    long long *v = &vv;
    bool bad = false;
    for (size_t i = 0; i < 100; i++)
        if (b.findElem(i, v))
            bad = true;
    if (bad)
        cerr << "Del test failed" << endl;
}

void test_reuse(){
    Btree<int, long long, 35> b;
    for (size_t i = 0; i < 100; i++)
        b.addElem(i, 4);

    for (size_t i = 0; i < 50; i++)
        b.delElem(i);

    for (size_t i = 100; i < 300; i++)
        b.addElem(i, 7);

    bool bad = false;
    long long vv;
    long long *v = &vv;
    for (size_t i = 0; i < 300; i++)
        if ((i < 50 && b.findElem(i, v)) || (i >= 50 && !b.findElem(i, v)))
            bad = true;
    if (bad)
        cerr << "Reuse test failed" << endl;
}

void test_all(){
    test_find();
    test_get();
    test_add();
    test_del();
    test_reuse();
}

int main(){
    test_all();
}

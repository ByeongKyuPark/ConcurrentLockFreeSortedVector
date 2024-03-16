#pragma once

#include <iostream>       // std::cout
#include <atomic>         // std::atomic
#include <thread>         // std::thread
#include <vector>         // std::vector
#include <deque>          // std::deque
#include <mutex>          // std::mutex
#include "MemoryBank.h"
#include "GarbageRemover.h"

struct Pair {
    std::vector<int>* pointer;
    long              refCount;
};//__attribute__((aligned(16), packed));
// for some compilers alignment needed to stop std::atomic<Pair>::load to segfault

class LFSV {
    std::atomic<Pair> mPtrData;
    MemoryBank& mRefMemoryBank;
    GarbageRemover& mRefRemover;

public:

    LFSV(MemoryBank& bank, GarbageRemover& remover);
    ~LFSV();

    void Insert(const int& v);
    int operator[] (int pos);
};
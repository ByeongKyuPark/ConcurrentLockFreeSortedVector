#pragma once

#include <iostream>       // std::cout
#include <atomic>         // std::atomic
#include <thread>         // std::thread
#include <vector>         // std::vector
#include <deque>          // std::deque
#include <mutex>          // std::mutex
#include "MemoryBank.h"
#include "GarbageRemover.h"

// 'DeferredVectorNodeHandle' is a POD struct used with std::atomic for compare-and-swap (CAS) operations.
// In C++11 onwards, std::atomic can be applied to trivially copyable and standard-layout types,
// enabling atomic operations on the whole memory block represented by the struct.
// This approach allows atomic CAS without the need for an explicit 'operator==' !, 
// as it directly compares the memory content, ensuring thread-safe modifications.
struct DeferredVectorNodeHandle {
    std::vector<int>* pointer;
    long              refCount;
};//__attribute__((aligned(16), packed));
// for some compilers alignment needed to stop std::atomic<Pair>::load to segfault


class ConcurrentSortedVector {
    std::atomic<DeferredVectorNodeHandle> mPtrData;
    MemoryBank& mRefMemoryBank;
    GarbageRemover& mRefRemover;

public:
    ConcurrentSortedVector(MemoryBank& bank, GarbageRemover& remover);
    ConcurrentSortedVector(MemoryBank& bank, GarbageRemover& remover, std::vector<int> initialData);
    ~ConcurrentSortedVector();

    void Insert(const int& v);
    int operator[] (int pos);
};
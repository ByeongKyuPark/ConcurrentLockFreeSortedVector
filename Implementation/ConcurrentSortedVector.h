#include <iostream>
#include <atomic>
#include <thread>
#include <vector>
#include <deque>
#include <mutex>
#include "MemoryBank.h"
#include "GarbageRemover.h"

template<typename T>
class ConcurrentSortedVector {
    struct DeferredVectorNodeHandle {
        std::vector<T>* pointer;
        long refCount;
        DeferredVectorNodeHandle(std::vector<T>* ptr = nullptr, long count = 0) : pointer(ptr), refCount(count) {}
    };

    std::atomic<DeferredVectorNodeHandle> mPtrData;
    MemoryBank<T>& mRefMemoryBank;
    GarbageRemover<T>& mRefRemover;

    // thread-local cache for allocated vectors
    static thread_local std::vector<T>* cachedVector;

public:
    ConcurrentSortedVector(MemoryBank<T>& bank, GarbageRemover<T>& remover);
    ConcurrentSortedVector(MemoryBank<T>& bank, GarbageRemover<T>& remover, std::vector<T> initialData);
    ~ConcurrentSortedVector();

    void Insert(const T& v);
    T operator[](int pos);
};

#include "ConcurrentSortedVector.inl"
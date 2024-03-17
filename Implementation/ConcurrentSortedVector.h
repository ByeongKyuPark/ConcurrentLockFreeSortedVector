#pragma once

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
    };

    std::atomic<DeferredVectorNodeHandle> mPtrData;
    MemoryBank<T>& mRefMemoryBank;
    GarbageRemover<T>& mRefRemover;

public:
    ConcurrentSortedVector(MemoryBank<T>& bank, GarbageRemover<T>& remover);
    ConcurrentSortedVector(MemoryBank<T>& bank, GarbageRemover<T>& remover, std::vector<T> initialData);
    ~ConcurrentSortedVector();

    void Insert(const T& v);
    T operator[](int pos);
};

template<typename T>
ConcurrentSortedVector<T>::ConcurrentSortedVector(MemoryBank<T>& bank, GarbageRemover<T>& remover)
    : mPtrData({ bank.Acquire(), 1 }), mRefMemoryBank(bank), mRefRemover(remover)
{}

template<typename T>
ConcurrentSortedVector<T>::ConcurrentSortedVector(MemoryBank<T>& bank, GarbageRemover<T>& remover, std::vector<T> initialData)
    : mRefMemoryBank(bank), mRefRemover(remover) {

    Quicksort(initialData.data(), 0, initialData.size(), 2);

    auto* sortedVector = mRefMemoryBank.Acquire();
    *sortedVector = std::move(initialData);

    mPtrData.store(DeferredVectorNodeHandle{ sortedVector, 1 });
}

template<typename T>
ConcurrentSortedVector<T>::~ConcurrentSortedVector() {
    auto data = mPtrData.load().pointer;
    mRefRemover.ScheduleForDeletion(data);
}

template<typename T>
void ConcurrentSortedVector<T>::Insert(const T& v) {
    DeferredVectorNodeHandle oldData = mPtrData.load();
    while (true) {
        DeferredVectorNodeHandle newData{ mRefMemoryBank.Acquire(), 1 };
        *newData.pointer = *oldData.pointer;
        auto it = std::lower_bound(newData.pointer->begin(), newData.pointer->end(), v);
        newData.pointer->insert(it, v);

        if (mPtrData.compare_exchange_weak(oldData, newData)) {
            mRefRemover.ScheduleForDeletion(oldData.pointer);
            break;
        }
        else {
            mRefMemoryBank.Release(newData.pointer);
        }
    }
}

template<typename T>
T ConcurrentSortedVector<T>::operator[](int pos) {
    DeferredVectorNodeHandle pdata_new, pdata_old;
    do {
        pdata_old = mPtrData.load();
        pdata_new = pdata_old;
        ++pdata_new.refCount;
    } while (!(this->mPtrData).compare_exchange_weak(pdata_old, pdata_new));

    T ret_val = (*pdata_new.pointer)[pos];

    do {
        pdata_old = mPtrData.load();
        pdata_new = pdata_old;
        --pdata_new.refCount;
    } while (!(this->mPtrData).compare_exchange_weak(pdata_old, pdata_new));

    return ret_val;
}

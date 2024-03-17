#include "ConcurrentSortedVector.h"
#include "Quicksort.h"
ConcurrentSortedVector::ConcurrentSortedVector(MemoryBank& bank, GarbageRemover& remover) : mPtrData({ bank.Acquire(), 1 }), mRefMemoryBank(bank), mRefRemover(remover)
{}

// Constructor for LFSV with bulk data initialization
ConcurrentSortedVector::ConcurrentSortedVector(MemoryBank& bank, GarbageRemover& remover, std::vector<int> initialData)
    : mRefMemoryBank(bank), mRefRemover(remover) {

    // sort the initialData using Concurrent Quick Sort
    Quicksort(initialData.data(), 0, initialData.size(), 2);

    auto* sortedVector = mRefMemoryBank.Acquire();
    *sortedVector = std::move(initialData);

    mPtrData.store(DeferredVectorNodeHandle{ sortedVector, 1 });
}

ConcurrentSortedVector::~ConcurrentSortedVector() {
    auto data = mPtrData.load().pointer;
    mRefRemover.ScheduleForDeletion(data); // schedule the final vector for deletion
}

void ConcurrentSortedVector::Insert(int const& v) {
    DeferredVectorNodeHandle oldData = mPtrData.load();
    while (true) {
        DeferredVectorNodeHandle newData{ mRefMemoryBank.Acquire(), 1 };
        *newData.pointer = *oldData.pointer; // copy data
        // insert v into newData.pointer in sorted order
        auto it = std::lower_bound(newData.pointer->begin(), newData.pointer->end(), v);
        newData.pointer->insert(it, v); // insert value in the correct position

        if (mPtrData.compare_exchange_weak(oldData, newData)) {
            mRefRemover.ScheduleForDeletion(oldData.pointer); // old data scheduled for delayed deletion
            break;
        }
        else {
            mRefMemoryBank.Release(newData.pointer); // returns the meory if CAS fails
        }
    }
}

int ConcurrentSortedVector::operator[] (int pos) { // not a const method
    DeferredVectorNodeHandle pdata_new, pdata_old;
    do { // before read - increment counter, use CAS
        pdata_old = mPtrData.load();
        pdata_new = pdata_old;
        ++pdata_new.refCount;
    } while (!(this->mPtrData).compare_exchange_weak(pdata_old, pdata_new));

    int ret_val = (*pdata_new.pointer)[pos];

    do { // before return - decrement counter, use CAS
        pdata_old = mPtrData.load();
        pdata_new = pdata_old;
        --pdata_new.refCount;
    } while (!(this->mPtrData).compare_exchange_weak(pdata_old, pdata_new));

    return ret_val;
}
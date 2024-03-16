#include "lfsv.h"

LFSV::LFSV(MemoryBank& bank, GarbageRemover& remover) : mPtrData({ bank.Acquire(), 1 }), mRefMemoryBank(bank), mRefRemover(remover)
{}

LFSV::~LFSV() {
    auto data = mPtrData.load().pointer;
    mRefRemover.ScheduleForDeletion(data); // schedule the final vector for deletion
}

void LFSV::Insert(int const& v) {
    Pair oldData = mPtrData.load();
    while (true) {
        Pair newData{ mRefMemoryBank.Acquire(), 1 };
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

int LFSV::operator[] (int pos) { // not a const method
    Pair pdata_new, pdata_old;
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
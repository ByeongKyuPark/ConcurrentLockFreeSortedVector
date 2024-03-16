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
    long              ref_count;
}; // __attribute__((aligned(16),packed));
// for some compilers alignment needed to stop std::atomic<Pair>::load to segfault

class LFSV {
    std::atomic<Pair> mDataPtr;
    MemoryBank& mRefMemoryBank;
    GarbageRemover& mRefRemover;

public:

    LFSV(MemoryBank& bank, GarbageRemover& remover) : mRefMemoryBank(bank), mRefRemover(remover), mDataPtr({ bank.Acquire(), 1 }) {}

    ~LFSV() {
        auto data = mDataPtr.load().pointer;
        mRefRemover.ScheduleForDeletion(data); // Schedule the final vector for deletion
    }

    void Insert(int const& v) {
        Pair oldData = mDataPtr.load();
        while (true) {
            Pair newData{ mRefMemoryBank.Acquire(), 1 };
            *newData.pointer = *oldData.pointer; // Copy data
            // Insert v into newData.pointer in sorted order
            auto it = std::lower_bound(newData.pointer->begin(), newData.pointer->end(), v);
            newData.pointer->insert(it, v); // insert value in the correct position

            if (mDataPtr.compare_exchange_weak(oldData, newData)) {
                mRefRemover.ScheduleForDeletion(oldData.pointer); // Old data scheduled for delayed deletion
                break;
            }
            else {
                mRefMemoryBank.Release(newData.pointer); // Release if CAS fails
            }
        }
    }


    int operator[] (int pos) { // not a const method anymore
        Pair pdata_new, pdata_old;
        do { // before read - increment counter, use CAS
            pdata_old = mDataPtr.load();
            pdata_new = pdata_old;
            ++pdata_new.ref_count;
        } while (!(this->mDataPtr).compare_exchange_weak(pdata_old, pdata_new));

        int ret_val = (*pdata_new.pointer)[pos];

        do { // before return - decrement counter, use CAS
            pdata_old = mDataPtr.load();
            pdata_new = pdata_old;
            --pdata_new.ref_count;
        } while (!(this->mDataPtr).compare_exchange_weak(pdata_old, pdata_new));

        return ret_val;
    }
};

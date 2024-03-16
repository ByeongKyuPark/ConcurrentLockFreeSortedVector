#pragma once
#include "MemoryBank.h"
#include <chrono>
#include <queue>
#include <condition_variable>
#include <atomic>
#include <thread>

class GarbageRemover {
    std::queue<std::pair<std::vector<int>*, std::chrono::steady_clock::time_point>> mDelayedQueue;
    std::mutex mMutex;
    std::condition_variable mCond;
    std::atomic<bool> mStop{ false };
    std::thread mWorker;
    MemoryBank& mRefBank;
    static constexpr std::chrono::milliseconds DELAY_DURATION{ 100 }; // delay before recycling

    void Process() {
        while (!mStop) {
            std::unique_lock<std::mutex> lock(mMutex);
            mCond.wait(lock, [this] { return !mDelayedQueue.empty() || mStop; });

            auto now = std::chrono::steady_clock::now();
            while (!mDelayedQueue.empty() && now >= mDelayedQueue.front().second) {
                mRefBank.Release(mDelayedQueue.front().first);
                mDelayedQueue.pop();
                if (!mDelayedQueue.empty()) {
                    now = std::chrono::steady_clock::now(); // update time for the next loop iteration
                }
            }
        }

        // cleanup remaining items on stop
        while (!mDelayedQueue.empty()) {
            mRefBank.Release(mDelayedQueue.front().first);
            mDelayedQueue.pop();
        }
    }

public:
    GarbageRemover(MemoryBank& bank) : mRefBank(bank), mWorker(&GarbageRemover::Process, this) {}

    ~GarbageRemover() {
        mStop = true;
        mCond.notify_one();
        mWorker.join();
    }

    void ScheduleForDeletion(std::vector<int>* vec, const std::chrono::milliseconds& delay = DELAY_DURATION) {
        std::lock_guard<std::mutex> lock(mMutex);
        mDelayedQueue.push({ vec, std::chrono::steady_clock::now() + delay });
        mCond.notify_one();
    }
};

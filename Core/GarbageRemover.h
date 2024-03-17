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

    void Process();

public:
    GarbageRemover(MemoryBank& bank) : mWorker(&GarbageRemover::Process, this), mRefBank(bank) {}
    ~GarbageRemover();

    void ScheduleForDeletion(std::vector<int>* vec, const std::chrono::milliseconds& delay = std::chrono::milliseconds(100));
};

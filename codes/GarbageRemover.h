#pragma once

#include <vector>
#include <mutex>
#include <chrono>
#include <queue>
#include <condition_variable>
#include <atomic>
#include <thread>
#include "MemoryBank.h"

template<typename T>
class GarbageRemover {
    std::queue<std::pair<std::vector<T>*, std::chrono::steady_clock::time_point>> mDelayedQueue;
    std::mutex mMutex;
    std::condition_variable mCond;
    std::atomic<bool> mStop{ false };
    std::thread mWorker;
    MemoryBank<T>& mRefBank;

    void Process();

public:
    GarbageRemover(MemoryBank<T>& bank) : mWorker(&GarbageRemover<T>::Process, this), mRefBank(bank) {}
    ~GarbageRemover();

    void ScheduleForDeletion(std::vector<T>* vec, const std::chrono::milliseconds& delay = std::chrono::milliseconds(100));
};
#include "GarbageRemover.inl"
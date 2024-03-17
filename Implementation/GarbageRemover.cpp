#include "GarbageRemover.h"

void GarbageRemover::Process() {
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

GarbageRemover::~GarbageRemover() {
    mStop = true;
    mCond.notify_one();
    mWorker.join();
}

void GarbageRemover::ScheduleForDeletion(std::vector<int>* vec, const std::chrono::milliseconds& delay) {
    std::lock_guard<std::mutex> lock(mMutex);
    mDelayedQueue.push({ vec, std::chrono::steady_clock::now() + delay });
    mCond.notify_one();
}

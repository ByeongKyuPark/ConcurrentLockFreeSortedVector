#include "MemoryBank.h"

MemoryBank::MemoryBank(size_t size) {
    for (size_t i = 0; i < size; ++i) {
        mPool.push_back(new std::vector<int>());
    }
}

MemoryBank::~MemoryBank() {
    for (auto* vec : mPool) {
        delete vec;
    }
}

std::vector<int>* MemoryBank::Acquire() {
    std::lock_guard<std::mutex> lock(mMutex);
    if (mPool.empty()) return new std::vector<int>(); // fallback to dynamic allocation if pool is exhausted
    auto* vec = mPool.back();
    mPool.pop_back();
    return vec;
}

void MemoryBank::Release(std::vector<int>* vec) {
    std::lock_guard<std::mutex> lock(mMutex);
    if (vec != nullptr) {
        vec->clear(); // reset the vector for reuse only if it's not nullptr
        mPool.push_back(vec);
    }
}

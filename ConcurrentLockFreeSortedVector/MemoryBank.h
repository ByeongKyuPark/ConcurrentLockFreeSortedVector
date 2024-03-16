#pragma once
#include <vector>
#include <mutex>

class MemoryBank {
    std::vector<std::vector<int>*> pool;
    std::mutex mutex;

public:
    MemoryBank(size_t size) {
        for (size_t i = 0; i < size; ++i) {
            pool.push_back(new std::vector<int>());
        }
    }

    ~MemoryBank() {
        for (auto* vec : pool) {
            delete vec;
        }
    }

    std::vector<int>* Acquire() {
        std::lock_guard<std::mutex> lock(mutex);
        if (pool.empty()) return new std::vector<int>(); // fallback to dynamic allocation if pool is exhausted
        auto* vec = pool.back();
        pool.pop_back();
        return vec;
    }

    void Release(std::vector<int>* vec) {
        std::lock_guard<std::mutex> lock(mutex);
        vec->clear(); // reset the vector for reuse
        pool.push_back(vec);
    }
};

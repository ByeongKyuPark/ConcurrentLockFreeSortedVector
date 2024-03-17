#pragma once

#include <vector>
#include <mutex>

template<typename T>
class MemoryBank {
    std::vector<std::vector<T>*> mPool;
    std::mutex mMutex;

public:
    explicit MemoryBank(size_t size);
    ~MemoryBank()=default;

    std::vector<T>* Acquire();
    void Release(std::vector<T>* vec);
};
#include "MemoryBank.inl"
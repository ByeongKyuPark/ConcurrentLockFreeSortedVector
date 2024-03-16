#pragma once
#include <vector>
#include <mutex>

class MemoryBank {
    std::vector<std::vector<int>*> mPool;
    std::mutex mMutex;

public:
    MemoryBank(size_t size);
    ~MemoryBank();

    std::vector<int>* Acquire();
    void Release(std::vector<int>* vec);
};

template<typename T>
MemoryBank<T>::MemoryBank(size_t size) {
    for (size_t i = 0; i < size; ++i) {
        mPool.push_back(new std::vector<T>());
    }
}

template<typename T>
std::vector<T>* MemoryBank<T>::Acquire() {
    std::lock_guard<std::mutex> lock(mMutex);
    if (mPool.empty()) return new std::vector<T>(); // fallback to dynamic allocation if pool is exhausted
    auto* vec = mPool.back();
    mPool.pop_back();
    return vec;
}

template<typename T>
void MemoryBank<T>::Release(std::vector<T>* vec) {
    std::lock_guard<std::mutex> lock(mMutex);
    if (vec != nullptr) {
        vec->clear(); // reset the vector for reuse only if it's not nullptr
        mPool.push_back(vec);
    }
}

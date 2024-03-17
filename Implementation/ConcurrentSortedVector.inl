
// Initialize the thread-local cache
template<typename T>
thread_local std::vector<T>* ConcurrentSortedVector<T>::cachedVector = nullptr;

template<typename T>
ConcurrentSortedVector<T>::ConcurrentSortedVector(MemoryBank<T>& bank, GarbageRemover<T>& remover)
    : mPtrData({ bank.Acquire(), 1 }), mRefMemoryBank(bank), mRefRemover(remover) {}

template<typename T>
ConcurrentSortedVector<T>::ConcurrentSortedVector(MemoryBank<T>& bank, GarbageRemover<T>& remover, std::vector<T> initialData)
    : mRefMemoryBank(bank), mRefRemover(remover) {

    auto* sortedVector = cachedVector ? cachedVector : mRefMemoryBank.Acquire();
    *sortedVector = std::move(initialData);
    mPtrData.store(DeferredVectorNodeHandle{ sortedVector, 1 });
    // clear the thread-local cache to avoid reusing it after initialization.
    cachedVector = nullptr;
}

template<typename T>
ConcurrentSortedVector<T>::~ConcurrentSortedVector() {
    auto data = mPtrData.load().pointer;
    mRefRemover.ScheduleForDeletion(data);
    // ensure the thread-local cache is cleared upon destruction to prevent memory leaks.
    if (cachedVector) {
        mRefMemoryBank.Release(cachedVector);
        cachedVector = nullptr;
    }
}

template<typename T>
void ConcurrentSortedVector<T>::Insert(const T& v) {
    while (true) {
        DeferredVectorNodeHandle oldData = mPtrData.load();
        std::vector<T>* newDataVector = cachedVector ? cachedVector : mRefMemoryBank.Acquire();

        // reuse or reset the thread-local cache as needed
        if (cachedVector) {
            newDataVector->clear(); // clear before reuse
            cachedVector = nullptr;
        }

        *newDataVector = *oldData.pointer;
        auto it = std::lower_bound(newDataVector->begin(), newDataVector->end(), v);
        newDataVector->insert(it, v);

        DeferredVectorNodeHandle newData(newDataVector, 1);
        if (mPtrData.compare_exchange_weak(oldData, newData)) {
            mRefRemover.ScheduleForDeletion(oldData.pointer);
            break;
        }
        else {
            cachedVector = newDataVector;// cache for future use
        }
    }
}


template<typename T>
T ConcurrentSortedVector<T>::operator[](int pos) {
    DeferredVectorNodeHandle pdata_new, pdata_old;
    do {
        pdata_old = mPtrData.load();
        pdata_new = pdata_old;
        ++pdata_new.refCount;
    } while (!(this->mPtrData).compare_exchange_weak(pdata_old, pdata_new));

    T ret_val = (*pdata_new.pointer)[pos];

    do {
        pdata_old = mPtrData.load();
        pdata_new = pdata_old;
        --pdata_new.refCount;
    } while (!(this->mPtrData).compare_exchange_weak(pdata_old, pdata_new));

    return ret_val;
}

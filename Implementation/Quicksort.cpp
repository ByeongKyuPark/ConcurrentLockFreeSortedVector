#include <iostream>
#include <vector>
#include <thread>//std::thread
#include <mutex>//std::mtex
#include <semaphore.h>//std::semaphore
#include <atomic>
#include <condition_variable>
#include "SortSmallArrays.h"

template< typename T>
unsigned Partition(T* a, unsigned begin, unsigned end) {
    unsigned i = begin, last = end - 1;
    T pivot = a[last];

    for (unsigned j = begin; j < last; ++j) {
        if (a[j] < pivot) {
            std::swap(a[j], a[i]);
            ++i;
        }
    }
    std::swap(a[i], a[last]);
    return i;
}

template< typename T>
unsigned PartitionNew(T* a, unsigned begin, unsigned end) {
    if (end - begin > 8) return partition_old(a, begin, end);

    unsigned i = begin, last = end - 1, step = (end - begin) / 4;

    T* pivots[5] = { a + begin, a + begin + step, a + begin + 2 * step, a + begin + 3 * step, a + last };
    quicksort_base_5_pointers(pivots);

    std::swap(a[last], a[begin + 2 * step]);
    T pivot = a[last];

    for (unsigned j = begin; j < last; ++j) {
        if (a[j] < pivot /*|| a[j]==pivot*/) {
            std::swap(a[j], a[i]);
            ++i;
        }
    }
    std::swap(a[i], a[last]);
    return i;
}

/* recursive */
template< typename T>
void QuicksortRec(T* a, unsigned begin, unsigned end)
{
    if (end - begin < 6) {
        switch (end - begin) {
        case 5: quicksort_base_5(a + begin); break;
        case 4: quicksort_base_4(a + begin); break;
        case 3: quicksort_base_3(a + begin); break;
        case 2: quicksort_base_2(a + begin); break;
        }
        return;
    }

    unsigned q = Partition(a, begin, end);

    QuicksortRec(a, begin, q);//[begin,q)
    QuicksortRec(a, q, end);  //[q,end)
}
/* iterative */
#define STACK 
#define xVECTOR
#define xPRIORITY_QUEUE 

#include <utility> // std::pair

template <typename T>
using Triple = typename std::pair< T*, std::pair<unsigned, unsigned>>;

template< typename T>
struct CompareTriples {
    bool operator() (Triple<T> const& op1, Triple<T> const& op2) const {
        return op1.second.first > op2.second.first;
    }
};

#ifdef STACK
#include <stack>
template< typename T>
using Container = std::stack< Triple<T>>;
#define PUSH push
#define TOP  top
#define POP  pop
#endif

#ifdef VECTOR
#include <vector>
template< typename T>
using Container = std::vector< triple<T>>;
#define PUSH push_back
#define TOP  back
#define POP  pop_back
#endif

#ifdef PRIORITY_QUEUE
#include <queue>
template< typename T>
using Container = std::priority_queue< triple<T>, std::vector<triple<T>>, compare_triples<T> >;
#define PUSH push
#define TOP  top
#define POP  pop
#endif

template <typename T>
class ThreadSafeContainer {
public:

    ThreadSafeContainer(unsigned N_, int num_threads_ = 1) : mContainer{}, mMtx{}, mCond{}, countSortedElems{}, N{ N_ }, numThreads{ num_threads_ } {
    }
    void push(const Triple<T>& range) {
        std::unique_lock<std::mutex> lock(mMtx);
        mContainer.PUSH(range);
        mCond.notify_one();
    }

    Triple<T> pop() {
        std::unique_lock<std::mutex> lock(mMtx);
        mCond.wait(lock, [&] {
            return !mContainer.empty();
            });
        Triple<T> range = mContainer.TOP();
        mContainer.POP();
        return range;
    }

    bool empty() const {
        std::unique_lock<std::mutex> lock(mMtx);
        return mContainer.empty();
    }

    void wakeup_all() {
        for (int i{ 1 }; i < numThreads; ++i) {
            push(std::make_pair(nullptr, std::make_pair(0, 0)));
        }
        mCond.notify_all();
    }

private:
    Container<T> mContainer;
    mutable std::mutex mMtx;
    std::condition_variable mCond;

    std::atomic<unsigned> countSortedElems;
    unsigned N;
    int numThreads;

    template <typename U>
    friend void QuicksortIterativeAux(ThreadSafeContainer<U>& bag);
};

template< typename T>
void QuicksortIterativeAux(ThreadSafeContainer<T>& bag);

template< typename T>
void QuicksortIterative(T* a, unsigned begin, unsigned end)
{
    //empty for now 
}

template<typename T>
void Quicksort(T* a, unsigned begin, unsigned end, int num_threads)
{
    //The optimal number of threads to sort an array of 200 Ratios with a delay was 
    //64, which took 1.55689 seconds.

    //The second best one was 26 threads, which resulted in an elapsed time of 1.58789s.
    //On the other hand, using a single thread resulted in a maximum elapsed time of 4.18361s.

    std::vector<std::thread> threads;
    threads.reserve(num_threads);

    ThreadSafeContainer<T> bag(end - begin, num_threads);
    bag.push(std::make_pair(a, std::make_pair(begin, end)));

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&bag]() {
            QuicksortIterativeAux(bag);
            });
    }
    for (auto& t : threads) {
        t.join();
    }
}

template< typename T>
void QuicksortIterativeAux(ThreadSafeContainer<T>& bag)
{
    bool done{ false };
    while (!done) {
        Triple<T> r = bag.pop();

        T* a = r.first;
        unsigned b = r.second.first;
        unsigned e = r.second.second;

        if (b == e) {
            if (bag.countSortedElems == bag.N) {
                return;
            }
            continue;
        }

        if (e - b < 6) {
            switch (e - b) {
            case 5: quicksort_base_5(a + b); bag.countSortedElems += 5; break;
            case 4: quicksort_base_4(a + b); bag.countSortedElems += 4; break;
            case 3: quicksort_base_3(a + b); bag.countSortedElems += 3; break;
            case 2: quicksort_base_2(a + b); bag.countSortedElems += 2; break;
            case 1: bag.countSortedElems++; break;
            }
            if (bag.countSortedElems == bag.N) {
                done = true;
                bag.wakeup_all();
                return;
            }
            continue;
        }

        unsigned q = Partition(a, b, e);
        bag.countSortedElems++;

        if (bag.countSortedElems == bag.N) {
            done = true;
            bag.wakeup_all();
            return;
        }

        bag.push(std::make_pair(a, std::make_pair(b, q)));
        bag.push(std::make_pair(a, std::make_pair(q + 1, e)));
    }
}

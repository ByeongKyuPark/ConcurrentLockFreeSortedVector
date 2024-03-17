#include "pch.h"
#include <gtest/gtest.h>
#include <vector>
#include <thread>
#include "MemoryBank.h"
#include "LFSV.h"

TEST(MemoryBankTest, AcquireReturnsValidPointer) {
    MemoryBank bank(1);
    auto* vec1 = bank.Acquire(); 
    ASSERT_NE(vec1, nullptr);
    bank.Release(vec1); // clean up

    auto* vec2 = bank.Acquire();
    ASSERT_EQ(vec1, vec2);
    bank.Release(vec2); // clean up
}

TEST(MemoryBankTest, ReleaseNullptrDoesNotCrash) {
    MemoryBank bank(10);
    std::vector<int>* vec = nullptr;
    EXPECT_NO_THROW(bank.Release(vec)); // ensure releasing nullptr is safe
}

TEST(MemoryBankTest, ReleaseHandlesNullptrWithoutAffectingPool) {
    constexpr int BANK_SIZE = 5;
    MemoryBank bank(BANK_SIZE);

    // initially acquire all vectors to deplete the pool
    std::vector<std::vector<int>*> acquiredVectors;
    for (int i = 0; i < BANK_SIZE; ++i) {
        acquiredVectors.push_back(bank.Acquire());
    }

    // releasing all acquired vectors back to the pool
    for (auto* vec : acquiredVectors) {
        bank.Release(vec);
    }
    acquiredVectors.clear(); // clear the list after releasing

    // attempt to release a nullptr, expecting no side effects on the pool
    bank.Release(nullptr);

    // reacquire vectors to check if pool integrity is maintained
    for (int i = 0; i < BANK_SIZE; ++i) {
        auto* vec = bank.Acquire();

        // verify acquired memory is not nullptr
        ASSERT_NE(vec, nullptr);

        // keep track of reacquired vectors for later cleanup
        acquiredVectors.push_back(vec);
    }

    // verify the last reacquired vector is the same as the first one initially acquired  (cuz it's a stack!)
    ASSERT_EQ(acquiredVectors.back(), acquiredVectors[BANK_SIZE-1]);

    // clean up: release any reacquired vectors back to the pool
    for (auto* vec : acquiredVectors) {
        bank.Release(vec);
    }
}


TEST(MemoryBankTest, AcquireExhaustsPool) {
    MemoryBank bank(1); // pool size of 1 for test
    auto* firstVec = bank.Acquire();
    ASSERT_NE(nullptr, firstVec);

    auto* secondVec = bank.Acquire(); // should trigger fallback allocation
    ASSERT_NE(nullptr, secondVec);
    ASSERT_NE(firstVec, secondVec); // ensure a new vector was allocated

    bank.Release(firstVec);
    bank.Release(secondVec);
}

TEST(MemoryBankTest, ConcurrentAcquireAndRelease) {
    MemoryBank bank(10); //pool for concurrency test
    std::vector<std::vector<int>*> acquiredVectors(20, nullptr);
    std::vector<std::thread> threads;

    // concurrently acquire and release vectors
    for (int i = 0; i < 20; ++i) {
        threads.emplace_back([&bank, &acquiredVectors, i]() {
            auto* vec = bank.Acquire();
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Simulate work
        acquiredVectors[i] = vec;
        bank.Release(vec);
            });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // check for unique vectors to ensure no overlap in acquisition
    std::sort(acquiredVectors.begin(), acquiredVectors.end());
    auto last = std::unique(acquiredVectors.begin(), acquiredVectors.end());
    ASSERT_EQ(last, acquiredVectors.end()); // fail if duplicates are found
}

TEST(LFSVTest, InsertAndRetrieve) {
    MemoryBank bank(10);
    GarbageRemover remover(bank);
    LFSV lfsv(bank, remover);

    lfsv.Insert(5);
    ASSERT_EQ(lfsv[0], 5);

    lfsv.Insert(3);
    // since new elements are inserted in a sorted manner, 
    // check if the elements are in the expected positions after insertion.
    ASSERT_EQ(lfsv[0], 3); // the smaller element should be first.
    ASSERT_EQ(lfsv[1], 5); // the larger element should be second.
}

TEST(LFSVTest, HandleLargeNumberOfInserts) {
    MemoryBank bank(1000);
    GarbageRemover remover(bank);
    LFSV lfsv(bank, remover);

    int N = 50000; // number of inserts
    for (int i = N; i > 0; --i) {
        lfsv.Insert(i);
    }

    // verify that elements are sorted
    for (int i = 0; i < N; ++i) {
        ASSERT_EQ(i + 1, lfsv[i]);
    }
}

TEST(LFSVTest, ConcurrentInserts) {
    MemoryBank bank(500);
    GarbageRemover remover(bank);
    LFSV lfsv(bank, remover);

    std::vector<std::thread> threads;
    int N = 10000; // number of elements to insert

    for (int i = 0; i < N; ++i) {
        threads.emplace_back([&lfsv, i]() {
            lfsv.Insert(i);
            });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // verify elements are sorted after concurrent inserts
    for (int i = 0; i < N; ++i) {
        ASSERT_EQ(i, lfsv[i]);
    }
}
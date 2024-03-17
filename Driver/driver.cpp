// debug_new.cpp
// compile by using: cl /EHsc /W4 /D_DEBUG /MDd debug_new.cpp
#include <algorithm>//copy, random_shuffle
#include <ctime>    //std::time (NULL) to seed srand
#include <functional> // std::bind
#include <iostream>
#include <vector>
#include <thread>
#include <random>
#include <chrono>
#include <cassert>
#include "ConcurrentSortedVector.h"
#include "Quicksort.h"
#include "Ratio.h"

//-------------------------------------------------------------
const std::vector<int> Threads = { 1,2,3,4,5,6,7,8, 16 };

//for the quick sort
constexpr int RATIO_DATA_SIZE = 100;
//for the concurrent vector
constexpr int DATA_SIZE = 50000;
constexpr int MEMORY_BANK_SIZE = 1000;
//-------------------------------------------------------------

std::atomic<bool> doread(true);

void GenerateTestData(std::vector<Ratio>& data);
void ReadPosition0(ConcurrentSortedVector<int>& concurrentSortedVector);
void RWTest(int num_threads, int num_per_thread);
void ConcurrentReadWriteTest();
void InsertRange(ConcurrentSortedVector<int>& concurrentSortedVector, int b, int e);

void QuickSortPerformanceTest(const std::vector<Ratio>& originalTestData);
void StdSortPerformanceTest(const std::vector<Ratio>& originalTestData);

int main(int /*argc*/, char** /*argv*/){
    std::cout << "The number of logical cores = " << std::thread::hardware_concurrency() << "\n\n";
    std::vector<Ratio> testData(RATIO_DATA_SIZE);
    GenerateTestData(testData);

    //(1) Concurrent Quicksort
    //StdSortPerformanceTest(testData);
    //QuickSortPerformanceTest(testData);

    //(2) Concurrent SortedVector
	ConcurrentReadWriteTest();

    return 0;
}

void GenerateTestData(std::vector<Ratio>& data) {
    std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<int> dist(1, RATIO_DATA_SIZE);

    for (auto& ratio : data) {
        int n = dist(gen);
        int d = dist(gen); 
        ratio = Ratio(n, d);
    }
}

void RWTest(int num_threads, int num_per_thread)
{
    MemoryBank<int> bank(MEMORY_BANK_SIZE);
    GarbageRemover<int> remover(bank);

    ConcurrentSortedVector<int> concurrentSortedVector(bank, remover);

    std::vector<std::thread> threads;
    concurrentSortedVector.Insert(-1);//for ReadPosition0
    std::thread reader = std::thread(ReadPosition0, std::ref(concurrentSortedVector));

    auto startTime = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < num_threads; ++i) {
        threads.push_back(std::thread(InsertRange, std::ref(concurrentSortedVector), i * num_per_thread, (i + 1) * num_per_thread));
    }
    for (auto& th : threads) th.join();

    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = endTime - startTime;
    std::cout << "Concurrent Read/Write Test using " << num_threads << " thread(s) executed "
        << DATA_SIZE << " operations in " << elapsed.count() << " seconds.\n";

    doread.store(false);
    reader.join();

    for (int i = 0; i < num_threads * num_per_thread; ++i) {
        if (!(concurrentSortedVector[i] == i - 1)) {
            std::cerr << "Error: not sorted at index " << i << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }
    std::cout << "All sorted!\n";
}
void ReadPosition0(ConcurrentSortedVector<int>& concurrentSortedVector) {
    int c = 0;
    while (doread.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        if (concurrentSortedVector[0] != -1) {
            std::cout << "not -1 on iteration " << c << "\n"; // see main - all element are non-negative, so index 0 should always be -1
        }
        ++c;
    }
}
void InsertRange(ConcurrentSortedVector<int>& concurrentSortedVector, int b, int e) {
    int* range = new int[e - b];
    for (int i = b; i < e; ++i) {
        range[i - b] = i;
    }
    std::srand(static_cast<unsigned int>(std::time(NULL)));
    std::random_shuffle(range, range + e - b);
    for (int i = 0; i < e - b; ++i) {
        concurrentSortedVector.Insert(range[i]);
    }
    delete[] range;
}

void ConcurrentReadWriteTest() {
    std::cout << "Initiating Concurrent Read/Write Performance Evaluation\n";

    for (int threadCount : Threads) {
        RWTest(threadCount, DATA_SIZE / threadCount);
    }
    std::cout << "Concurrent Read/Write Performance Evaluation Completed\n";
}


void QuickSortPerformanceTest(const std::vector<Ratio>& originalTestData) {
    std::cout << "Starting Quick Sort Performance Evaluation" << std::endl;

    for (int threads : Threads) {
        // copy testData from the original to ensure the same data for each test
        std::vector<Ratio> testData = originalTestData;

        // measure the time taken to sort the array using Quick Sort with the specified number of threads.
        auto startTime = std::chrono::high_resolution_clock::now();

        Quicksort(testData.data(), 0, testData.size(), threads);

        auto endTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsedSeconds = endTime - startTime;
        std::cout << "Quick Sort using " << threads << " threads sorted an array of " << RATIO_DATA_SIZE
            << " Ratios with delay in " << elapsedSeconds.count() << " seconds.\n" << std::endl;
    }
}

void StdSortPerformanceTest(const std::vector<Ratio>& originalTestData) {
    std::cout << "Starting std::sort Performance Evaluation" << std::endl;

    std::vector<Ratio> testData = originalTestData;

    auto startTime = std::chrono::high_resolution_clock::now();

    std::sort(testData.begin(), testData.end());
    
    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsedSeconds = endTime - startTime;
    std::cout << "std::sort sorted an array of " << RATIO_DATA_SIZE
        << " Ratios with delay in " << elapsedSeconds.count() << " seconds." << std::endl;
}

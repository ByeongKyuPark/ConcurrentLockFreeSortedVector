// debug_new.cpp
// compile by using: cl /EHsc /W4 /D_DEBUG /MDd debug_new.cpp
#include <algorithm>//copy, random_shuffle
#include <ctime>    //std::time (NULL) to seed srand
#include <functional> // std::bind
#include <cstdio> //sscanf
#include <iostream>
#include <vector>
#include <thread>
#include <random>
#include <chrono>
#include <cassert>
#include "LFSV.h"
#include "Quicksort.h"
#include "driver.h"

constexpr int DATA_SIZE = 5000000;
constexpr int MEMORY_BANK_SIZE = 100000;
const std::vector<int> threadCounts = { 1,2,3,4,5,6,7,8, 16 };

void GenerateTestData(std::vector<int>& data) {
    std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<int> dist(1, DATA_SIZE);
    std::generate(data.begin(), data.end(), [&] { return dist(gen); });
}

std::atomic<bool> doread(true);

void ReadPosition0(LFSV& lfsv);
void RWTest(int num_threads, int num_per_thread);
void WTest(int num_threads, int num_per_thread);
void ConcurrentReadWriteTest();
void InsertRange(LFSV& lfsv, int b, int e);

void QuickSortPerformanceTest(const std::vector<int>& originalTestData) {
    std::cout << "Starting Quick Sort Performance Evaluation" << std::endl;

    for (int threads : threadCounts) {
        // Copy testData from the original to ensure the same data for each test
        std::vector<int> testData = originalTestData;

        // Measure the time taken to sort the array using Quick Sort with the specified number of threads.
        auto startTime = std::chrono::high_resolution_clock::now();

        Quicksort(testData.data(), 0, testData.size(), threads); // Assuming this is your Quicksort function

        auto endTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsedSeconds = endTime - startTime;
        std::cout << "Quick Sort using " << threads << " threads sorted an array of " << DATA_SIZE
            << " elements in " << elapsedSeconds.count() << " seconds." << std::endl;
    }
}

void StdSortPerformanceTest(const std::vector<int>& originalTestData) {
    std::cout << "Starting std::sort Performance Evaluation" << std::endl;

    // Copy testData from the original to ensure the same data for each test
    std::vector<int> testData = originalTestData;

    auto startTime = std::chrono::high_resolution_clock::now();

    std::sort(testData.begin(), testData.end()); // Using std::sort

    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsedSeconds = endTime - startTime;
    std::cout << "std::sort sorted an array of " << DATA_SIZE
        << " elements in " << elapsedSeconds.count() << " seconds." << std::endl;
}

int main(int /*argc*/, char** /*argv*/){
 //   if (argc==2) { //use test[ argv[1] ]
	//	int test = 0;
	//	std::sscanf(argv[1],"%i",&test);
	//	try {
 //           pTests[test]();
	//	} catch( const char* msg) {
	//		std::cerr << msg << std::endl;
	//	}
 //       return 0;
	//}
    std::cout << "The number of logical cores = " << std::thread::hardware_concurrency() << "\n\n";
    std::vector<int> testData(DATA_SIZE);
    GenerateTestData(testData);

    //(1) quicksort
    QuickSortPerformanceTest(testData);
    StdSortPerformanceTest(testData);

    //(2) LFSV
	ConcurrentReadWriteTest();

    return 0;
}



void RWTest(int num_threads, int num_per_thread)
{
    MemoryBank bank(MEMORY_BANK_SIZE);
    GarbageRemover remover(bank);

    LFSV lfsv(bank, remover);

    std::vector<std::thread> threads;
    lfsv.Insert(-1);//for ReadPosition0
    std::thread reader = std::thread(ReadPosition0, std::ref(lfsv));

    for (int i = 0; i < num_threads; ++i) {
        threads.push_back(std::thread(InsertRange, std::ref(lfsv), i * num_per_thread, (i + 1) * num_per_thread));
    }
    for (auto& th : threads) th.join();

    doread.store(false);
    reader.join();

    for (int i = 0; i < num_threads * num_per_thread; ++i) {
        if (!(lfsv[i] == i - 1)) {
            std::cerr << "Error: not sorted at index " << i << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }
    std::cout << "All sorted!\n";
}
void WTest(int num_threads, int num_per_thread)
{
    MemoryBank bank(MEMORY_BANK_SIZE);
    GarbageRemover remover(bank);

    LFSV lfsv(bank, remover);


    std::vector<std::thread> threads;

    auto startTime = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < num_threads; ++i) {
        threads.push_back(std::thread(InsertRange, std::ref(lfsv), i * num_per_thread, (i + 1) * num_per_thread));
    }
    for (auto& th : threads) th.join();
    auto endTime = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> elapsed = endTime - startTime;
    std::cout << "Concurrent Write Test using " << num_threads << " thread(s) executed ()"
        << DATA_SIZE << " operations in " << elapsed.count() << " seconds.\n";

    bool isSorted = true;
    for (int i = 0; i < num_threads * num_per_thread - 1; ++i) {
        if (lfsv[i] > lfsv[i + 1]) {
            isSorted = false;
            break;
        }
    }

    if (isSorted) {
        std::cout << "All elements are correctly ordered.\n";
    }
    else {
        std::cerr << "Error: Elements are not in the correct order.\n";
        std::exit(EXIT_FAILURE);
    }
}
void ReadPosition0(LFSV& lfsv) {
    int c = 0;
    while (doread.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        if (lfsv[0] != -1) {
            std::cout << "not -1 on iteration " << c << "\n"; // see main - all element are non-negative, so index 0 should always be -1
        }
        ++c;
    }
}
void InsertRange(LFSV& lfsv, int b, int e) {
    int* range = new int[e - b];
    for (int i = b; i < e; ++i) {
        range[i - b] = i;
    }
    std::srand(static_cast<unsigned int>(std::time(NULL)));
    std::random_shuffle(range, range + e - b);
    for (int i = 0; i < e - b; ++i) {
        lfsv.Insert(range[i]);
    }
    delete[] range;
}

void ConcurrentReadWriteTest() {
    std::cout << "Initiating Concurrent Read/Write Performance Evaluation\n";

    for (int threadCount : threadCounts) {
        auto startTime = std::chrono::high_resolution_clock::now();

        RWTest(threadCount, DATA_SIZE / threadCount);

        auto endTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = endTime - startTime;
        std::cout << "Concurrent Read/Write Test using " << threadCount << " thread(s) executed ()"
            << DATA_SIZE << " operations in " << elapsed.count() << " seconds.\n";
    }
    std::cout << "Concurrent Read/Write Performance Evaluation Completed\n";
}
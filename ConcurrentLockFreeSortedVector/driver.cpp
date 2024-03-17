// debug_new.cpp
// compile by using: cl /EHsc /W4 /D_DEBUG /MDd debug_new.cpp
#include <algorithm>//copy, random_shuffle
#include <ctime>    //std::time (NULL) to seed srand

#include <cstdio> //sscanf
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <cassert>
#include "lfsv.h"

constexpr int TOTAL_OPERATIONS = 100000;

void insert_range(LFSV& lfsv, int b, int e ) {
    int * range = new int [e-b];
    for ( int i=b; i<e; ++i ) {
        range[i-b] = i;
    }
    std::srand( static_cast<unsigned int>(std::time (NULL)) );
    std::random_shuffle( range, range+e-b );
    for ( int i=0; i<e-b; ++i ) {
        lfsv.Insert( range[i] );
    }
    delete [] range;
}

std::atomic<bool> doread( true );

void read_position_0(LFSV& lfsv) {
    int c = 0;
    while ( doread.load() ) {
        std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
        if ( lfsv[0] != -1 ) {
            std::cout << "not -1 on iteration " << c << "\n"; // see main - all element are non-negative, so index 0 should always be -1
        }
        ++c;
    }
}

void test( int num_threads, int num_per_thread )
{
    MemoryBank bank(15000);
    GarbageRemover remover(bank);

    LFSV lfsv(bank, remover);

    std::vector<std::thread> threads;
    lfsv.Insert( -1 );
    std::thread reader = std::thread(read_position_0, std::ref(lfsv));

    for (int i=0; i<num_threads; ++i) {
        threads.push_back( std::thread( insert_range,std::ref(lfsv), i*num_per_thread, (i+1)*num_per_thread ) );
    }
    for (auto& th : threads) th.join();

    doread.store( false );
    reader.join();

    for (int i=0; i<num_threads*num_per_thread; ++i) { 
        if (!(lfsv[i] == i - 1)) {
            std::cerr << "Error: not sorted at index " << i << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }
    std::cout << "All sorted!\n";
}

void ConcurrentReadWriteTest() {
    std::cout << "The number of logical cores =" << std::thread::hardware_concurrency() << '\n';
    const std::vector<int> threadCounts = { 1,2, 4, 8, 16 };

    for (int threadCount : threadCounts) {
        auto startTime = std::chrono::high_resolution_clock::now();

        test(threadCount, TOTAL_OPERATIONS / threadCount);

        auto endTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = endTime - startTime;
        std::cout << "Test with " << threadCount << " thread(s) completed in " << elapsed.count() << " seconds." << std::endl;
    }
}

void (*pTests[])() = { 
    ConcurrentReadWriteTest
}; 

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
    ConcurrentReadWriteTest();

    return 0;
}



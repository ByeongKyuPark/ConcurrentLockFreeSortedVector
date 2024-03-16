// debug_new.cpp
// compile by using: cl /EHsc /W4 /D_DEBUG /MDd debug_new.cpp
//#define _CRTDBG_MAP_ALLOC
//#include <cstdlib>
//#include <crtdbg.h>

#ifdef _DEBUG
#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
// Replace _NORMAL_BLOCK with _CLIENT_BLOCK if you want the
// allocations to be of _CLIENT_BLOCK type
#else
#define DBG_NEW new
#endif

#include "lfsv.h"

#include <algorithm>//copy, random_shuffle
#include <ctime>    //std::time (NULL) to seed srand
#include <chrono>

#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <cassert>

constexpr int TOTAL_OPERATIONS = 500000;

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
    MemoryBank bank(30000);
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
        //        std::cout << lfsv[i] << ' '; 
        assert(lfsv[i] == i - 1,"Error::not sorted");
    }
    std::cout << "All sorted\n";
}

void test0() { test( 1, 10 ); }
void test1() { test( 2, 10 ); }
void test2() { test( 8, 100 ); }
void test3() { test( 16, 100 ); } 
//void customTest() { 
//    test(4, 5000);
//    //test(64, 300);//<---time:10.722 seconds, 11.0908 seconds
//                    //release: 5.41367 seconds, 5.59725 seconds
//
//    //test(128, 300);//<---time:
//    //test(8, 15); 
//}



void customTest() {
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
    test0,test1,test2,test3,customTest
}; 


#include <cstdio>    /* sscanf */
int main( int argc, char ** argv ) {
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
    //test3();
    //test3();
    //test0();
    customTest();
    //_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
    //_CrtDumpMemoryLeaks();

    return 0;
}

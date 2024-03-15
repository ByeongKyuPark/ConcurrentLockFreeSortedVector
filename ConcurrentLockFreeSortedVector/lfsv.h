#include <iostream>       // std::cout
#include <atomic>         // std::atomic
#include <thread>         // std::thread
#include <vector>         // std::vector
#include <deque>          // std::deque
#include <mutex>          // std::mutex

struct Pair {
    std::vector<int>* pointer;
    long              ref_count;
}; // __attribute__((aligned(16),packed));
// for some compilers alignment needed to stop std::atomic<Pair>::load to segfault

class MemoryBank {
    std::deque< std::vector<int>*> slots;
    std::mutex m;
public:
    MemoryBank() : slots(50000){//6000->3000
        for (int i = 0; i < 50000; ++i) {
            slots[i] = reinterpret_cast<std::vector<int>*>(new char[sizeof(std::vector<int>)] {});
        }
    }
    std::vector<int>* Get() {
        std::lock_guard<std::mutex> lock(m);
        std::vector<int>* p = slots[0];
        slots.pop_front();
        return p;
    }
    void Store(std::vector<int>* p) {
        std::lock_guard<std::mutex> lock(m);
        slots.push_back(p);
    }
    ~MemoryBank() {
        for (auto& el : slots) { 
            el->~vector();//ADDED
            delete[] reinterpret_cast<char*>(el); 
        }
    }
};

class LFSV {
    MemoryBank mb;
    std::atomic< Pair > pdata;

public:

    LFSV() : mb{}, pdata(Pair{ new (mb.Get()) std::vector<int>, 1 }) {
		//        std::cout << "Is lockfree " << pdata.is_lock_free() << std::endl;
	}

    ~LFSV() { 
        mb.Store(pdata.load().pointer);
    }

    void Insert( int const & v ) {
        Pair pdata_new, pdata_old;
        pdata_new.pointer  = nullptr;
        do {

            if (pdata_new.pointer) {
                mb.Store(pdata_new.pointer);
            }

            pdata_old.pointer = pdata.load().pointer;
            pdata_old.ref_count = 1;
            pdata_new.pointer = new (mb.Get()) std::vector<int>(*pdata_old.pointer);
            pdata_new.ref_count = 1;

            // working on a local copy
            std::vector<int>::iterator b = pdata_new.pointer->begin();
            std::vector<int>::iterator e = pdata_new.pointer->end();
            if ( b==e || v>=pdata_new.pointer->back() ) { pdata_new.pointer->push_back( v ); } //first in empty or last element
            else {
                for ( ; b!=e; ++b ) {
                    if ( *b >= v ) {
                        pdata_new.pointer->insert( b, v );
                        break;
                    }
                }
            }

        } while ( !(this->pdata).compare_exchange_weak( pdata_old, pdata_new  ));        
        pdata_old.pointer->~vector();//ADDED
        mb.Store(pdata_old.pointer);
    }

    int operator[] (int pos) { // not a const method anymore
        Pair pdata_new, pdata_old;
        do { // before read - increment counter, use CAS
            pdata_old = pdata.load();
            pdata_new = pdata_old;
            ++pdata_new.ref_count;
        } while (!(this->pdata).compare_exchange_weak(pdata_old, pdata_new));

        int ret_val = (*pdata_new.pointer)[pos];

        do { // before return - decrement counter, use CAS
            pdata_old = pdata.load();
            pdata_new = pdata_old;
            --pdata_new.ref_count;
        } while (!(this->pdata).compare_exchange_weak(pdata_old, pdata_new));

        return ret_val;
    }
};

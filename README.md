## High-Performance Concurrent Sorted Vector

This project introduces a concurrent sorted vector designed to support efficient and thread-safe concurrent write and read operations. By utilizing advanced C++ features, this solo-developed project aims to meet the demands of applications requiring real-time data processing.

### Performance Insights

- **Quick Sort Performance**: A key highlight is the parallel Quick Sort algorithm, which vastly outperforms `std::sort`. When sorting 100 `Ratio` objects, it completes the task in **3.75904 seconds using 7 threads**, compared to the **16.3723 seconds** taken by `std::sort`, demonstrating a **4.35x speed improvement**. This performance showcases the algorithm's effective use of multi-threading.

- **Concurrent Read/Write Performance**: The vector's ability to efficiently manage concurrent operations is showcased by its performance under simultaneous read/write conditions. Transitioning from **1 to 6 writing threads** while continuously reading from the container results in operation times dropping from **1.90929 seconds to 1.12647 seconds**, illustrating its robustness in concurrent modification scenarios.

### Implementation Highlights

#### Quick Sort with Bag of Tasks

The parallel Quick Sort algorithm leverages a "bag of tasks" model to distribute sorting work among multiple threads, which is essential for reducing completion time and optimizing CPU usage. 

```cpp
template<typename T>
void Quicksort(T* a, unsigned begin, unsigned end, int num_threads) {
    std::vector<std::thread> threads;
    threads.reserve(num_threads);

    ThreadSafeContainer<T> bag(end - begin, num_threads);
    bag.Push(std::make_pair(a, std::make_pair(begin, end)));

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&bag]() {
            QuicksortIterativeAux(bag);
        });
    }
    for (auto& t : threads) {
        t.join();
    }
}
```

#### Memory Management and Thread Safety

Efficient memory management and thread safety are achieved through a combination of thread-local caching for insert operations and atomic operations for managing shared data integrity, ensuring optimal performance even under high concurrency.

#### Atomic Operations and Compare-And-Swap (CAS)

The project extensively uses atomic operations and the CAS technique for managing the vector's state without traditional locking mechanisms. This atomic-based approach is crucial for the lock-free insertion process, addressing the ABA problem and ensuring data consistency among concurrent threads.

### The Role of the Ratio Class

The `Ratio` class, equipped with built-in delays for comparison operations, plays a crucial role in evaluating the sorting algorithm's time complexity and performance. This class mimics the computational load of real-world scenarios, offering a controlled benchmark that accurately reflects the vector's capabilities in handling complex operations efficiently.

### Application Scenarios

This vector is exceptionally suited for:

- **Real-time Data Analytics**: Facilitating quick processing and analysis of streaming data.
- **Interactive Gaming**: Managing dynamic entities and leaderboards in real time.
- **Financial Trading**: Accelerating data updates and access for trading algorithms.
- **Scientific Simulations**: Modeling systems with concurrent data interactions accurately and efficiently.

### Conclusion

The concurrent sorted vector, with its parallel Quick Sort implementation and sophisticated thread safety mechanisms, represents a significant advancement in the field of high-performance computing. This project illustrates the potential for modern C++ to enhance data structure performance, particularly in applications that demand fast, concurrent processing of data.

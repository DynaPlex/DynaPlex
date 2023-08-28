#include <iostream>
#include <deque>
#include <vector>
#define NOMINMAX
#include <windows.h>
#include <psapi.h>
#include "dynaplex/modelling_helpers/queue.h"

size_t getMemoryUsage() {
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
        return pmc.PrivateUsage;
    }
    return 0;  // Error
}

int main() {
    const size_t num = 100000;
    const size_t elements = 10;

    // Measure memory usage for deque
    size_t startDequeMem = getMemoryUsage();

    std::vector<std::deque<int64_t>> deques;
    deques.reserve(num);
    for (size_t i = 0; i < num; i++) {
        deques.push_back(std::deque<int64_t>{});
        for (size_t j = 0; j < elements; j++)
        {
            deques.back().push_back(0);
        }
    }
    size_t endDequeMem = getMemoryUsage();

    // Measure memory usage for Queue
    size_t startFIFOMem = getMemoryUsage();

    std::vector<DynaPlex::Queue<int64_t>> Queues;
    Queues.reserve(num);
    for (size_t i = 0; i < num; i++) {
        Queues.push_back(DynaPlex::Queue{ elements });
        for (size_t j = 0; j < elements; j++)
        {
            Queues.back().push_back(0);
        }
    }
    size_t endFIFOMem = getMemoryUsage();

    // Measure memory usage for vector
    size_t startVectorMem = getMemoryUsage();

    std::vector<std::vector<int64_t>> vectors;
    vectors.reserve(num);
    for (size_t i = 0; i < num; i++) {
        vectors.push_back(std::vector<int64_t>());
        vectors.back().reserve(elements);
        for (size_t j = 0; j < elements; j++)
        {
            vectors.back().push_back(0);
        }
    }
    size_t endVectorMem = getMemoryUsage();

    // Report memory usage for both classes
    std::cout << "Average memory used by a deque: " << 1.0 * (endDequeMem - startDequeMem) / elements / num << " bytes" << std::endl;
    std::cout << "Average memory used by a Queue: " << 1.0 * (endFIFOMem - startFIFOMem) / elements / num << " bytes" << std::endl;
    std::cout << "Average memory used by a vector: " << 1.0 * (endVectorMem - startVectorMem) / elements / num << " bytes" << std::endl;
    return 0;
}
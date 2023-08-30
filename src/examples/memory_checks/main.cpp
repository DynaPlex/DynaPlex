#include <iostream>
#include <deque>
#include <vector>
#define NOMINMAX
#include <windows.h>
#include <psapi.h>
#include "dynaplex/modelling/queue.h"
#include "dynaplex/rng.h"

size_t getMemoryUsage() {
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
        return pmc.PrivateUsage;
    }
    return 0;  // Error
}

void CheckMemContainers()
{
    const size_t num = 50000;
    const size_t elements = 40;

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
        Queues.push_back(DynaPlex::Queue<int64_t>(elements));
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
}

void CheckMemRNG()
{
    const size_t num = 100000;

    // Measure memory usage for mt19937
    size_t startmtMem = getMemoryUsage();
    
    std::vector<std::mt19937> mt;
    mt.reserve(num);
    for (size_t i = 0; i < num; i++) {
        std::seed_seq seq{ i };
        mt.push_back(std::mt19937(seq));
    }
    size_t endmtMem = getMemoryUsage();

    // Measure memory usage for pcg64
    size_t startpcgMem = getMemoryUsage();

    std::vector<pcg_cpp::pcg64> pcg;
    pcg.reserve(num);
    for (size_t i = 0; i < num; i++) {
		std::seed_seq seq{ i,i+1,i+123 };
		pcg.push_back(pcg_cpp::pcg64(seq));
	}
    size_t endpcgMem = getMemoryUsage();

    // measure memory usage for ranlux48
    size_t startranluxMem = getMemoryUsage();

    std::vector<std::ranlux48> ranlux;
    ranlux.reserve(num);
    for (size_t i = 0; i < num; i++) {
		std::seed_seq seq{ i };
		ranlux.push_back(std::ranlux48(seq));
	}
    size_t endranluxMem = getMemoryUsage();


  

    // Report memory usage for the random generators


    std::cout << "Average memory used by a mt19937: " << 1.0 * (endmtMem - startmtMem) / num << " bytes" << std::endl;
    std::cout << "Average memory used by a pcg64: " << 1.0 * (endpcgMem - startpcgMem) / num << " bytes" << std::endl;
    std::cout << "Average memory used by a ranlux48: " << 1.0 * (endranluxMem - startranluxMem) / num << " bytes" << std::endl;

}

int main() {
    CheckMemContainers();

    CheckMemRNG();



}
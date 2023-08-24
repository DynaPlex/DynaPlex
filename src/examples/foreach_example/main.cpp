#include <iostream>
#include <deque>
#include <windows.h>
#include <psapi.h>

size_t getMemoryUsage() {
    PROCESS_MEMORY_COUNTERS memCounter;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &memCounter, sizeof(memCounter))) {
        return memCounter.WorkingSetSize;
    }
    return 0; // error, you might want to handle this more gracefully
}

int main() {
    const size_t numDeques = 100;

    size_t startMem = getMemoryUsage();

    // Create multiple deques to get more accurate average memory usage reading
    std::deque<std::deque<int>> deques;
    for (size_t i = 0; i < numDeques; i++) {
        deques.push_back(std::deque<int>(100000, 42));
    }

    size_t endMem = getMemoryUsage();

    std::cout << "Average memory used by a deque: " << (endMem - startMem) / numDeques << " bytes" << std::endl;

    return 0;
}
#pragma once
#include <vector>
#include <string>
#include <cstddef>

class FlatMemoryAllocator {
public:

    FlatMemoryAllocator(size_t maximumSize);
    bool allocate(int pid, size_t size, size_t& startIndex);
    void deallocate(int pid);
    std::string visualizeMemory() const;
    int getNumProcessesInMemory() const;
    size_t getExternalFragmentation(size_t processSize) const;
    void reset();
    const std::vector<int>& getMemory() const;

private:
    size_t maximumSize;
    std::vector<int> memory;
    void initializeMemory();
};

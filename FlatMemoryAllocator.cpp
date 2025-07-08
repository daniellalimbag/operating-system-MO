#include "FlatMemoryAllocator.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <unordered_set>

FlatMemoryAllocator::FlatMemoryAllocator(size_t maximumSize)
    : maximumSize(maximumSize), memory(maximumSize, -1) {}

void FlatMemoryAllocator::initializeMemory() {
    std::fill(memory.begin(), memory.end(), -1);
}

bool FlatMemoryAllocator::allocate(int pid, size_t size, size_t& startIndex) {
    if (size > maximumSize) return false;
    size_t freeCount = 0;
    for (size_t i = 0; i < maximumSize; ++i) {
        if (memory[i] == -1) {
            ++freeCount;
            if (freeCount == size) {
                size_t start = i + 1 - size;
                std::fill(memory.begin() + start, memory.begin() + start + size, pid);
                startIndex = start;
                return true;
            }
        } else {
            freeCount = 0;
        }
    }
    return false;
}

void FlatMemoryAllocator::deallocate(int pid) {
    for (auto& cell : memory) {
        if (cell == pid) cell = -1;
    }
}

std::string FlatMemoryAllocator::visualizeMemory() const {
    std::ostringstream oss;
    for (size_t i = 0; i < memory.size(); ++i) {
        if (memory[i] == -1) oss << '.';
        else oss << (memory[i] % 10);
        if ((i + 1) % 64 == 0) oss << '\n';
    }
    return oss.str();
}

int FlatMemoryAllocator::getNumProcessesInMemory() const {
    std::unordered_set<int> pids;
    for (int cell : memory) {
        if (cell != -1) pids.insert(cell);
    }
    return static_cast<int>(pids.size());
}

size_t FlatMemoryAllocator::getExternalFragmentation(size_t processSize) const {
    size_t frag = 0, freeCount = 0;
    for (size_t i = 0; i < memory.size(); ++i) {
        if (memory[i] == -1) ++freeCount;
        else {
            if (freeCount > 0 && freeCount < processSize) frag += freeCount;
            freeCount = 0;
        }
    }
    if (freeCount > 0 && freeCount < processSize) frag += freeCount;
    return frag;
}

void FlatMemoryAllocator::reset() {
    initializeMemory();
}

const std::vector<int>& FlatMemoryAllocator::getMemory() const {
    return memory;
}

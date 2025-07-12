#pragma once
#include <vector>
#include <ostream>
#include <map>

class FirstFitMemoryAllocator;
extern FirstFitMemoryAllocator* globalMemoryAllocator;

struct Block {
    int start;
    int size;
    Block(int s, int sz) : start(s), size(sz) {}
};

struct AllocatedBlock {
    int processId;
    int start;
    int size;
    AllocatedBlock(int pid, int s, int sz) : processId(pid), start(s), size(sz) {}
};

class FirstFitMemoryAllocator {
public:
    FirstFitMemoryAllocator(int totalMem, int memPerProc);
    bool isAllocated(int processId) const;
    bool allocate(int processId);
    void release(int processId);
    int getExternalFragmentation() const;
    int getNumProcessesInMemory() const;
    void printMemory(std::ostream& out) const;
    void getMemorySnapshot(std::vector<AllocatedBlock>& outBlocks, std::vector<Block>& outFreeBlocks) const;
private:
    int totalMemory;
    int memPerProc;
    std::vector<Block> freeBlocks;
    std::vector<AllocatedBlock> allocatedBlocks;
    void mergeFreeBlocks();
};

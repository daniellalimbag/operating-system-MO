#include "FirstFitMemoryAllocator.h"

FirstFitMemoryAllocator* globalMemoryAllocator = nullptr;
#include <algorithm>
#include <iomanip>
#include <ctime>
#include <sstream>

FirstFitMemoryAllocator::FirstFitMemoryAllocator(int totalMem, int memPerProc)
    : totalMemory(totalMem), memPerProc(memPerProc) {
    freeBlocks.push_back(Block(0, totalMem));
}

bool FirstFitMemoryAllocator::allocate(int processId) {
    for (size_t i = 0; i < freeBlocks.size(); ++i) {
        Block& block = freeBlocks[i];
        if (block.size >= memPerProc) {
            allocatedBlocks.push_back(AllocatedBlock(processId, block.start, memPerProc));
            if (block.size == memPerProc) {
                freeBlocks.erase(freeBlocks.begin() + i);
            } else {
                block.start += memPerProc;
                block.size -= memPerProc;
            }
            return true;
        }
    }
    return false;
}

void FirstFitMemoryAllocator::release(int processId) {
    auto it = std::find_if(allocatedBlocks.begin(), allocatedBlocks.end(), [processId](const AllocatedBlock& ab) {
        return ab.processId == processId;
    });
    if (it != allocatedBlocks.end()) {
        freeBlocks.push_back(Block(it->start, it->size));
        allocatedBlocks.erase(it);
        mergeFreeBlocks();
    }
}

bool FirstFitMemoryAllocator::isAllocated(int processId) const {
    return std::any_of(allocatedBlocks.begin(), allocatedBlocks.end(), [processId](const AllocatedBlock& ab) {
        return ab.processId == processId;
    });
}

void FirstFitMemoryAllocator::mergeFreeBlocks() {
    if (freeBlocks.empty()) return;
    std::sort(freeBlocks.begin(), freeBlocks.end(), [](const Block& a, const Block& b) { return a.start < b.start; });
    std::vector<Block> merged;
    merged.push_back(freeBlocks[0]);
    for (size_t i = 1; i < freeBlocks.size(); ++i) {
        Block& last = merged.back();
        if (last.start + last.size == freeBlocks[i].start) {
            last.size += freeBlocks[i].size;
        } else {
            merged.push_back(freeBlocks[i]);
        }
    }
    freeBlocks = std::move(merged);
}

int FirstFitMemoryAllocator::getExternalFragmentation() const {
    int frag = 0;
    for (const Block& b : freeBlocks) {
        if (b.size < memPerProc) {
            frag += b.size;
        }
    }
    return frag;
}

int FirstFitMemoryAllocator::getNumProcessesInMemory() const {
    return (int)allocatedBlocks.size();
}

void FirstFitMemoryAllocator::printMemory(std::ostream& out) const {
    std::vector<AllocatedBlock> sortedBlocks = allocatedBlocks;
    std::sort(sortedBlocks.begin(), sortedBlocks.end(), [](const AllocatedBlock& a, const AllocatedBlock& b) {
        return a.start > b.start;
    });
    int lastAddr = totalMemory;
    out << "----end---- = " << totalMemory << "\n\n";
    for (const auto& ab : sortedBlocks) {
        out << lastAddr << "\n";
        out << "P" << ab.processId << "\n";
        out << ab.start << "\n\n";
        lastAddr = ab.start;
    }
    out << "----start----- = 0\n";
}

void FirstFitMemoryAllocator::getMemorySnapshot(std::vector<AllocatedBlock>& outBlocks, std::vector<Block>& outFreeBlocks) const {
    outBlocks = allocatedBlocks;
    outFreeBlocks = freeBlocks;
}

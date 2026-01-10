//
// Created by aspadien on 08.01.2026.
//

#ifndef CUBEREBUILD_CHUNKALLOCATOR_HPP
#define CUBEREBUILD_CHUNKALLOCATOR_HPP
#include <stack>

#include "Configuration/Config.h"
#include "Configuration/Constants.hpp"


class ChunkAllocator {
    std::stack<uint8_t*> pool;
    std::mutex poolMutex;

public:
    static ChunkAllocator& Get() {
        static ChunkAllocator instance;
        return instance;
    }

    uint8_t* Allocate() {
        std::lock_guard<std::mutex> lock(poolMutex);
        if (!pool.empty()) {
            uint8_t* ptr = pool.top();
            pool.pop();
            return ptr;
        }
        return new uint8_t[CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z];
    }

    void Free(uint8_t* ptr) {
        if (!ptr) return;
        if (!programIsRunning) {
            delete[] ptr; // Если программа закрывается, чистим сразу
            return;
        }
        std::lock_guard<std::mutex> lock(poolMutex);
        pool.push(ptr);
    }
};

#endif //CUBEREBUILD_CHUNKALLOCATOR_HPP
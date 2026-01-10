// --- ФАЙЛ: ChunkSystem/ChunkGenerationManager.h (или ChunkManager.hpp) ---
#pragma once
#include <mutex>
#include <atomic>
#include <cmath>
#include <glm/vec3.hpp>

#include "HashSystem.hpp"

class Chunk{
public:
    glm::ivec3 worldPosition;
    uint8_t* blocks = nullptr;

    bool needsMeshUpdate = false;

    void* renderInfo = nullptr;
    size_t renderListIndex = -1;

    std::mutex mutex;

     [[nodiscard]] uint8_t get(int x, int y, int z) const;

    explicit Chunk(glm::ivec3 pos);

    ~Chunk();

};

inline glm::ivec3 getChunkIndex(const glm::vec3 worldPos) {
    int X = static_cast<int>(std::floor(worldPos.x / 32.0f));
    int Y = static_cast<int>(std::floor(worldPos.y / 32.0f));
    int Z = static_cast<int>(std::floor(worldPos.z / 32.0f));

    return {X, Y, Z};
}

inline uint8_t getBlock(const glm::vec3 worldPos, const ChunkMap &chunks) {
    glm::ivec3 chunkIndex = getChunkIndex(worldPos);

    int x = static_cast<int>(std::floor(worldPos.x)) % 32;
    int y = static_cast<int>(std::floor(worldPos.y)) % 32;
    int z = static_cast<int>(std::floor(worldPos.z)) % 32;

    if (x < 0) x += 32;
    if (y < 0) y += 32;
    if (z < 0) z += 32;

    auto it = chunks.tryGet(chunkIndex);
    if (it == nullptr) {
        return 0; // нет чанка → воздух
    }

    return it->get(x, y, z);
}

inline bool isSolidBlock(const glm::vec3 pos, ChunkMap &chunks) {
    return getBlock(pos, chunks) > 0;
}
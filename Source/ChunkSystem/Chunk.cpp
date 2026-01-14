// --- ВАЖНО: Сначала подключаем зависимости реализации ---
module;

#include <cstdint>
#include <cmath>          // std::floor
#include <iostream>
#include <algorithm>

// GLM нужен здесь, чтобы видеть операторы векторов
#include <glm/vec3.hpp>

// ЭТОГО не было видно в cpp файле, поэтому были ошибки:

#include "../../Definitions/Core/Constants.hpp"

// --- Теперь говорим, что это реализация модуля Chunk ---
import ChunkAllocator;

module Chunk;

// --- Реализация методов ---

uint8_t Chunk::get(const int x, const int y, const int z) const {
    if (blocks == nullptr) {
        return 255; // Лучше вернуть явный код ошибки (или 0), -1 для uint8_t это 255
    }
    // CHUNK_SIZE теперь будет виден благодаря #include "Configuration/Constants.hpp" выше
    if (x < 0 || x >= CHUNK_SIZE || y < 0 || y >= CHUNK_SIZE || z < 0 || z >= CHUNK_SIZE) {
        return 255;
    }
    return blocks[x + y*CHUNK_SIZE + z*CHUNK_SIZE*CHUNK_SIZE];
}

Chunk::Chunk(const glm::ivec3 pos) : worldPosition(pos) {
    // ChunkAllocator теперь виден
    blocks = ChunkAllocator::Get().Allocate();
}

Chunk::~Chunk() {
    if (blocks) ChunkAllocator::Get().Free(blocks);
}

glm::ivec3 getChunkIndex(const glm::vec3 worldPos) {
    // std::floor теперь виден благодаря <cmath>
    int X = static_cast<int>(std::floor(worldPos.x / 32.0f));
    int Y = static_cast<int>(std::floor(worldPos.y / 32.0f));
    int Z = static_cast<int>(std::floor(worldPos.z / 32.0f));
    return {X, Y, Z};
}

uint8_t getBlock(const glm::vec3 worldPos, const ChunkMap& chunks) {
    glm::ivec3 chunkIndex = getChunkIndex(worldPos);

    int x = static_cast<int>(std::floor(worldPos.x)) % 32;
    int y = static_cast<int>(std::floor(worldPos.y)) % 32;
    int z = static_cast<int>(std::floor(worldPos.z)) % 32;

    if (x < 0) x += 32;
    if (y < 0) y += 32;
    if (z < 0) z += 32;

    // ShardedMap методы (tryGet) должны быть определены в хедере Hash_Map_Modification.hpp
    // Если tryGet возвращает указатель или optional:
    auto ptr = chunks.tryGet(chunkIndex);
    if (!ptr) return 0;

    // Если ptr это shared_ptr<Chunk> (как объявлено в using ChunkMap)
    return (*ptr).get(x, y, z);
}

bool isSolidBlock(const glm::vec3 pos, ChunkMap& chunks) {
    return getBlock(pos, chunks) > 0;
}








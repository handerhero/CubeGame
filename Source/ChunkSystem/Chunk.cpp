//
// Created by aspadien on 08.01.2026.
//
#include "ChunkSystem/Chunk.hpp"

#include "ChunkSystem/ChunkAllocator.hpp"
#include "Configuration/Constants.hpp"


uint8_t Chunk::get(const int x, const int y, const int z) const {
    if (blocks == nullptr) {
        return -1;
    }
    if (x < 0 || x >= CHUNK_SIZE || y < 0 || y >= CHUNK_SIZE || z < 0 || z >= CHUNK_SIZE) {
        return -1;
    }
    return blocks[x + y*CHUNK_SIZE + z*CHUNK_SIZE*CHUNK_SIZE];
}

Chunk::Chunk(const glm::ivec3 pos) : worldPosition(pos) {
    blocks = ChunkAllocator::Get().Allocate();
}

Chunk::~Chunk() {
    if (blocks) ChunkAllocator::Get().Free(blocks);
    // renderInfo мы не удаляем здесь, его должен очистить GpuManager ПЕРЕД удалением чанка
}


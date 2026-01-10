//
// Created by aspadien on 08.01.2026.
//

#ifndef CUBEREBUILD_HASHSYSTEM_HPP
#define CUBEREBUILD_HASHSYSTEM_HPP
#include "Hash_Map_Modification.hpp"
#include "glm/vec3.hpp"

class Chunk;
// ... Ваши хешеры и ChunkMap без изменений ...
struct GoodVec3Hasher {
    std::size_t operator()(const glm::ivec3& v) const noexcept {
        std::size_t seed = std::hash<int>{}(v.x);
        seed ^= std::hash<int>{}(v.y) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<int>{}(v.z) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
    }
};

struct FastIVec3Equal {
    bool operator()(const glm::ivec3& a, const glm::ivec3& b) const noexcept {
        return a.x == b.x && a.y == b.y && a.z == b.z;
    }
};

using ChunkMap = ThreadSafeMap<glm::ivec3, std::shared_ptr<Chunk>, GoodVec3Hasher, FastIVec3Equal>;
using ChunkSet = ThreadSafeSet<glm::ivec3, GoodVec3Hasher, FastIVec3Equal>;



#endif //CUBEREBUILD_HASHSYSTEM_HPP
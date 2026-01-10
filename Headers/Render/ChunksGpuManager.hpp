#pragma once
#include <glad/glad.h>
#include <vector>
#include <memory>
#include "Utils/OptimizationTools.hpp"
#include "ChunkSystem/ChunkGenerationManager.h"
#include "Configuration/Config.h"

// Размер буфера: 256 МБ (хватит на ~20-30k чанков)
// Увеличивайте при необходимости
constexpr uint32_t MAX_VERTEX_BUFFER_SIZE = 256 * 1024 * 1024 / 4;
constexpr int BUFFER_FRAMES = 3;

struct ZombieIndex {
    int index;
    int frameOfDeath;
};

inline std::vector<ZombieIndex> zombies;
inline int globalFrameCounter = 0;

class ChunkGpuManager {
public:
    ChunkGpuManager(int maxDist, int maxHeight);
    ~ChunkGpuManager();

    void uploadChunk(Chunk* chunk, const std::vector<uint32_t>& meshData);
    void freeChunk(Chunk* chunk) const;

    // Синхронизация памяти (если используете Coherent, барьер делает драйвер, но для надежности оставим)
    static void syncMemory();

    // Контекст для Indirect Rendering
    struct {
        GLuint commandBuffer{}; // Буфер команд (DrawArraysIndirectCommand[])
        // Мы не мапим его на CPU для чтения, так как пишет туда Compute Shader
    } indirectContext;

    GLuint parameterBuffer = 0; // Атомарный счетчик (Count Buffer)
    GLuint vao = 0;
    GLuint vertexSSBO = 0;      // Вся геометрия (Static)
    GLuint chunkInfoBuffer = 0; // Метаданные (ChunkMetadata[])

    int maxChunksCapacity = 0;
    void recycleZombies();

private:
    std::unique_ptr<VramAllocator> allocator;
    std::vector<int> freeChunkMetadataIndicesList;

    // Указатели на Persistent Mapped память
    uint32_t* mappedVertices = nullptr;
    ChunkMetadata* mappedChunksInfos = nullptr;

    int allocateChunkMetadataIndex();
    void freeChunkMetadataIndex(int index);


};

// Функция мешинга без изменений
std::vector<uint32_t> BuildChunkMesh(const Chunk* center, const ChunkMap& map);
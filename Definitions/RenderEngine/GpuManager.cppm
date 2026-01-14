module;
#include <glad/glad.h>
#include <vector>
#include <memory>
#include "../Core/Config.h"

import ChunkGenerationSystem;
import Chunk;
import VramAllocator;
export module GpuManager;
// Размер буфера: 256 МБ (хватит на ~20-30k чанков)
// Увеличивайте при необходимости
constexpr uint32_t MAX_VERTEX_BUFFER_SIZE = 512 * 1024 * 1024 / 4;
constexpr int BUFFER_FRAMES = 3;

struct ZombieIndex {
     int index;
    uint64_t frameOfDeath;
};
export  std::vector<ZombieIndex> zombies;
export  uint64_t globalFrameCounter = 0;
// Структура для хранения отложенного обновления
struct PendingUpdate {
    Chunk* chunk;           // Указатель на чанк (для проверки валидности)
    uint32_t newOffset;     // VRAM адрес нового меша
    uint32_t newSize;       // Размер нового меша
    uint32_t oldOffset;     // Старый адрес (для удаления)
    uint32_t oldSize;       // Старый размер
    uint64_t targetFrame;   // Когда применять
    int metaIndex;          // Индекс слота в SSBO
};

std::vector<PendingUpdate> pendingUpdates;

// Настройка задержки. При 1000 FPS 3 кадра = 3 мс, этого хватит для PCIe передачи.
const int UPDATE_DELAY_FRAMES = 3;


export class GpuManager {
public:

    struct ZombieMemory {
        uint32_t offset;
        uint32_t size;
        uint64_t frameOfDeath;
    };
    std::vector<ZombieMemory> memoryZombies; // Очередь на удаление памяти

    GpuManager(int maxDist, int maxHeight);
    ~GpuManager();

    void uploadChunk(Chunk* chunk, const std::vector<uint32_t>& meshData);
    void freeChunk(Chunk* chunk);
    void processPendingUpdates();

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
    std::unique_ptr<VRamAllocator> allocator;
    std::vector<int> freeChunkMetadataIndicesList;

    // Указатели на Persistent Mapped память
    uint32_t* mappedVertices = nullptr;
    ChunkMetadata* mappedChunksInfos = nullptr;

    int allocateChunkMetadataIndex();
    void freeChunkMetadataIndex(int index);


};

// Функция мешинга без изменений
export std::vector<uint32_t> BuildChunkMesh(const Chunk* center, const ChunkMap& map);
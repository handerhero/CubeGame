module;
#include <condition_variable>
#include <queue>
#include <atomic>
#include <mutex>
#include <memory>
#include <glm/vec3.hpp>
import Chunk;

export module ChunkGenerationSystem;

// Подключаем ChunkManager.hpp, так как там определен struct Chunk, ChunkMap и ChunkSet



// Структура задачи для генерации
export struct ChunkGenerationTask {
    glm::ivec3 chunkPos;
    float distToPlayerSq; // Дистанция в квадрате

    // Приоритетная очередь: min dist -> top
    bool operator<(const ChunkGenerationTask& other) const {
        return distToPlayerSq > other.distToPlayerSq;
    }
};


// 1. Очередь ЗАДАЧ НА ГЕНЕРАЦИЮ (позиции)
export std::priority_queue<ChunkGenerationTask> generationQueue;
export ChunkSet pendingGeneration; // Чтобы не добавлять дубликаты
export std::mutex generationMutex;
export std::condition_variable generationCV;

// 2. Очередь ГОТОВЫХ ДАННЫХ (сгенерированные чанки ждут отправки в ChunkMap)
export std::queue<std::shared_ptr<Chunk>> voxelDataQueue;
export std::mutex voxelDataMutex;

// 3. Очередь НА ВЫГРУЗКУ (позиции для удаления)
export std::queue<glm::ivec3> unloadQueue;
export std::mutex unloadMutex;

// 4. Состояние
export std::atomic<bool> running{true};
export glm::ivec3 currentPlayerChunk;
export std::mutex playerPosMutex;

export constexpr int SEA_LEVEL = 16;

// --- Функции ---

// Генерирует воксельные данные (возвращает готовый чанк)
std::shared_ptr<Chunk> generateChunkData(const glm::ivec3& chunkPos);

// Поток-работник: берет позицию из generationQueue -> генерирует -> в voxelDataQueue
export void chunkWorker();

// Поток-поисковик: ищет, какие чанки загрузить/выгрузить, обновляет очереди
// Принимает ссылку на ТЕКУЩУЮ карту чанков для проверки существования
export void chunkFinder(const ChunkMap& chunks);

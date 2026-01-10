//
// Created by aspadien on 15.10.2025.
//

#ifndef UNTITLED2_CHUNKGENERATIONMANAGER_H
#define UNTITLED2_CHUNKGENERATIONMANAGER_H
// --- START OF FILE ChunkGenerationManager.h.txt ---
#include <condition_variable>
#include <queue>
#include <atomic>
#include <mutex>
#include <memory>
#include <glm/vec3.hpp>


// Подключаем ChunkManager.hpp, так как там определен struct Chunk, ChunkMap и ChunkSet
#include "Chunk.hpp"

// Если FastNoise используется только в .cpp, лучше перенести инклюды туда,
// чтобы ускорить компиляцию. Здесь оставим только forward declaration, если нужно.
// #include "../Libs/FastNoise2-master/include/FastSIMD/FunctionList.h"
// #include "../Libs/FastNoise2-master/include/FastNoise/FastNoise.h"

// Структура задачи для генерации
struct ChunkGenerationTask {
    glm::ivec3 chunkPos;
    float distToPlayerSq; // Дистанция в квадрате

    // Приоритетная очередь: min dist -> top
    bool operator<(const ChunkGenerationTask& other) const {
        return distToPlayerSq > other.distToPlayerSq;
    }
};

// --- Глобальные очереди и синхронизация ---
// (Используем inline для глобальных переменных в заголовке, стандарт C++17)

// 1. Очередь ЗАДАЧ НА ГЕНЕРАЦИЮ (позиции)
inline std::priority_queue<ChunkGenerationTask> generationQueue;
inline ChunkSet pendingGeneration; // Чтобы не добавлять дубликаты
inline std::mutex generationMutex;
inline std::condition_variable generationCV;

// 2. Очередь ГОТОВЫХ ДАННЫХ (сгенерированные чанки ждут отправки в ChunkMap)
inline std::queue<std::shared_ptr<Chunk>> voxelDataQueue;
inline std::mutex voxelDataMutex;

// 3. Очередь НА ВЫГРУЗКУ (позиции для удаления)
inline std::queue<glm::ivec3> unloadQueue;
inline std::mutex unloadMutex;

// 4. Состояние
inline std::atomic<bool> running{true};
inline glm::ivec3 currentPlayerChunk;
inline std::mutex playerPosMutex;

constexpr int SEA_LEVEL = 16;

// --- Функции ---

// Генерирует воксельные данные (возвращает готовый чанк)
std::shared_ptr<Chunk> generateChunkData(const glm::ivec3& chunkPos);

// Поток-работник: берет позицию из generationQueue -> генерирует -> в voxelDataQueue
void chunkWorker();

// Поток-поисковик: ищет, какие чанки загрузить/выгрузить, обновляет очереди
// Принимает ссылку на ТЕКУЩУЮ карту чанков для проверки существования
void chunkFinder(const ChunkMap& chunks);

#endif //UNTITLED2_CHUNKGENERATIONMANAGER_H


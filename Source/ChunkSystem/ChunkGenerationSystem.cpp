//
// Created by aspadien on 15.10.2025.
//

module;

#include <algorithm>
#include <FastNoise/FastNoise.h>
#include <mutex>
#include <thread>
#include <memory>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include "../../Definitions/Core/Config.h"
#include "../../Definitions/Core/Constants.hpp"

import ChunkAllocator;
import Chunk;

module ChunkGenerationSystem;

// --- 1. Исправленная инициализация FastNoise (Singleton) ---
FastNoise::SmartNode<FastNoise::FractalFBm> GetTerrainNoise() {
    static auto noise = [] {
        auto fractal = FastNoise::New<FastNoise::FractalFBm>();
        auto simplex = FastNoise::New<FastNoise::Simplex>();

        fractal->SetSource(simplex);
        fractal->SetOctaveCount(4);
        fractal->SetLacunarity(2.0f);
        fractal->SetGain(0.5f);

        return fractal;
    }();
    return noise;
}

// Вспомогательная функция линейной интерполяции (LERP)
inline float lerp(float a, float b, float t) {
    return a + t * (b - a);
}

// ... (Функция GetTerrainNoise и lerp остаются теми же) ...

std::shared_ptr<Chunk> generateChunkData(const glm::ivec3& chunkPos) {
    if (chunkPos.y * CHUNK_SIZE_Y > MAX_TERRAIN_HEIGHT) {
        auto emptyChunk = std::make_shared<Chunk>(chunkPos);
        emptyChunk->blocks = ChunkAllocator::Get().Allocate();
        std::fill_n(emptyChunk->blocks, CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z, BLOCK_AIR);
        return emptyChunk;
    }

    auto newChunk = std::make_shared<Chunk>(chunkPos);
    newChunk->blocks = ChunkAllocator::Get().Allocate();

    // --- НАСТРОЙКИ РАЗМЕРОВ (ВАЖНОЕ ИЗМЕНЕНИЕ) ---
    // X, Z: 17 точек * шаг 2 = 32 единицы (индексы 0..31) - Идеально для ширины чанка.
    // Y:    18 точек * шаг 2 = 34 единицы (индексы 0..33) - Покрывает нужный нам 32-й слой.
    const int LR_XZ = 17;
    const int LR_Y  = 18; // Было 17, стало 18

    // Размеры полного массива (High Res)
    const int HR_X = 32;
    const int HR_Y = 33; // Нам нужно минимум 33 уровня (0..32)
    const int HR_Z = 32;

    // --- ПАМЯТЬ ---
    thread_local std::vector<float> lowResNoise(LR_XZ * LR_Y * LR_XZ);
    if (lowResNoise.size() < LR_XZ * LR_Y * LR_XZ) lowResNoise.resize(LR_XZ * LR_Y * LR_XZ);

    thread_local std::vector<float> highResNoise(HR_X * HR_Y * HR_Z);
    if (highResNoise.size() < HR_X * HR_Y * HR_Z) highResNoise.resize(HR_X * HR_Y * HR_Z);

    // --- ГЕНЕРАЦИЯ ШУМА ---
    GetTerrainNoise()->GenUniformGrid3D(
        lowResNoise.data(),
        chunkPos.x * (LR_XZ - 1), // startX
        chunkPos.y * (LR_XZ - 1), // startY (тут множитель 16, как по X/Z, чтобы чанки стыковались)
        chunkPos.z * (LR_XZ - 1), // startZ
        LR_XZ, LR_Y, LR_XZ,       // size: Y теперь 18
        0.004f * 2.0f,            // freq
        1773
    );

    // --- UPSCALING (ИНТЕРПОЛЯЦИЯ) ---
    // i, k идут до 16. j (по Y) идет до 17.
    for (int k = 0; k < LR_XZ - 1; ++k) {     // Z
        for (int j = 0; j < LR_Y - 1; ++j) {  // Y (теперь сделает 17 итераций)
            for (int i = 0; i < LR_XZ - 1; ++i) { // X

                // Индексы LowRes (X -> Y -> Z)
                // Y размерность теперь LR_Y (18), а не 17!
                int idx000 = (i)     + (j)     * LR_XZ + (k)     * (LR_XZ * LR_Y);
                int idx100 = (i + 1) + (j)     * LR_XZ + (k)     * (LR_XZ * LR_Y);
                int idx010 = (i)     + (j + 1) * LR_XZ + (k)     * (LR_XZ * LR_Y);
                int idx110 = (i + 1) + (j + 1) * LR_XZ + (k)     * (LR_XZ * LR_Y);

                int strideZ = LR_XZ * LR_Y; // Смещение на один шаг по Z в LowRes
                int idx001 = idx000 + strideZ;
                int idx101 = idx100 + strideZ;
                int idx011 = idx010 + strideZ;
                int idx111 = idx110 + strideZ;

                float v000 = lowResNoise[idx000]; float v100 = lowResNoise[idx100];
                float v010 = lowResNoise[idx010]; float v110 = lowResNoise[idx110];
                float v001 = lowResNoise[idx001]; float v101 = lowResNoise[idx101];
                float v011 = lowResNoise[idx011]; float v111 = lowResNoise[idx111];

                int xBase = i * 2;
                int yBase = j * 2;
                int zBase = k * 2;

                for (int zOff = 0; zOff < 2; zOff++) {
                    int hZ = zBase + zOff;
                    if (hZ >= HR_Z) continue;

                    float tZ = zOff * 0.5f;
                    float c00 = lerp(v000, v001, tZ);
                    float c10 = lerp(v100, v101, tZ);
                    float c01 = lerp(v010, v011, tZ);
                    float c11 = lerp(v110, v111, tZ);

                    for (int yOff = 0; yOff < 2; yOff++) {
                        int hY = yBase + yOff;
                        if (hY >= HR_Y) continue; // Защита (до 33)

                        float tY = yOff * 0.5f;
                        float c0 = lerp(c00, c01, tY);
                        float c1 = lerp(c10, c11, tY);

                        for (int xOff = 0; xOff < 2; xOff++) {
                            // X интерполяция
                            float val = lerp(c0, c1, xOff * 0.5f);

                            // Запись (X -> Y -> Z)
                            // Размерность по Y в HighRes = HR_Y = 33!
                            // Индекс: x + y*32 + z*32*33
                            int hIdx = (xBase + xOff) + hY * HR_X + hZ * HR_X * HR_Y;
                            highResNoise[hIdx] = val;
                        }
                    }
                }
            }
        }
    }

    // --- ГЕНЕРАЦИЯ БЛОКОВ ---
    const float DENSITY_THRESHOLD = 0.0f;
    const int DIRT_THICKNESS = 3;
    const float GRADIENT_STEP = 1.0f / 64.0f;
    const int CHUNK_Y = 32;
    int startY = chunkPos.y * CHUNK_Y;

    // Страйды для HighRes массива
    const int NR_STRIDE_Y = 32;
    const int NR_STRIDE_Z = 32 * 33;

    // Временные буферы
    float rowDensitiesAbove[32];
    uint8_t rowDirtDepth[32];

    for (int z = 0; z < 32; ++z) {
        float* noiseZPtr = &highResNoise[z * NR_STRIDE_Z];
        uint8_t* blockZPtr = newChunk->blocks + (z * 32 * 32);

        // 1. Инициализация по Y=32
        // ТЕПЕРЬ ТУТ ГАРАНТИРОВАННО ВАЛИДНЫЕ ДАННЫЕ
        {
            float* noiseTopRow = noiseZPtr + (32 * NR_STRIDE_Y);
            float worldYTop = (float)(startY + CHUNK_Y);
            float gradientTop = (SEA_LEVEL - worldYTop) * GRADIENT_STEP;

            for (int x = 0; x < 32; ++x) {
                rowDensitiesAbove[x] = noiseTopRow[x] + gradientTop;
                rowDirtDepth[x] = 0;
            }
        }

        // 2. Цикл вниз
        for (int y = CHUNK_Y - 1; y >= 0; --y) {
            float* noiseRowPtr = noiseZPtr + (y * NR_STRIDE_Y);
            uint8_t* blockRowPtr = blockZPtr + (y * 32);
            float worldY = (float)(startY + y);
            float rowGradient = (SEA_LEVEL - worldY) * GRADIENT_STEP;

            for (int x = 0; x < 32; ++x) {
                float currentDensity = noiseRowPtr[x] + rowGradient;
                float densityUp = rowDensitiesAbove[x];

                uint8_t blockType;
                if (currentDensity > DENSITY_THRESHOLD) {
                    // Если сверху пусто -> это трава
                    if (densityUp <= DENSITY_THRESHOLD) {
                        blockType = BLOCK_GRASS;
                        rowDirtDepth[x] = DIRT_THICKNESS;
                    }
                    // Если сверху блок и есть запас земли -> земля
                    else if (rowDirtDepth[x] > 0) {
                        blockType = BLOCK_DIRT;
                        rowDirtDepth[x]--;
                    }
                    // Иначе камень
                    else {
                        blockType = BLOCK_STONE;
                    }
                } else {
                    blockType = BLOCK_AIR;
                    rowDirtDepth[x] = 0;
                }

                blockRowPtr[x] = blockType;
                rowDensitiesAbove[x] = currentDensity;
            }
        }
    }

    return newChunk;
}


void chunkWorker() {
    while (running) {
        ChunkGenerationTask task{};

        // 1. Быстро берем задачу и освобождаем лок очереди
        {
            // unique_lock работает со SpinLock
            std::unique_lock<std::mutex> lock(generationMutex);

            // Ждем с помощью condition_variable_any
            generationCV.wait(lock, [] { return !generationQueue.empty() || !running; });

            if (!running && generationQueue.empty()) break;

            task = generationQueue.top();
            generationQueue.pop();
        }

        // 2. Проверка актуальности
        glm::ivec3 currentPlPos;
        {
            std::lock_guard<std::mutex> lock(playerPosMutex);
            currentPlPos = currentPlayerChunk;
        }

        auto dx = static_cast<float>(task.chunkPos.x - currentPlPos.x);
        auto dz = static_cast<float>(task.chunkPos.z - currentPlPos.z);
        float distSq = dx*dx + dz*dz;
        float cancelRadius = static_cast<float>(renderDistanceXZ + 2);

        if (distSq > cancelRadius * cancelRadius) {
            pendingGeneration.erase(task.chunkPos);
            continue;
        }

        // 3. Тяжелая работа
        std::shared_ptr<Chunk> newChunk = generateChunkData(task.chunkPos);

        // 4. Удаляем из "ожидающих"
        pendingGeneration.erase(task.chunkPos);

        // 5. Отправляем результат
        {
            std::lock_guard<std::mutex> lock(voxelDataMutex);
            voxelDataQueue.push(newChunk);
        }
    }
}

std::vector<glm::ivec3> precomputedSpiralOffsets;

void InitSpiralOffsets(int radius, int height) {
    precomputedSpiralOffsets.clear();
    precomputedSpiralOffsets.reserve((radius * 2 + 1) * (radius * 2 + 1) * (height * 2 + 1));

    for (int x = -radius; x <= radius; ++x) {
        for (int z = -radius; z <= radius; ++z) {
            for (int y = -height; y <= height; ++y) {
                precomputedSpiralOffsets.push_back(glm::ivec3(x, y, z));
            }
        }
    }

    std::sort(precomputedSpiralOffsets.begin(), precomputedSpiralOffsets.end(),
        [](const glm::ivec3& a, const glm::ivec3& b) {
            int distA = a.x*a.x + a.z*a.z;
            int distB = b.x*b.x + b.z*b.z;
            if (distA != distB) return distA < distB;
            return std::abs(a.y) < std::abs(b.y);
        });
}


// ==========================================
// 5. Chunk Finder (На SpinLock)
// ==========================================

void chunkFinder(const ChunkMap& chunks) {
    if (precomputedSpiralOffsets.empty()) {
        InitSpiralOffsets(renderDistanceXZ, renderHeightY);
    }

    size_t spiralIndex = 0;
    glm::ivec3 lastPlayerPos = glm::ivec3(999999);

    while (running) {
        glm::ivec3 playerPos;
        {
            std::lock_guard<std::mutex> lock(playerPosMutex);
            playerPos = currentPlayerChunk;
        }

        if (playerPos != lastPlayerPos) {
            spiralIndex = 0;
            lastPlayerPos = playerPos;
        }

        size_t queueSize;
        {
            std::lock_guard<std::mutex> lock(generationMutex);
            queueSize = generationQueue.size();
        }

        if (queueSize > 400) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }

        // --- БЫСТРЫЙ ПОИСК ---
        int tasksAdded = 0;
        const int BATCH_LIMIT = 100;

        for (; spiralIndex < precomputedSpiralOffsets.size(); ++spiralIndex) {
            if (tasksAdded >= BATCH_LIMIT) break;

            glm::ivec3 offset = precomputedSpiralOffsets[spiralIndex];
            glm::ivec3 targetPos = playerPos + offset;

            if (pendingGeneration.contains(targetPos)) continue;
            if (chunks.contains(targetPos)) continue;

            {
                std::lock_guard<std::mutex> lock(generationMutex);
                if (pendingGeneration.count(targetPos) == 0) {
                    pendingGeneration.insert(targetPos);
                    float distSq = (float)(offset.x*offset.x + offset.z*offset.z);
                    generationQueue.push({ targetPos, distSq });
                    tasksAdded++;
                }
            }
        }

        if (spiralIndex >= precomputedSpiralOffsets.size()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        } else if (tasksAdded > 0) {
            generationCV.notify_all();
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        } else {
             std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }

        // --- ВЫГРУЗКА ---
        std::vector<glm::ivec3> toUnload;
        auto chunkList = chunks.getSnapshot();
        for (const auto& pair : chunkList) {
            const glm::ivec3& chunkPos = pair.first;

            int dist_x = abs(chunkPos.x - playerPos.x);
            int dist_z = abs(chunkPos.z - playerPos.z);
            int dist_y = abs(chunkPos.y - playerPos.y);

            int horizontal_dist = std::max(dist_x, dist_z);

            if (horizontal_dist > renderDistanceXZ + 2 || dist_y > renderHeightY + 2) {
                toUnload.push_back(chunkPos);
            }
        }

        if (!toUnload.empty()) {
            std::lock_guard<std::mutex> lock(unloadMutex);
            for(const auto& pos : toUnload) {
                if (chunks.tryGet(pos)) {
                    unloadQueue.push(pos);
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

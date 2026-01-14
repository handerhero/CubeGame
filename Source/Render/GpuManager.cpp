module;
#include <algorithm>
#include <cstring>
#include <iostream>
#include <memory>
#include <glm/vec3.hpp>
#include <vector>

#include "../../Definitions/Core/Config.h"
#include "glad/glad.h"
import VramAllocator;
import Chunk;
module GpuManager;

GpuManager::GpuManager(const int maxDist, const int maxHeight) {
    allocator = std::make_unique<VRamAllocator>(MAX_VERTEX_BUFFER_SIZE);

    int diameter = (maxDist * 2) + 5;
    maxChunksCapacity = diameter * diameter * maxHeight * 1.1f;

    // --- ИЗМЕНЕНИЕ 1: Все буферы теперь Dynamic Storage (без маппинга) ---
    GLbitfield flags = GL_DYNAMIC_STORAGE_BIT;

    // Vertex SSBO
    glCreateBuffers(1, &vertexSSBO);
    glNamedBufferStorage(vertexSSBO, MAX_VERTEX_BUFFER_SIZE * sizeof(uint32_t), nullptr, flags);

    // Command Buffer
    glCreateBuffers(1, &indirectContext.commandBuffer);
    glNamedBufferStorage(indirectContext.commandBuffer,
        static_cast<GLsizeiptr>(maxChunksCapacity * sizeof(DrawArraysIndirectCommand)),
        nullptr, flags);

    // Parameter Buffer
    glCreateBuffers(1, &parameterBuffer);
    glNamedBufferStorage(parameterBuffer, sizeof(uint32_t), nullptr, flags);

    // Info Buffer (Метаданные) - ТЕПЕРЬ ТОЖЕ DYNAMIC
    glCreateBuffers(1, &chunkInfoBuffer);
    glNamedBufferStorage(chunkInfoBuffer, static_cast<GLsizeiptr>(maxChunksCapacity * sizeof(ChunkMetadata)), nullptr, flags);

    // mappedChunksInfos УДАЛЯЕМ. Мы больше не пишем напрямую в память.

    // Инициализация пула индексов
    freeChunkMetadataIndicesList.reserve(maxChunksCapacity);
    for (int i = maxChunksCapacity - 1; i >= 0; --i) freeChunkMetadataIndicesList.push_back(i);

    glGenVertexArrays(1, &vao);
}

GpuManager::~GpuManager() {
    // Unmap больше не нужен
    glDeleteBuffers(1, &vertexSSBO);
    glDeleteBuffers(1, &indirectContext.commandBuffer);
    glDeleteBuffers(1, &parameterBuffer);
    glDeleteBuffers(1, &chunkInfoBuffer);
    glDeleteVertexArrays(1, &vao);
}

void GpuManager::syncMemory() {
    // Ждем завершения ВСЕХ команд update buffer
    glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
}

int GpuManager::allocateChunkMetadataIndex() {
    if (freeChunkMetadataIndicesList.empty()) return -1;
    int i = freeChunkMetadataIndicesList.back();
    freeChunkMetadataIndicesList.pop_back();
    return i;
}

void GpuManager::recycleZombies() {
    globalFrameCounter++;

    // 1. Чистим индексы метаданных
    auto it = zombies.begin();
    while (it != zombies.end()) {
        if (globalFrameCounter >= it->frameOfDeath + BUFFER_FRAMES) {
            // Возвращаем индекс в пул только когда GPU точно закончил с ним
            freeChunkMetadataIndicesList.push_back(it->index);
            it = zombies.erase(it);
        } else {
            ++it;
        }
    }

    // 2. Чистим память вершин
    auto memIt = memoryZombies.begin();
    while (memIt != memoryZombies.end()) {
        if (globalFrameCounter >= memIt->frameOfDeath + BUFFER_FRAMES) {
            // Безопасно освобождаем память в аллокаторе
            allocator->free(memIt->offset, memIt->size);
            memIt = memoryZombies.erase(memIt);
        } else {
            ++memIt;
        }
    }
}

void GpuManager::freeChunk(Chunk* chunk) {
    if (!chunk || !chunk->renderInfo) return;

    auto* info = static_cast<ChunkMetadata*>(chunk->renderInfo);
    int idxBeingFreed = info->number;

    if (info->instanceCount > 0) {
        memoryZombies.push_back({ info->first, info->instanceCount * 2, globalFrameCounter });
    }

    if (idxBeingFreed != -1) {
        // --- ИЗМЕНЕНИЕ 2: Скрываем чанк через команду ---
        // Создаем временную структуру или просто пишем 0 в поле instanceCount
        // Но проще перезаписать структуру целиком нулями или только instanceCount.
        // Для скорости запишем только instanceCount (offset 12 байт: X,Y,Z - int, int, int)

        uint32_t zero = 0;
        // ChunkMetadata: int X, int Y, int Z, uint instanceCount...
        // instanceCount находится по смещению 12 байт (3 * 4)
        glNamedBufferSubData(chunkInfoBuffer,
                             idxBeingFreed * sizeof(ChunkMetadata) + 12,
                             sizeof(uint32_t),
                             &zero);

        zombies.push_back({ idxBeingFreed, globalFrameCounter });
    }

    delete info;
    chunk->renderInfo = nullptr;
}

void GpuManager::processPendingUpdates() {
    auto it = pendingUpdates.begin();
    while (it != pendingUpdates.end()) {
        // Если задача была аннулирована в freeChunk (chunk == nullptr), просто удаляем её из очереди
        if (it->chunk == nullptr) {
            it = pendingUpdates.erase(it);
            continue;
        }

        // Если время пришло
        if (globalFrameCounter >= it->targetFrame) {
            ChunkMetadata& gpuInfo = mappedChunksInfos[it->metaIndex];

            // ДВОЙНАЯ ПРОВЕРКА: Убеждаемся, что чанк все еще владеет этим индексом
            // (защита от багов переиспользования памяти)
            ChunkMetadata* cpuInfo = static_cast<ChunkMetadata*>(it->chunk->renderInfo);
            if (cpuInfo->number == it->metaIndex) {

                // 1. Обновляем CPU
                cpuInfo->first = it->newOffset;
                cpuInfo->instanceCount = it->newSize / 2;

                // 2. Обновляем GPU
                // Позицию X,Y,Z не трогаем, она уже верная с момента uploadChunk
                gpuInfo.first = it->newOffset;

                // Барьер для компилятора
                std::atomic_thread_fence(std::memory_order_release);

                gpuInfo.instanceCount = it->newSize / 2; // Включаем меш

                // 3. Старую память в зомби
                if (it->oldSize > 0) {
                    memoryZombies.push_back({ it->oldOffset, it->oldSize, globalFrameCounter });
                }
            }
            else {
                // Это аварийный случай: индекс не совпадает.
                // Значит чанк был пересоздан криво. Чистим новую память.
                if (it->newSize > 0) allocator->free(it->newOffset, it->newSize);
            }

            it = pendingUpdates.erase(it);
        } else {
            ++it;
        }
    }
}

void GpuManager::uploadChunk(Chunk* chunk, const std::vector<uint32_t>& meshData) {
    uint32_t totalUints = meshData.size();

    // 1. Загрузка Геометрии
    uint32_t newOffset = 0;
    if (totalUints > 0) {
        uint32_t alignedSize = (totalUints + 3) & ~3;
        newOffset = allocator->allocate(alignedSize);

        if (newOffset == UINT32_MAX) {
            std::cerr << "VRAM Full!" << std::endl;
            return;
        }

        // КОМАНДА 1: Загрузить вершины
        glNamedBufferSubData(vertexSSBO,
                             newOffset * sizeof(uint32_t),
                             totalUints * sizeof(uint32_t),
                             meshData.data());
    }

    // 2. Подготовка Метаданных (CPU)
    ChunkMetadata* info = nullptr;
    int metaIdx = -1;

    if (chunk->renderInfo) {
        info = static_cast<ChunkMetadata*>(chunk->renderInfo);
        metaIdx = info->number;
        if (info->instanceCount > 0) {
            memoryZombies.push_back({ info->first, info->instanceCount * 2, globalFrameCounter });
        }
    } else {
        metaIdx = allocateChunkMetadataIndex();
        if (metaIdx == -1) {
            if (totalUints > 0) allocator->free(newOffset, totalUints);
            return;
        }
        chunk->renderInfo = new ChunkMetadata();
        info = static_cast<ChunkMetadata*>(chunk->renderInfo);
        info->number = metaIdx;
    }

    // Обновляем CPU структуру
    info->X = chunk->worldPosition.x;
    info->Y = chunk->worldPosition.y;
    info->Z = chunk->worldPosition.z;
    info->first = newOffset;
    info->instanceCount = totalUints / 2;
    // pad1, pad2 не важны, но лучше обнулить при создании

    // --- ИЗМЕНЕНИЕ 3: Загрузка Метаданных через Команду ---
    // Теперь мы отправляем метаданные в ТУ ЖЕ очередь, что и вершины.
    // OpenGL гарантирует: КОМАНДА 1 завершится до того, как данные КОМАНДЫ 2 будут использованы
    // (при условии правильного барьера в RenderFrame).
    // А самое главное: порядок поступления в GPU строгий.

    ChunkMetadata gpuData = *info; // Копия структуры для отправки

    glNamedBufferSubData(chunkInfoBuffer,
                         metaIdx * sizeof(ChunkMetadata),
                         sizeof(ChunkMetadata),
                         &gpuData);
}
// ... (PushGreedyQuad, VoxelContext, BuildChunkMesh остаются без изменений) ...

// === CPU MESHER IMPLEMENTATION ===
// Упаковка данных в 64 бита
// [X:5][Y:5][Z:5][Face:3][W:5][H:5][BlockID:8][Unused:28]
inline void PushGreedyQuad(std::vector<uint32_t>& data, int x, int y, int z, int face, int w, int h, uint8_t blockId) {
    uint64_t q = 0;
    q |= (uint64_t(x) & 31);             // 0..4
    q |= (uint64_t(y) & 31) << 5;        // 5..9
    q |= (uint64_t(z) & 31) << 10;       // 10..14
    q |= (uint64_t(face) & 7) << 15;     // 15..17

    // Width и Height (храним val-1)
    q |= (uint64_t(w - 1) & 31) << 18;   // 18..22
    q |= (uint64_t(h - 1) & 31) << 23;   // 23..27

    // BlockID (8 бит)
    q |= (uint64_t(blockId)) << 28;      // 28..35

    // Осталось 28 бит свободными (36..63)

    data.push_back(static_cast<uint32_t>(q & 0xFFFFFFFF));
    data.push_back(static_cast<uint32_t>(q >> 32));
}

// Вспомогательный класс для доступа к вокселям с учетом соседей
struct VoxelContext {
    const uint8_t* center;
    const uint8_t* neighbors[6];

    // Безопасное получение блока (с учетом выхода за границы 0..31)
     [[nodiscard]] uint8_t get(int x, int y, int z) const {
        if (x >= 0 && x < 32 && y >= 0 && y < 32 && z >= 0 && z < 32) {
            return center[z * 1024 + y * 32 + x];
        }

        // Логика соседей
        if (x < 0)  return neighbors[5] ? neighbors[5][z * 1024 + y * 32 + (x + 32)] : 0; // Left
        if (x > 31) return neighbors[4] ? neighbors[4][z * 1024 + y * 32 + (x - 32)] : 0; // Right
        if (y < 0)  return neighbors[3] ? neighbors[3][z * 1024 + (y + 32) * 32 + x] : 0; // Bottom
        if (y > 31) return neighbors[2] ? neighbors[2][z * 1024 + (y - 32) * 32 + x] : 0; // Top
        if (z < 0)  return neighbors[1] ? neighbors[1][(z + 32) * 1024 + y * 32 + x] : 0; // Back
        if (z > 31) return neighbors[0] ? neighbors[0][(z - 32) * 1024 + y * 32 + x] : 0; // Front

        return 0; // Недостижимо при радиусе 1
    }
};

// VoxelContext, который копирует данные в плоский массив 34x34x34
// Это убирает ВСЕ проверки границ из внутреннего цикла.
struct FastVoxelContext {
    // 34 * 34 * 34 = 39304 байт (помещается в L1 кэш процессора!)
    uint8_t data[34 * 34 * 34];

    FastVoxelContext(const Chunk* center, const ChunkMap& map) {
        // Очищаем нулями (воздух)
        std::memset(data, 0, sizeof(data));

        // Копируем центр (32x32x32) в середину (смещение 1,1,1)
        if (center && center->blocks) {
            for (int z = 0; z < 32; ++z) {
                for (int y = 0; y < 32; ++y) {
                    // Копируем строку X целиком (очень быстро, memcpy)
                    std::memcpy(
                        &data[idx(1, y + 1, z + 1)],
                        &center->blocks[x_y_z_to_idx(0, y, z)], // Предполагаем ваш индекс x + y*32 + z*32*32
                        32
                    );
                }
            }
        }

        // Копируем соседей (только граничные слои)
        // Пример для соседа +X (offset 1, 0, 0)
        // Нам нужен его слой x=0, чтобы положить в наш буфер x=33
        // ... (код копирования границ опущен для краткости, но суть понятна) ...
        // ЛУЧШЕ: Просто копируйте блоки как есть, если `ctx.get` старый был сложный.
        // Но даже без соседей (если просто 0 на границе) будет быстрее.
        // Для полной корректности нужно скопировать 6 граней соседей.

        fillNeighbors(center->worldPosition, map);
    }

    // Индекс в массиве 34x34x34
    // x, y, z от 0 до 33
    inline int idx(int x, int y, int z) const {
        return x + y * 34 + z * 34 * 34;
    }

    // Получение без проверок if (быстро!)
    // Входные координаты 0..31 транслируются в 1..32
    inline uint8_t get(int x, int y, int z) const {
        return data[idx(x + 1, y + 1, z + 1)];
    }

    // Вспомогательная для копирования
    void fillNeighbors(glm::ivec3 pos, const ChunkMap& map) {
        // Пример: Сосед +X (East)
        if (auto ptr = map.tryGet(pos + glm::ivec3(1, 0, 0))) {
            const uint8_t* nb = ptr->blocks;
            for(int z=0; z<32; ++z)
                for(int y=0; y<32; ++y)
                    data[idx(33, y+1, z+1)] = nb[0 + y*32 + z*1024]; // Берем x=0 у соседа
        }
        // Сосед -X (West)
        if (auto ptr = map.tryGet(pos + glm::ivec3(-1, 0, 0))) {
             const uint8_t* nb = ptr->blocks;
             for(int z=0; z<32; ++z)
                for(int y=0; y<32; ++y)
                    data[idx(0, y+1, z+1)] = nb[31 + y*32 + z*1024]; // Берем x=31 у соседа
        }

        // ... (аналогично для Y и Z) ...
        // Y+
        if (auto ptr = map.tryGet(pos + glm::ivec3(0, 1, 0))) {
             const uint8_t* nb = ptr->blocks;
             for(int z=0; z<32; ++z)
                std::memcpy(&data[idx(1, 33, z+1)], &nb[0 + 0*32 + z*1024], 32);
        }
        // Y-
        if (auto ptr = map.tryGet(pos + glm::ivec3(0, -1, 0))) {
             const uint8_t* nb = ptr->blocks;
             for(int z=0; z<32; ++z)
                std::memcpy(&data[idx(1, 0, z+1)], &nb[0 + 31*32 + z*1024], 32);
        }

        // Z+
        if (auto ptr = map.tryGet(pos + glm::ivec3(0, 0, 1))) {
             const uint8_t* nb = ptr->blocks;
             for(int y=0; y<32; ++y)
                std::memcpy(&data[idx(1, y+1, 33)], &nb[0 + y*32 + 0*1024], 32);
        }
        // Z-
        if (auto ptr = map.tryGet(pos + glm::ivec3(0, 0, -1))) {
             const uint8_t* nb = ptr->blocks;
             for(int y=0; y<32; ++y)
                std::memcpy(&data[idx(1, y+1, 0)], &nb[0 + y*32 + 31*1024], 32);
        }
    }

    // Хелпер для индексов исходного массива
     static int x_y_z_to_idx(const int x, const int y, const int z) { return x + y*32 + z*32*32; }
};
// ----------------------------------------------------------------------------
// 1. Thread Local Storage (Скратчпад)
// Это самая важная часть. Память выделяется 1 раз на поток и переиспользуется вечно.
// Никаких malloc/free внутри цикла мешинга!
// ----------------------------------------------------------------------------
struct MeshingScratchpad {
    std::vector<uint32_t> outputBuffer;
    // 32*32 = 1024 элемента uint16_t (2 КБ), помещается в L1 Cache.
    // Используем alignas для SIMD оптимизаций компилятора.
    alignas(64) uint16_t mask[1024];

    MeshingScratchpad() {
        outputBuffer.reserve(4096); // Резерв сразу с запасом
    }
};

static thread_local MeshingScratchpad tls;

// ----------------------------------------------------------------------------
// 2. Шаблонная функция мешинга плоскости
// Axis: 0=X, 1=Y, 2=Z.
// Шаблоны позволяют компилятору сгенерировать 3 разные супер-оптимизированные
// функции, где все проверки осей вырезаны на этапе компиляции.
// ----------------------------------------------------------------------------
template <int Axis>
void MeshPlane(const FastVoxelContext& ctx, std::vector<uint32_t>& out, uint16_t* mask) {
    // --- ИСПРАВЛЕНИЕ ТУТ ---
    // Настраиваем оси так, чтобы V (внутренний цикл) всегда был "горизонтальным"
    // Axis 0 (X): U=Y, V=Z. (Сканируем Z, потом Y). OK.
    // Axis 1 (Y): U=Z, V=X. (Сканируем X, потом Z). OK.
    // Axis 2 (Z): Было U=X, V=Y. СТАЛО U=Y, V=X. (Сканируем X, потом Y). FIX!

    constexpr int U = (Axis == 0) ? 1 : (Axis == 1 ? 2 : 1); // Axis 2 теперь берет 1 (Y)
    constexpr int V = (Axis == 0) ? 2 : (Axis == 1 ? 0 : 0); // Axis 2 теперь берет 0 (X)

    for (int faceDir = 0; faceDir < 2; ++faceDir) {
        int faceID;
        if constexpr (Axis == 0) faceID = (faceDir == 0) ? 5 : 4;
        else if constexpr (Axis == 1) faceID = (faceDir == 0) ? 3 : 2;
        else faceID = (faceDir == 0) ? 1 : 0;

        int offset = (faceDir == 0) ? -1 : 1;

        for (int d = 0; d < 32; ++d) {
            int n = 0;

            // --- Pass 1: Заполнение маски ---
            for (int u = 0; u < 32; ++u) {
                for (int v = 0; v < 32; ++v) {
                    int x, y, z;
                    // --- ИСПРАВЛЕНИЕ КООРДИНАТ ТУТ ---
                    // Нам нужно правильно собрать x,y,z обратно из d,u,v
                    if constexpr (Axis == 0)      { x = d; y = u; z = v; }
                    else if constexpr (Axis == 1) { x = v; y = d; z = u; }
                    else                          { x = v; y = u; z = d; } // Axis 2: x=v(X), y=u(Y)

                    uint8_t b = ctx.get(x, y, z);

                    int nx = x + (Axis == 0 ? offset : 0);
                    int ny = y + (Axis == 1 ? offset : 0);
                    int nz = z + (Axis == 2 ? offset : 0);

                    uint8_t neighbor = ctx.get(nx, ny, nz);
                    mask[n++] = b * (neighbor == 0);
                }
            }

            // --- Pass 2: Greedy Meshing ---
            n = 0;
            for (int u = 0; u < 32; ++u) {
                for (int v = 0; v < 32; ) {
                    uint16_t type = mask[n + v];
                    if (type != 0) {
                        int w = 1;
                        while (v + w < 32 && mask[n + v + w] == type) w++;

                        int h = 1;
                        bool done = false;
                        while (u + h < 32) {
                            int rowStart = n + (h * 32) + v;
                            for (int k = 0; k < w; ++k) {
                                if (mask[rowStart + k] != type) { done = true; break; }
                            }
                            if (done) break;
                            h++;
                        }

                        // --- ИСПРАВЛЕНИЕ КООРДИНАТ ДЛЯ PUSH ---
                        int x, y, z;
                        if constexpr (Axis == 0)      { x=d; y=u; z=v; }
                        else if constexpr (Axis == 1) { x=v; y=d; z=u; }
                        else                          { x=v; y=u; z=d; } // Axis 2: x=v(X), y=u(Y)

                        // Важно: w и h теперь соответствуют новым осям.
                        // Для Axis 2: w - это ширина по X, h - высота по Y.
                        // PushGreedyQuad должен принимать это корректно.
                        PushGreedyQuad(out, x, y, z, faceID, w, h, type);

                        for (int l = 0; l < h; ++l) {
                            int rowOffset = n + v + (l * 32);
                            for (int k = 0; k < w; ++k) mask[rowOffset + k] = 0;
                        }
                        v += w;
                    } else {
                        v++;
                    }
                }
                n += 32;
            }
        }
    }
}

// ----------------------------------------------------------------------------
// 3. Основная функция (Точка входа)
// ----------------------------------------------------------------------------
std::vector<uint32_t> BuildChunkMesh(const Chunk* center, const ChunkMap& map) {
    // 1. Очищаем Thread-Local буфер (O(1) - просто сброс счетчика)
    tls.outputBuffer.clear();

    // 2. Создаем контекст (копирование данных в стек/L1)
    FastVoxelContext ctx(center, map);

    // 3. Запускаем шаблоны для каждой оси
    // Код развернется (inlining) в одну большую простыню инструкций без лишних call
    MeshPlane<0>(ctx, tls.outputBuffer, tls.mask); // Axis X
    MeshPlane<1>(ctx, tls.outputBuffer, tls.mask); // Axis Y
    MeshPlane<2>(ctx, tls.outputBuffer, tls.mask); // Axis Z

    // 4. Возвращаем копию данных
    // Мы копируем из thread_local вектора в возвращаемый вектор.
    // Это очень быстрая операция (memcpy), намного быстрее, чем
    // постоянные realloc внутри циклов.
    return tls.outputBuffer;
}
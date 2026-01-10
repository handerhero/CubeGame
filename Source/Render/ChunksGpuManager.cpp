#include "../../Headers/Render/ChunksGpuManager.hpp"
#include <cstring>
#include <algorithm>
#include <iostream>

ChunkGpuManager::ChunkGpuManager(const int maxDist, const int maxHeight) {
    // 1. Аллокатор VRAM (линейный)
    allocator = std::make_unique<VramAllocator>(MAX_VERTEX_BUFFER_SIZE);

    int diameter = (maxDist * 2) + 5;
    maxChunksCapacity = diameter * diameter * maxHeight;

    // Флаги для Persistent Mapping
    GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;

    // 2. Vertex SSBO (Огромный буфер всей геометрии)
    glCreateBuffers(1, &vertexSSBO);
    glNamedBufferStorage(vertexSSBO, MAX_VERTEX_BUFFER_SIZE * sizeof(uint32_t), nullptr, flags);
    mappedVertices = (uint32_t*)glMapNamedBufferRange(vertexSSBO, 0, MAX_VERTEX_BUFFER_SIZE * sizeof(uint32_t), flags);

    // 3. Command Buffer (Массив команд отрисовки)
    // Размер: MaxChunks * sizeof(DrawCommand)
    glCreateBuffers(1, &indirectContext.commandBuffer);
    glNamedBufferStorage(indirectContext.commandBuffer,
        static_cast<GLsizeiptr>(maxChunksCapacity * sizeof(DrawArraysIndirectCommand)),
        nullptr, GL_DYNAMIC_STORAGE_BIT); // DYNAMIC, так как CS пишет туда каждый кадр

    // 4. Parameter Buffer (Атомарный счетчик)
    glCreateBuffers(1, &parameterBuffer);
    glNamedBufferStorage(parameterBuffer, sizeof(uint32_t), nullptr, GL_DYNAMIC_STORAGE_BIT);

    // 5. Info Buffer (Метаданные чанков)
    glCreateBuffers(1, &chunkInfoBuffer);
    glNamedBufferStorage(chunkInfoBuffer, static_cast<GLsizeiptr>(maxChunksCapacity * sizeof(ChunkMetadata)), nullptr, flags);
    mappedChunksInfos = (ChunkMetadata*)(glMapNamedBufferRange(chunkInfoBuffer, 0, static_cast<GLsizeiptr>(maxChunksCapacity * sizeof(ChunkMetadata)), flags));

    // Инициализация пула индексов
    freeChunkMetadataIndicesList.reserve(maxChunksCapacity);
    for (int i = maxChunksCapacity - 1; i >= 0; --i) freeChunkMetadataIndicesList.push_back(i);

    // 6. VAO (Пустой, так как данные берем из SSBO по gl_VertexID)
    glGenVertexArrays(1, &vao);
}

ChunkGpuManager::~ChunkGpuManager() {
    if (vertexSSBO) glUnmapNamedBuffer(vertexSSBO);
    if (chunkInfoBuffer) glUnmapNamedBuffer(chunkInfoBuffer);

    glDeleteBuffers(1, &vertexSSBO);
    glDeleteBuffers(1, &indirectContext.commandBuffer);
    glDeleteBuffers(1, &parameterBuffer); // Не забываем удалить
    glDeleteBuffers(1, &chunkInfoBuffer);
    glDeleteVertexArrays(1, &vao);
}

void ChunkGpuManager::syncMemory() {
    // Барьер для гарантии видимости записей перед запуском Compute/Draw
    glMemoryBarrier(GL_COMMAND_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
}

int ChunkGpuManager::allocateChunkMetadataIndex() {
    if (freeChunkMetadataIndicesList.empty()) return -1;
    int i = freeChunkMetadataIndicesList.back();
    freeChunkMetadataIndicesList.pop_back();
    return i;
}

// Эту функцию нужно добавить
void ChunkGpuManager::recycleZombies() {
    globalFrameCounter++; // Увеличиваем счетчик кадров

    if (zombies.empty()) return;

    // Проходим по списку "мертвых" индексов
    auto it = zombies.begin();
    while (it != zombies.end()) {
        // Если прошло достаточно кадров (BUFFER_FRAMES), индекс можно использовать снова
        // Мы ждем, пока GPU точно закончит использовать этот индекс в предыдущих кадрах
        if (globalFrameCounter >= it->frameOfDeath + BUFFER_FRAMES) {
            freeChunkMetadataIndicesList.push_back(it->index);
            it = zombies.erase(it);
        } else {
            ++it;
        }
    }
}

// ИЗМЕНЕННАЯ функция freeChunk (теперь использует зомби)
void ChunkGpuManager::freeChunk(Chunk* chunk) const {
    if (!chunk || !chunk->renderInfo) return;

    auto* info = static_cast<ChunkMetadata*>(chunk->renderInfo);

    // 1. Память вершин освобождаем сразу (аллокатор сам решит, когда перезаписать)
    if (info->instanceCount > 0) {
        allocator->free(info->first, info->instanceCount * 2);
    }

    // 2. Метаданные на GPU зануляем
    if (info->number != -1) {
        ChunkMetadata& gpuInfo = mappedChunksInfos[info->number];
        gpuInfo.instanceCount = 0;
        gpuInfo.first = 0;
        gpuInfo.X = 0; gpuInfo.Y = 0; gpuInfo.Z = 0;

        // ВМЕСТО мгновенного возврата: freeChunkMetadataIndex(info->number);
        // Добавляем в список зомби. Индекс вернется в пул через 3 кадра.
        zombies.push_back({ (int)info->number, globalFrameCounter });
    }

    delete info;
    chunk->renderInfo = nullptr;
}

void ChunkGpuManager::uploadChunk(Chunk* chunk, const std::vector<uint32_t>& meshData) {
    // 1. Инициализация метаданных CPU
    if (!chunk->renderInfo) {
        int idx = allocateChunkMetadataIndex();
        if (idx == -1) {
            std::cerr << "GPU Info Buffer Full!" << std::endl;
            return;
        }
        chunk->renderInfo = new ChunkMetadata{
            chunk->worldPosition.x, chunk->worldPosition.y, chunk->worldPosition.z,
            0, 0, static_cast<uint32_t>(idx), 0, 0
        };
    }
    auto* info = static_cast<ChunkMetadata*>(chunk->renderInfo);

    // 2. Освобождение старой памяти VRAM (если была)
    if (info->instanceCount > 0) {
        allocator->free(info->first, info->instanceCount * 2);
        info->instanceCount = 0;
        info->first = 0;
    }

    // Ссылка на GPU память
    ChunkMetadata& gpuInfo = mappedChunksInfos[info->number];

    // 3. Если меш пустой
    if (meshData.empty()) {
        gpuInfo.instanceCount = 0;
        gpuInfo.first = 0;
        // Обновляем координаты на всякий случай
        gpuInfo.X = chunk->worldPosition.x;
        gpuInfo.Y = chunk->worldPosition.y;
        gpuInfo.Z = chunk->worldPosition.z;
        return;
    }

    // 4. Аллокация новой памяти
    uint32_t totalUints = meshData.size();
    uint32_t offset = allocator->allocate(totalUints);

    if (offset == UINT32_MAX) {
        std::cerr << "VRAM Full! Cannot upload chunk." << std::endl;
        return;
    }

    // 5. Копирование вершин (memcpy)
    std::memcpy(&mappedVertices[offset], meshData.data(), totalUints * sizeof(uint32_t));

    // 6. Обновление CPU Info
    info->first = offset;
    info->instanceCount = totalUints / 2; // Квады

    // 7. Обновление GPU Info (Compute Shader прочитает это)
    gpuInfo.X = chunk->worldPosition.x;
    gpuInfo.Y = chunk->worldPosition.y;
    gpuInfo.Z = chunk->worldPosition.z;
    gpuInfo.first = offset;
    gpuInfo.instanceCount = totalUints / 2; // Квады
    gpuInfo.number = info->number;
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

std::vector<uint32_t> BuildChunkMesh(const Chunk* center, const ChunkMap& map) {
    std::vector<uint32_t> data;
    data.reserve(2048); // Меньше, т.к. квадов меньше

    // Подготовка контекста
    VoxelContext ctx{};
    ctx.center = center->blocks;
    const glm::ivec3 OFFSETS[6] = { {0,0,1}, {0,0,-1}, {0,1,0}, {0,-1,0}, {1,0,0}, {-1,0,0} };
    for(int i=0; i<6; ++i) {
        auto ptr = map.tryGet(center->worldPosition + OFFSETS[i]);
        ctx.neighbors[i] = ptr ? ptr.get()->blocks : nullptr;
    }

    // Маска для текущего слоя (хранит ID блока или 0)
    // 32x32 = 1024 элемента
    uint16_t mask[32 * 32];

    // Мы проходим 6 граней по отдельности. Это проще для понимания осей.
    // Faces: 0:+Z, 1:-Z, 2:+Y, 3:-Y, 4:+X, 5:-X

    // --- ОСЬ Z (Faces 0 and 1) ---
    // Срез по XY, идем вглубь по Z
    for (int face = 0; face < 2; ++face) {
        bool back = (face == 1);
        int direction = back ? -1 : 1;

        for (int z = 0; z < 32; ++z) {
            // 1. Заполняем маску для слоя Z
            int n = 0;
            for (int y = 0; y < 32; ++y) {
                for (int x = 0; x < 32; ++x) {
                    uint8_t b = ctx.get(x, y, z);
                    uint8_t neighbor = ctx.get(x, y, z + direction);
                    // Грань видна, если блок есть, а соседа в направлении грани нет
                    // (или сосед прозрачный - тут упрощено до 0)
                    mask[n++] = (b != 0 && neighbor == 0) ? b : 0;
                }
            }

            // 2. Жадное объединение (Greedy Meshing)
            n = 0;
            for (int j = 0; j < 32; ++j) {       // Rows (Y)
                for (int i = 0; i < 32; ) {      // Cols (X)
                    uint16_t current = mask[n + i];
                    if (current != 0) {
                        // Вычисляем ширину (W)
                        int w = 1;
                        while (i + w < 32 && mask[n + i + w] == current) {
                            w++;
                        }

                        // Вычисляем высоту (H)
                        int h = 1;
                        bool done = false;
                        while (j + h < 32) {
                            for (int k = 0; k < w; ++k) {
                                if (mask[n + k + h * 32 + i] != current) {
                                    done = true;
                                    break;
                                }
                            }
                            if (done) break;
                            h++;
                        }

                        // Добавляем квад (x, y, z, w, h)
                        // Для оси Z ширина это X, высота это Y
                        PushGreedyQuad(data, i, j, z, face, w, h, current);

                        // Очищаем маску
                        for (int l = 0; l < h; ++l) {
                            for (int k = 0; k < w; ++k) {
                                mask[n + k + l * 32 + i] = 0;
                            }
                        }
                        i += w;
                    } else {
                        i++;
                    }
                }
                n += 32;
            }
        }
    }

    // --- ОСЬ X (Faces 4 and 5) ---
    // Срез по ZY (или YZ), идем вглубь по X
    // Width = Z, Height = Y
    for (int face = 4; face < 6; ++face) {
        bool left = (face == 5);
        int direction = left ? -1 : 1;

        for (int x = 0; x < 32; ++x) {
            int n = 0;
            for (int y = 0; y < 32; ++y) {
                for (int z = 0; z < 32; ++z) {
                    uint8_t b = ctx.get(x, y, z);
                    uint8_t neighbor = ctx.get(x + direction, y, z);
                    mask[n++] = (b != 0 && neighbor == 0) ? b : 0;
                }
            }

            // Greedy
            n = 0;
            for (int j = 0; j < 32; ++j) {      // Y
                for (int i = 0; i < 32; ) {     // Z
                    uint16_t current = mask[n + i];
                    if (current != 0) {
                        int w = 1; while (i + w < 32 && mask[n + i + w] == current) w++;
                        int h = 1; bool done = false;
                        while (j + h < 32) {
                            for (int k = 0; k < w; ++k) if (mask[n + k + h * 32 + i] != current) { done = true; break; }
                            if (done) break; h++;
                        }
                        // Для оси X: i=Z, j=Y. Значит W=Z, H=Y
                        // В PushGreedyQuad порядок аргументов x,y,z всегда мировой.
                        // Здесь x фиксирован циклом. Позиция (x, j, i) -> (x, y, z)
                        // Но стоп. i это Z, j это Y.
                        PushGreedyQuad(data, x, j, i, face, w, h, current);

                        for (int l = 0; l < h; ++l) for (int k = 0; k < w; ++k) mask[n + k + l * 32 + i] = 0;
                        i += w;
                    } else i++;
                }
                n += 32;
            }
        }
    }

    // --- ОСЬ Y (Faces 2 and 3) ---
    // Срез по XZ, идем вглубь по Y
    // Width = X, Height = Z
    for (int face = 2; face < 4; ++face) {
        bool bottom = (face == 3);
        int direction = bottom ? -1 : 1;

        for (int y = 0; y < 32; ++y) {
            int n = 0;
            for (int z = 0; z < 32; ++z) {
                for (int x = 0; x < 32; ++x) {
                    uint8_t b = ctx.get(x, y, z);
                    uint8_t neighbor = ctx.get(x, y + direction, z);
                    mask[n++] = (b != 0 && neighbor == 0) ? b : 0;
                }
            }

            // Greedy
            n = 0;
            for (int j = 0; j < 32; ++j) {      // Z
                for (int i = 0; i < 32; ) {     // X
                    uint16_t current = mask[n + i];
                    if (current != 0) {
                        int w = 1; while (i + w < 32 && mask[n + i + w] == current) w++;
                        int h = 1; bool done = false;
                        while (j + h < 32) {
                            for (int k = 0; k < w; ++k) if (mask[n + k + h * 32 + i] != current) { done = true; break; }
                            if (done) break; h++;
                        }
                        // Для оси Y: i=X, j=Z. W=X, H=Z.
                        PushGreedyQuad(data, i, y, j, face, w, h, current);

                        for (int l = 0; l < h; ++l) for (int k = 0; k < w; ++k) mask[n + k + l * 32 + i] = 0;
                        i += w;
                    } else i++;
                }
                n += 32;
            }
        }
    }

    return data;
}
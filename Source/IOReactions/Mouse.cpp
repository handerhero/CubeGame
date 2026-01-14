module;

#include "glm/glm.hpp"
#include <chrono>
#include "GLFW/glfw3.h"

import Camera;
import ChunkGenerationSystem;
import MathUtils;
import Chunk;

module Mouse;

void MouseInteractions::set(int x, int y, int z, uint8_t block, const std::shared_ptr<Chunk>& chunk) {
    // Важно: координаты должны быть локальными (0-31)
    if (x < 0 || y < 0 || z < 0 || x >= 32 || y >= 32 || z >= 32) return;

    // Убираем const, чтобы можно было менять данные
    Chunk* temp = chunk.get();
    temp->blocks[x + (y << 5) + (z << 10)] = block;
    temp->needsMeshUpdate = false; // Ставим флаг прямо здесь
}


std::shared_ptr<Chunk> MouseInteractions::castRayAndModifyBlockCached(
    GLFWwindow *window,
    Camera &camera,
    ChunkMap &chunks,
    int placeBlockID,
    float maxDist
)
{
    // Проверка нажатия (лучше вынести управление таймером/кликом наружу,
    // иначе будет ломать/строить 60 блоков в секунду)
    bool breakPressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    bool placePressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;

    if (!breakPressed && !placePressed) return nullptr;

    glm::vec3 origin = camera.pos;
    glm::vec3 dir = glm::normalize(camera.camera->forward());

    int bx = static_cast<int>(std::floor(origin.x));
    int by = static_cast<int>(std::floor(origin.y));
    int bz = static_cast<int>(std::floor(origin.z));

    glm::ivec3 chunkIndex(fastFloorDiv(bx, 32), fastFloorDiv(by, 32), fastFloorDiv(bz, 32));
    auto curChunk = chunks.tryGet(chunkIndex);

    // DDA Инициализация
    int stepX = (dir.x > 0.0f) ? 1 : -1;
    int stepY = (dir.y > 0.0f) ? 1 : -1;
    int stepZ = (dir.z > 0.0f) ? 1 : -1;

    float tMaxX = intbound(origin.x, dir.x);
    float tMaxY = intbound(origin.y, dir.y);
    float tMaxZ = intbound(origin.z, dir.z);

    float tDeltaX = (dir.x != 0.0f) ? std::fabs(1.0f / dir.x) : FLT_MAX;
    float tDeltaY = (dir.y != 0.0f) ? std::fabs(1.0f / dir.y) : FLT_MAX;
    float tDeltaZ = (dir.z != 0.0f) ? std::fabs(1.0f / dir.z) : FLT_MAX;

    float traveled = 0.0f;
    int prevBX = bx, prevBY = by, prevBZ = bz;

    while (traveled <= maxDist)
    {
        // Если вышли за пределы чанка, обновляем указатель
        glm::ivec3 newChunkIndex(fastFloorDiv(bx, 32), fastFloorDiv(by, 32), fastFloorDiv(bz, 32));
        if (newChunkIndex != chunkIndex) {
            chunkIndex = newChunkIndex;
            curChunk = chunks.tryGet(chunkIndex);
        }

        // Если чанка нет (пустота), мы не можем взаимодействовать с блоками
        if (!curChunk) break;

        // Локальные координаты
        int localX = fastFloorMod32(bx);
        int localY = fastFloorMod32(by);
        int localZ = fastFloorMod32(bz);

        int block = curChunk->get(localX, localY, localZ);

        // [Ломаем блок]
        if (breakPressed && block != 0)
        {
            set(localX, localY, localZ, 0, curChunk);
            // ВОЗВРАЩАЕМ измененный чанк, чтобы Main Loop его обновил
            return curChunk;
        }

        // [Ставим блок]
        // Ставим в ПРЕДЫДУЩУЮ позицию (prevBX), то есть перед стенкой
        if (placePressed && block != 0)
        {
            glm::ivec3 placeChunkIndex(fastFloorDiv(prevBX, 32), fastFloorDiv(prevBY, 32), fastFloorDiv(prevBZ, 32));
            int placeX = fastFloorMod32(prevBX);
            int placeY = fastFloorMod32(prevBY);
            int placeZ = fastFloorMod32(prevBZ);

            std::shared_ptr<Chunk> targetChunk = curChunk;

            // Если место установки в другом чанке (на границе)
            if (placeChunkIndex != chunkIndex) {
                targetChunk = chunks.tryGet(placeChunkIndex);
                if (!targetChunk) return nullptr; // Нельзя ставить в несуществующий чанк
            }

            // Проверяем, не занято ли место (хотя мы пришли из воздуха, но на всякий случай)
            if (targetChunk->get(placeX, placeY, placeZ) == 0) {
                set(placeX, placeY, placeZ, placeBlockID, targetChunk);
                // ВОЗВРАЩАЕМ именно тот чанк, в который поставили блок
                return targetChunk;
            }
            // Если занято (например, entity), прерываемся
            break;
        }

        // DDA Шаг
        prevBX = bx; prevBY = by; prevBZ = bz;

        if (tMaxX < tMaxY) {
            if (tMaxX < tMaxZ) { bx += stepX; traveled = tMaxX; tMaxX += tDeltaX; }
            else { bz += stepZ; traveled = tMaxZ; tMaxZ += tDeltaZ; }
        } else {
            if (tMaxY < tMaxZ) { by += stepY; traveled = tMaxY; tMaxY += tDeltaY; }
            else { bz += stepZ; traveled = tMaxZ; tMaxZ += tDeltaZ; }
        }
    }

    // Ничего не изменили
    return nullptr;
}


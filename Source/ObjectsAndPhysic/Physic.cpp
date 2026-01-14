module;
#include <cmath>
#include <memory>
#include <vector>
#include <glm/vec3.hpp>

#include "../../Definitions/Core/Config.h"


import Keyboard;
import AABB;
import Mouse;
import Chunk;
import Window;
import Camera;

module Physic;

    Physic::Physic(const Camera &camera) {
        playerAABB = new AABB(camera.pos, 1.8f,0.8f,0.8f);
    }
    Physic::~Physic() {
        delete playerAABB;
    }


    void Physic::makeTick(Keyboard &keyboard_control,std::vector<std::shared_ptr<Chunk>> &batchG, Camera &camera, const double dt,ChunkMap &chunkMap, const Window &window) {

        keyboard_control.keyboardControlNotFree(&camera, window.window,dt,&kineticVector, onGround, legsPower,moveSpeed);
        movePlayer(*playerAABB, kineticVector,dt, onGround,chunkMap );

        if (onGround) {
            kineticVector -= kineticVector * 0.91f * (static_cast<float>(dt/(dt+0.005f)));
        }else {
            kineticVector.x -= kineticVector.x * static_cast<float>(0.021*(dt/(dt+0.005)));
            kineticVector.z -= kineticVector.z * static_cast<float>(0.021*(dt/(dt+0.005)));
            kineticVector.y -= kineticVector.y * static_cast<float>(0.0005*(dt/(dt+0.005)));
        }

        kineticVector.y -= static_cast<float>(dt * gravity);


        camera.pos = playerAABB->getCenter();
        camera.pos.y += playerEyeHeight;

if (const auto temp = MouseInteractions::castRayAndModifyBlockCached(window.window,camera,chunkMap,1,10); temp.get() != nullptr) {
            batchG.push_back(temp);
        }
    }


// двигаем игрока с учётом коллизий
void Physic::movePlayer(AABB& playerAABB, glm::vec3& velocity, const double dt, bool& onGround, ChunkMap &chunks) {
    if (std::isnan(velocity.x)) {
        velocity.x = 0;
    }
    if (std::isnan(velocity.y)) {
        velocity.y = 0;
    }
    if (std::isnan(velocity.z)) {
        velocity.z = 0;
    }
    if (velocity.x > 1000.0f || velocity.x < -1000.0f ) {
        velocity.x = 0;
    }
    if (velocity.y > 1000.0f || velocity.y < -1000.0f ) {
        velocity.y = 0;
    }
    if (velocity.z > 1000.0f || velocity.z < -1000.0f) {
        velocity.z = 0;
    }
    onGround = false;
    glm::vec3 size = playerAABB.max - playerAABB.min;
    glm::vec3 delta = velocity * static_cast<float>(dt);

    // заранее сохраним границы игрока
    float minx = playerAABB.min.x;
    float miny = playerAABB.min.y;
    float minz = playerAABB.min.z;
    float maxx = playerAABB.max.x;
    float maxy = playerAABB.max.y;
    float maxz = playerAABB.max.z;

    // вычисляем диапазон блоков для проверки
    int bx0 = std::floor(minx) -1;
    int by0 = std::floor(miny) -1;
    int bz0 = std::floor(minz) -1;
    int bx1 = std::floor(maxx) +1;
    int by1 = std::floor(maxy) +1;
    int bz1 = std::floor(maxz) +1;

    // --- X ---
    minx += delta.x; maxx += delta.x;
    for (int x = bx0; x <= bx1; x++) {
        for (int y = by0; y <= by1; y++) {
            for (int z = bz0; z <= bz1; z++) {
                if (!isSolidBlock(glm::vec3(x,y,z), chunks)) continue;

                if (checkCollision(minx,miny,minz,maxx,maxy,maxz,
                                   x,y,z, x+1,y+1,z+1))
                {
                    if (delta.x > 0) { maxx = x; minx = maxx - size.x; }
                    else if (delta.x < 0) { minx = x+1; maxx = minx + size.x; }
                    velocity.x = 0.0f;
                }
            }
        }
    }

    // --- Y ---
    miny += delta.y; maxy += delta.y;
    for (int x = bx0; x <= bx1; x++) {
        for (int y = by0; y <= by1; y++) {
            for (int z = bz0; z <= bz1; z++) {
                if (!isSolidBlock(glm::vec3(x,y,z), chunks)) continue;

                if (checkCollision(minx,miny,minz,maxx,maxy,maxz,
                                   x,y,z, x+1,y+1,z+1))
                {
                    if (delta.y > 0) { maxy = y; miny = maxy - size.y; velocity.y = 0; }
                    else if (delta.y < 0) { miny = y+1; maxy = miny + size.y; velocity.y = 0; onGround = true; }
                }
            }
        }
    }

    // --- Z ---
    minz += delta.z; maxz += delta.z;
    for (int x = bx0; x <= bx1; x++) {
        for (int y = by0; y <= by1; y++) {
            for (int z = bz0; z <= bz1; z++) {
                if (!isSolidBlock(glm::vec3(x,y,z), chunks)) continue;

                if (checkCollision(minx,miny,minz,maxx,maxy,maxz,
                                   x,y,z, x+1,y+1,z+1))
                {
                    if (delta.z > 0) { maxz = z; minz = maxz - size.z; }
                    else if (delta.z < 0) { minz = z+1; maxz = minz + size.z; }
                    velocity.z = 0.0f;
                }
            }
        }
    }

    // обновляем AABB игрока
    playerAABB.min = {minx,miny,minz};
    playerAABB.max = {maxx,maxy,maxz};
    playerAABB.center = (playerAABB.min + playerAABB.max) * 0.5f;
}


    bool Physic::checkCollision(
    float aminx, float aminy, float aminz,
    float amaxx, float amaxy, float amaxz,
    float bminx, float bminy, float bminz,
    float bmaxx, float bmaxy, float bmaxz)
    {
        return (aminx < bmaxx && amaxx > bminx) &&
               (aminy < bmaxy && amaxy > bminy) &&
               (aminz < bmaxz && amaxz > bminz);
    }


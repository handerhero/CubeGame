//
// Created by aspadien on 07.01.2026.
//
#include "ObjectsAndPhysic/Player.h"

AABB Player::getPlayerAABB(glm::vec3 center, float scale) {
    Player player;
    player.height = 1.8f * scale;
    player.width = 0.6f * scale;
    AABB aabb(center, player.height, player.width, player.width);
    return aabb;
}
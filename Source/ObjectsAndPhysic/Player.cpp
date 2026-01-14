module;
#include "glm/glm.hpp"
import AABB;
module Player;

AABB Player::getPlayerAABB(const glm::vec3 center, const float scale) {
    Player player;
    player.height = 1.8f * scale;
    player.width = 0.6f * scale;

    AABB aabb(center, player.height, player.width, player.width);
    return aabb;
}
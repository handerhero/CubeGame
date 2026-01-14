module;
#include <glm/glm.hpp> // для вектора позиции
import  AABB;
export module Player;

export class Player {
    float height = 1.8f;
    float width = 0.6f;
    float eyeHeight =  height / 0.8f;

    static AABB getPlayerAABB(glm::vec3 center, float scale);
};
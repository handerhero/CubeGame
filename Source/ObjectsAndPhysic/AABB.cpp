//
// Created by aspadien on 08.01.2026.
//
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/extended_min_max.hpp"
#include "ObjectsAndPhysic/AABB.h"

AABB::AABB(glm::vec3 center, const float size) {
    this->min.x = center.x - size/2;
    this->max.x = center.x + size/2;
    this->min.y = center.y - size/2;
    this->max.y = center.y + size/2;
    this->min.z = center.z - size/2;
    this->max.z = center.z + size/2;
    this->center = center;
}
AABB::AABB(glm::vec3 center, float height, float width, float length) {
    this->min.x = center.x - width/2;
    this->max.x = center.x + width/2;
    this->min.y = center.y - height/2;
    this->max.y = center.y + height/2;
    this->min.z = center.z - length/2;
    this->max.z = center.z + length/2;
    this->center = center;
}

// Трансформация AABB через матрицу модели
 AABB AABB::transformed(const glm::mat4& model) const {
    glm::vec3 corners[8] = {
        {min.x, min.y, min.z}, {max.x, min.y, min.z},
        {min.x, max.y, min.z}, {max.x, max.y, min.z},
        {min.x, min.y, max.z}, {max.x, min.y, max.z},
        {min.x, max.y, max.z}, {max.x, max.y, max.z}
    };
    AABB result;
    result.min = result.max = glm::vec3(model * glm::vec4(corners[0], 1.0f));
    for (int i = 1; i < 8; i++) {
        glm::vec3 p = glm::vec3(model * glm::vec4(corners[i], 1.0f));
        result.min = glm::min(result.min, p);
        result.max = glm::max(result.max, p);
    }
    return result;
}

AABB AABB::getBlockAABB(float center, const float scale) {
    const AABB aabb(glm::vec3(center),scale);
    return aabb;
}


glm::vec3 AABB::getCenter() const {
    return (this->min + this->max) * 0.5f;
}
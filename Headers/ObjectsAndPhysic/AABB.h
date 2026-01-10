#include "glm/vec3.hpp"
#include "glm/matrix.hpp"

//
// Created by aspadien on 09.09.2025.
//

#ifndef UNTITLED2_AABB_H
#define UNTITLED2_AABB_H

class AABB {
public:
    glm::vec3 min{};
    glm::vec3 max{};
    glm::vec3 center{};

        AABB() = default;

        AABB(glm::vec3 center, float size);
        AABB(glm::vec3 center, float height, float width, float length);

        // Трансформация AABB через матрицу модели
        [[nodiscard]] AABB transformed(const glm::mat4& model) const;

    static AABB getBlockAABB(float center, float scale);


    [[nodiscard]] glm::vec3 getCenter() const;
};












#endif //UNTITLED2_AABB_H
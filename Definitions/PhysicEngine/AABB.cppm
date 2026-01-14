module;

#include "glm/glm.hpp"

export module AABB;

export class AABB {
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
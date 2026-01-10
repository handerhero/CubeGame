//
// Created by aspadien on 13.12.2025.
//

#ifndef CUBEREBUILD_FRUSTUM_HPP
#define CUBEREBUILD_FRUSTUM_HPP

#pragma once
#include <glm/glm.hpp>
#include <array>


struct Plane {
    glm::dvec3 n; // нормаль (НАПРАВЛЕНА ВНУТРЬ ФРУСТУМА)
    double d;     // смещение

    [[nodiscard]] double distance(const glm::dvec3& p) const {
        return glm::dot(n, p) + d;
    }
};



class Frustum {
public:
    std::array<Plane, 6> planes;

    // ВАЖНО: Сюда нужно передавать "Человеческие" параметры!
    // nearPlane = 0.1f (или 0.5f), farPlane = Дальность прорисовки (например 2000.0f)
    // НЕ ПЕРЕДАВАЙТЕ сюда Reverse-Z параметры (4096, 0.1)!
    void updateFromCamera(
        const glm::dvec3& pos,
        const glm::dvec3& forward,
        const glm::dvec3& up,
        double fovY,     // В радианах! (Советую брать FOV чуть больше, чем у камеры, напр. 90-100 град)
        double aspect,
        double nearPlane,
        double farPlane
    ) {
        const glm::dvec3 f = glm::normalize(forward);
        const glm::dvec3 r = glm::normalize(glm::cross(f, up));
        const glm::dvec3 u = glm::cross(r, f); // Пересчитываем UP, чтобы он был точно перпендикулярен

        const double tanY = std::tan(fovY * 0.5);
        const double tanX = tanY * aspect;

        // --- Near Plane ---
        planes[0].n = f;
        planes[0].d = -glm::dot(f, pos + f * nearPlane);

        // --- Far Plane ---
        planes[1].n = -f;
        planes[1].d = -glm::dot(-f, pos + f * farPlane);

        // --- Left Plane ---
        // Нормаль должна смотреть ВПРАВО (внутрь). Строится на векторе левой грани (f - r*tanX) и Up.
        {
            glm::dvec3 normal = glm::normalize(glm::cross(f - r * tanX, u));
            planes[2].n = normal;
            planes[2].d = -glm::dot(normal, pos);
        }

        // --- Right Plane ---
        // Нормаль должна смотреть ВЛЕВО (внутрь). Строится на Up и векторе правой грани (f + r*tanX).
        {
            glm::dvec3 normal = glm::normalize(glm::cross(u, f + r * tanX));
            planes[3].n = normal;
            planes[3].d = -glm::dot(normal, pos);
        }

        // --- Bottom Plane ---
        // Нормаль смотрит ВВЕРХ.
        {
            glm::dvec3 normal = glm::normalize(glm::cross(r, f - u * tanY));
            planes[4].n = normal;
            planes[4].d = -glm::dot(normal, pos);
        }

        // --- Top Plane ---
        // Нормаль смотрит ВНИЗ.
        {
            glm::dvec3 normal = glm::normalize(glm::cross(f + u * tanY, r));
            planes[5].n = normal;
            planes[5].d = -glm::dot(normal, pos);
        }
    }

    // Эта функция у вас отличная, оставляем как есть.
    [[nodiscard]] bool isBoxVisible(const glm::dvec3& min, const glm::dvec3& max) const {
        // center = min + (max - min) * 0.5 = (min + max) * 0.5
        const glm::dvec3 center = (min + max) * 0.5;
        const glm::dvec3 extent = max - center; // Половина размера (half-extents)

        for (const Plane& p : planes) {
            // Проецируем радиус бокса на нормаль плоскости
            // Это самая быстрая реализация AABB-Plane теста
            const double r =
                extent.x * std::abs(p.n.x) +
                extent.y * std::abs(p.n.y) +
                extent.z * std::abs(p.n.z);

            double dist = p.distance(center);

            // Если центр бокса находится ЗА плоскостью дальше, чем его радиус -> бокс полностью снаружи
            if (dist < -r)
                return false;
        }
        return true;
    }
};


#endif //CUBEREBUILD_FRUSTUM_HPP
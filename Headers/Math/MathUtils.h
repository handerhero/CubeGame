//
// Created by aspadien on 11.09.2025.
//

#ifndef UNTITLED2_MATHUTILS_H
#define UNTITLED2_MATHUTILS_H

#pragma once
#include <glm/glm.hpp>

// Целочисленное деление с округлением вниз (floor division)
inline int floorDiv(int a, int b) {
    return (a >= 0 ? a / b : ((a - b + 1) / b));
}

// Целочисленный модуль с результатом в [0, b)
inline int floorMod(int a, int b) {
    int m = a % b;
    return m < 0 ? m + b : m;
}

// Версия для векторов glm::ivec3
inline glm::ivec3 floorDiv(const glm::ivec3& v, int b) {
    return { floorDiv(v.x, b), floorDiv(v.y, b), floorDiv(v.z, b) };
}

inline glm::ivec3 floorMod(const glm::ivec3& v, int b) {
    return { floorMod(v.x, b), floorMod(v.y, b), floorMod(v.z, b) };
}
// Быстрые inline-утилиты (без вызовов floor/abs из libc)
static inline int fastFloorDiv(int a, int b) {
    // корректно для любого знака b>0 (наш case: chunkSize = 32)
    int q = a / b;              // c++ truncates toward 0
    int r = a % b;
    if (r != 0 && ((r < 0) != (b < 0))) --q; // если остаток имеет знак, отличный от делителя, корректируем
    return q;
}
static inline int fastFloorMod32(int a) {
    // chunkSize == 32 -> use mask; but mask must work for negative a
    // (a & 31) gives wrong result for negative a in C++ because -1 & 31 == 31 which is actually desired for floorMod.
    // For negative a we want (a mod 32) in range 0..31 which (a & 31) actually produces because a is two's complement.
    // So for chunkSize=32 this is valid.
    return a & 31;
}

// intbound: возвращает расстояние до следующего целого границы по оси
static inline float intbound(float s, float ds) {
    if (ds > 0.0f) {
        // next integer boundary from s
        float s_floor = std::floor(s);
        return (s_floor + 1.0f - s) / ds;
    } else if (ds < 0.0f) {
        float s_floor = std::floor(s);
        return (s - s_floor) / -ds;
    } else {
        return FLT_MAX;
    }
}


#endif //UNTITLED2_MATHUTILS_H
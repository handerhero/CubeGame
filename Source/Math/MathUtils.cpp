module;
#include <glm/glm.hpp>
module MathUtils;
// Целочисленное деление с округлением вниз (floor division)
int floorDiv(int a, int b) {
    return (a >= 0 ? a / b : ((a - b + 1) / b));
}

// Целочисленный модуль с результатом в [0, b)
int floorMod(int a, int b) {
    int m = a % b;
    return m < 0 ? m + b : m;
}

// Версия для векторов glm::ivec3
glm::ivec3 floorDiv(const glm::ivec3& v, int b) {
    return { floorDiv(v.x, b), floorDiv(v.y, b), floorDiv(v.z, b) };
}

glm::ivec3 floorMod(const glm::ivec3& v, int b) {
    return { floorMod(v.x, b), floorMod(v.y, b), floorMod(v.z, b) };
}
// Быстрые утилиты (без вызовов floor/abs из libc)
int fastFloorDiv(int a, int b) {
    int q = a / b;              // c++ truncates toward 0
    int r = a % b;
    if (r != 0 && ((r < 0) != (b < 0))) --q; // если остаток имеет знак, отличный от делителя, корректируем
    return q;
}
int fastFloorMod32(int a) {
    return a & 31;
}


float intbound(float s, float ds) {
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
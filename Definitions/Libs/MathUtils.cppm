module;
#include <glm/glm.hpp>

export module MathUtils;

// Целочисленное деление с округлением вниз (floor division)
export int floorDiv(int a, int b);

// Целочисленный модуль с результатом в [0, b)
export int floorMod(int a, int b);

// Версия для векторов glm::ivec3
export glm::ivec3 floorDiv(const glm::ivec3& v, int b);

export glm::ivec3 floorMod(const glm::ivec3& v, int b);
// Быстрые inline-утилиты (без вызовов floor/abs из libc)
export int fastFloorDiv(int a, int b);
export int fastFloorMod32(int a);

// intbound: возвращает расстояние до следующего целого границы по оси
export float intbound(float s, float ds);
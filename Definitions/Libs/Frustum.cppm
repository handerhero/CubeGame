module;
#include "glm/glm.hpp"
export module Frustum;

export struct Frustum {
    float planes[6][4]; // 6 плоскостей по 4 float (x, y, z, w)
};

// Функция нормализации плоскости (обязательно!)
export void NormalizePlane(float plane[4]) {
    float len = std::sqrt(plane[0] * plane[0] + plane[1] * plane[1] + plane[2] * plane[2]);
    plane[0] /= len;
    plane[1] /= len;
    plane[2] /= len;
    plane[3] /= len;
}

export void CalculateFrustum(const glm::mat4& proj, const glm::mat4& view, float output[6][4]) {
    // ВАЖНО: Для Camera Relative подхода берем View матрицу, но обнуляем позицию (последний столбец)
    glm::mat4 viewRel = view;
    viewRel[3][0] = 0; viewRel[3][1] = 0; viewRel[3][2] = 0;

    glm::mat4 m = proj * viewRel;

    // Извлечение по Gribb-Hartmann
    // Left:   Row 3 + Row 0
    // Right:  Row 3 - Row 0
    // Bottom: Row 3 + Row 1
    // Top:    Row 3 - Row 1
    // Near:   Row 3 + Row 2
    // Far:    Row 3 - Row 2

    // Индексы плоскостей (порядок важен для шейдера)
    // 0: Left, 1: Right, 2: Bottom, 3: Top, 4: Near, 5: Far

    // ВНИМАНИЕ: GLM хранит [col][row], поэтому строки матрицы M это:
    // row0 = vec4(m[0][0], m[1][0], m[2][0], m[3][0]) и т.д.
    // Для простоты распишем через указатели или raw data, но вот логика для GLM:

    const float* mat = (const float*)&m;
    #define M(row, col) mat[col * 4 + row]

    // Left
    output[0][0] = M(3,0) + M(0,0); output[0][1] = M(3,1) + M(0,1); output[0][2] = M(3,2) + M(0,2); output[0][3] = M(3,3) + M(0,3);
    // Right
    output[1][0] = M(3,0) - M(0,0); output[1][1] = M(3,1) - M(0,1); output[1][2] = M(3,2) - M(0,2); output[1][3] = M(3,3) - M(0,3);
    // Bottom
    output[2][0] = M(3,0) + M(1,0); output[2][1] = M(3,1) + M(1,1); output[2][2] = M(3,2) + M(1,2); output[2][3] = M(3,3) + M(1,3);
    // Top
    output[3][0] = M(3,0) - M(1,0); output[3][1] = M(3,1) - M(1,1); output[3][2] = M(3,2) - M(1,2); output[3][3] = M(3,3) - M(1,3);
    // Near
    output[4][0] = M(3,0) + M(2,0); output[4][1] = M(3,1) + M(2,1); output[4][2] = M(3,2) + M(2,2); output[4][3] = M(3,3) + M(2,3);
    // Far
    output[5][0] = M(3,0) - M(2,0); output[5][1] = M(3,1) - M(2,1); output[5][2] = M(3,2) - M(2,2); output[5][3] = M(3,3) - M(2,3);

    for(int i=0; i<6; i++) NormalizePlane(output[i]);
}

// Отправка в шейдер

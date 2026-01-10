//
// Created by aspadien on 08.09.2025.
//

#ifndef UNTITLED2_PLAYER_H
#define UNTITLED2_PLAYER_H

#pragma once
#include "AABB.h"
#include <glm/glm.hpp> // для вектора позиции

class Player {
    float height = 1.8f;
    float width = 0.6f;
  float eyeHeight =  height / 0.8f;

    static AABB getPlayerAABB(glm::vec3 center, float scale);

};




#endif //UNTITLED2_PLAYER_H
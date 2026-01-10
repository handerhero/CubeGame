//
// Created by aspadien on 12.09.2025.
//

#ifndef UNTITLED2_MOUSEINTERACTIONS_H
#define UNTITLED2_MOUSEINTERACTIONS_H

#include <GLFW/glfw3.h>

#include "../Camera/Camera.hpp"
#include "../ChunkSystem/Chunk.hpp"

class MouseInteractions {
public:
   static std::shared_ptr<Chunk> castRayAndModifyBlockCached(
       GLFWwindow *window,
       Camera &camera,
       ChunkMap &chunks,
       int placeBlockID,
       float maxDist
   );

    static void set(int x, int y, int z, uint8_t block, const std::shared_ptr<Chunk> &chunk);

};



#endif //UNTITLED2_MOUSEINTERACTIONS_H

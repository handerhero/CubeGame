module;
#include <GLFW/glfw3.h>
#include <memory>
import Chunk;
import Camera;

export module Mouse;

export class MouseInteractions {
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

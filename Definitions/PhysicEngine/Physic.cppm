module;

#include <memory>
#include <glm/vec3.hpp>

#include <vector>

import Window;
import Camera;
import Keyboard;
import AABB;
import Chunk;

export module Physic;

export class Physic {
public:
    AABB *playerAABB;

    explicit Physic(const Camera &camera);
    ~Physic();
    bool onGround = true;
    glm::vec3 kineticVector = glm::vec3(0.0f);

    void makeTick(Keyboard &keyboard_control,std::vector<std::shared_ptr<Chunk>> &batchG, Camera &camera, double dt,ChunkMap &chunkMap, const Window &window);



    // двигаем игрока с учётом коллизий
    void movePlayer(AABB& playerAABB, glm::vec3& velocity,double dt, bool& onGround, ChunkMap &chunks);


    bool checkCollision(float aminx, float aminy, float aminz,float amaxx, float amaxy, float amaxz,float bminx, float bminy, float bminz,float bmaxx, float bmaxy, float bmaxz);

};

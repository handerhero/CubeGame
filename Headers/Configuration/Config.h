#ifndef UNTITLED2_CONFIG_H
#define UNTITLED2_CONFIG_H
#include "glm/vec4.hpp"

struct DrawCommand {
    uint32_t count;
    uint32_t instanceCount;
    uint32_t firstIndex;
    uint32_t baseVertex;
    uint32_t baseInstance;
};


struct DrawArraysIndirectCommand {
    uint32_t count;
    uint32_t instanceCount;
    uint32_t first;
    uint32_t baseInstance;
};

// Метаданные о чанке, которые мы отправляем на GPU.
// CPU знает только это.
struct ChunkMetadata {
    int X;
    int Y;
    int Z;
    uint32_t instanceCount;
    uint32_t first;
    uint32_t number;
    unsigned int pad1;
    unsigned int pad2;
};

struct Vertex {
    glm::ivec4 locationAndNormalAndType;
    glm::ivec4 empty;
};

inline float moveSpeed = 1500.0f; // блоков/секунда
inline float gravity = 40.0f;
inline float legsPower = 28.0f;
inline float playerEyeHeight = 0.72f;
inline int raySteps = 100;
inline int chunkSize = 32;
inline float stepSize = 0.1f;
inline float fov = 70.0f;
inline int renderDistanceXZ = 16;   // радиус генерации по X и Z
inline int renderHeightY   = 8;    // радиус генерации по Y (в блоках чанка, по высоте)
inline int MAX_TERRAIN_HEIGHT  = 128;

inline glm::vec4 dirt = glm::vec4(0.0f, 0.0f, 0.5f, 0.5f);
inline glm::vec4 stone = glm::vec4(0.5f, 0.0f, 1.0f, 0.5f);
inline glm::ivec4 err = glm::ivec4(glm::ivec3(0),-1);
inline bool programIsRunning = false;



#endif //UNTITLED2_CONFIG_H
//
// Created by aspadien on 17.10.2025.
//

#ifndef UNTITLED2_CHUNKSMESHINGONGPU_H
#define UNTITLED2_CHUNKSMESHINGONGPU_H


#include <iostream>
#include "../Configuration/Config.h"

inline void LoadTexture3D() {

}

inline void MeshChunk() {

}

inline void SelectMeshToRender() {

}

inline void VAOSettingSetInt(GLuint vao, GLuint arrayBuffer, int vertexSize) {

    glBindVertexArray(vao);
    // Используем тот же буфер SSBO, но теперь как VBO (Vertex Buffer Object)
    glBindBuffer(GL_ARRAY_BUFFER, arrayBuffer);

    // Атрибут инстансов
    glEnableVertexAttribArray(0);
    glVertexAttribIPointer(0, 1, GL_UNSIGNED_INT, vertexSize, (void*)0);
    glVertexAttribDivisor(0, 1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

}

inline void getCommandBufferInfo(const GLuint commandBuffer) {
    DrawArraysIndirectCommand resultCommand{};
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, commandBuffer);
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(DrawArraysIndirectCommand), &resultCommand);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // Выводим в консоль то, что насчитал GPU
    std::cout << "Vertex: " << resultCommand.count << std::endl;
    std::cout << "Offset: " << resultCommand.first << std::endl;
    std::cout << "instance count: " << resultCommand.instanceCount << std::endl;
    std::cout << "Base instance: " << resultCommand.baseInstance << std::endl;
}


inline void getChunkBufferInfo(const GLuint chunkBuffer) {
    ChunkMetadata resultCommand{};
    for (int i = 0; i < 10; i++){
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, chunkBuffer);
        glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, i * sizeof(ChunkMetadata), sizeof(ChunkMetadata), &resultCommand);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        // Выводим в консоль то, что насчитал GPU
        std::cout << "X: " << resultCommand.X << std::endl;
        std::cout << "Y: " << resultCommand.Y << std::endl;
        std::cout << "Y: " << resultCommand.Z << std::endl;
        std::cout << "Instance count: " << resultCommand.instanceCount << std::endl;
        std::cout << "Offset: " << resultCommand.first << std::endl;
        std::cout << "Number:" << resultCommand.number << std::endl;
    }
}

#endif //UNTITLED2_CHUNKSMESHINGONGPU_H
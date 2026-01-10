//
// Created by aspadien on 13.12.2025.
//

#ifndef CUBEREBUILD_FPSCOUNTER_HPP
#define CUBEREBUILD_FPSCOUNTER_HPP
#include <iosfwd>
#include <sstream>
#include <GLFW/glfw3.h>

struct FPSCounterAdvanced {
    double lastTime = 0.0;
    double lastFrameTime = 0.0;
    int nbFrames = 0;
    double maxFrameTime = 0.0; // Самый долгий кадр за интервал

    void update(GLFWwindow* window) {
        double currentTime = glfwGetTime();
        double currentDelta = currentTime - lastFrameTime;
        lastFrameTime = currentTime;

        nbFrames++;
        if (currentDelta > maxFrameTime) maxFrameTime = currentDelta;

        if (currentTime - lastTime >= 0.5) { // Раз в полсекунды
            double fps = double(nbFrames) / (currentTime - lastTime);
            double avgMs = 1000.0 / fps;
            double worstMs = maxFrameTime * 1000.0; // Худший кадр в мс

            std::stringstream ss;
            ss << "FPS: " << int(fps)
               << " | Avg: " << avgMs << "ms"
               << " | Worst: " << worstMs << "ms"; // Если Worst сильно больше Avg -> у вас фризы!

            glfwSetWindowTitle(window, ss.str().c_str());

            nbFrames = 0;
            lastTime = currentTime;
            maxFrameTime = 0.0;
        }
    }
};

#endif //CUBEREBUILD_FPSCOUNTER_HPP
//
// Created by aspadien on 07.01.2026.
//
#include "Camera/Camera.hpp"

Camera::Camera() {
    camera = new CameraNormal;
    camera_free = new CameraFree;
}

Camera::~Camera() {
    delete camera;
    delete camera_free;
}

glm::vec3 Camera::CameraNormal::forward() const{
            return glm::normalize(glm::vec3(
                cos(glm::radians(yaw)) * cos(glm::radians(pitch)),
                sin(glm::radians(pitch)),
                sin(glm::radians(yaw)) * cos(glm::radians(pitch))
            ));
        }

glm::vec3 Camera::CameraNormal::right() const {
    return glm::normalize(glm::cross(forward(), glm::vec3(0,1,0)));
}

glm::vec3 Camera::CameraNormal::up() const {
    return glm::normalize(glm::cross(right(), forward()));
}

void Camera::CameraNormal::rotate(const float dYaw, const float dPitch) {
    yaw   += dYaw;
    pitch += dPitch;

    // ограничиваем pitch (чтобы нельзя было "перевернуть голову")
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;
}

glm::vec3 Camera::CameraFree::forward() const {
    return glm::normalize(orientation * glm::vec3(0,0,-1));
}
glm::vec3 Camera::CameraFree::up() const {
    return glm::normalize(orientation * glm::vec3(0,1,0));
}
glm::vec3 Camera::CameraFree::right() const {
    return glm::normalize(orientation * glm::vec3(1,0,0));
}
void Camera::CameraFree::rotate(const float angleDeg, const glm::vec3 axis) {
    glm::quat q = glm::angleAxis(glm::radians(angleDeg), glm::normalize(axis));
    orientation = glm::normalize(q * orientation);
}
//
// Created by aspadien on 12.01.2026.
//
module;
#include "glm/gtc/quaternion.hpp"
#include "glm/vec3.hpp"

export module Camera;



export class Camera {
public:
    glm::vec3 pos{1, 15, 1};


    Camera();
    ~Camera();

    struct CameraNormal {

        float yaw   = -90.0f; // поворот по горизонтали (влево/вправо)
        float pitch = 0.0f;   // наклон головы (вверх/вниз)

        [[nodiscard]] glm::vec3 forward() const;

        [[nodiscard]] glm::vec3 right() const ;

        [[nodiscard]] glm::vec3 up() const;

        void rotate(float dYaw, float dPitch);

    };

    CameraNormal* camera;

    struct CameraFree {

        glm::quat orientation{1,0,0,0};

        // единичный кватернион = нет вращения
        [[nodiscard]] glm::vec3 forward() const;
        [[nodiscard]] glm::vec3 up() const;
        [[nodiscard]] glm::vec3 right() const;
        void rotate(float angleDeg, glm::vec3 axis);
    };

    CameraFree* camera_free;
};
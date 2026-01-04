#ifndef CAMERA_H
#define CAMERA_H

#include "core/math3d.h"

class Camera
{
public:
    // Орбитальная камера вокруг целевой точки
    Vec3 target{0, 2.0f, 0};

    float yaw = 0.7f;    // Радианы
    float pitch = 0.25f; // Радианы
    float radius = 35.0f;

    // Боковое смещение влево/вправо вдоль оси X
    float strafe = 0.0f;

    Mat4 view() const;
    Mat4 proj(float aspect) const;

    Vec3 eye() const;

    void rotate(float dyaw, float dpitch);
    void zoom(float dr);
    void slide(float dx);
};

#endif // CAMERA_H

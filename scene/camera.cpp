#include "camera.h"

#include <algorithm>

Vec3 Camera::eye() const
{
    float cp = std::cos(pitch);
    Vec3 dir{
        std::cos(yaw)*cp,
        std::sin(pitch),
        std::sin(yaw)*cp
    };
    Vec3 t = target + Vec3{strafe,0,0};
    return t + dir * radius;
}

Mat4 Camera::view() const
{
    Vec3 t = target + Vec3{strafe,0,0};
    return Mat4::lookAt(eye(), t, Vec3{0,1,0});
}

Mat4 Camera::proj(float aspect) const
{
    return Mat4::perspective(45.0f * 3.1415926f/180.0f, aspect, 0.1f, 500.0f);
}

void Camera::rotate(float dyaw, float dpitch)
{
    yaw += dyaw;
    pitch += dpitch;
    // Ограничение, чтобы камера не уходила ниже моста (запрет на вид снизу)
    // Оставление pitch в верхней полусфере (камера выше целевой точки по Y)
    const float maxPitch = 1.35f;
    const float minPitch = 0.10f; // ~5.7 градусов
    pitch = std::max(minPitch, std::min(maxPitch, pitch));
}

void Camera::zoom(float dr)
{
    radius += dr;
    radius = std::max(10.0f, std::min(120.0f, radius));
}

void Camera::slide(float dx)
{
    strafe += dx;
    strafe = std::max(-25.0f, std::min(25.0f, strafe));
}

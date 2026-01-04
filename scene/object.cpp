#include "object.h"
#include "core/math3d.h"

Mat4 Object::modelMatrix() const
{
    Mat4 T = Mat4::translate(position);
    Mat4 R = Mat4::rotateY(rotation.y) * Mat4::rotateX(rotation.x) * Mat4::rotateZ(rotation.z);
    Mat4 S = Mat4::scale(scale);
    return T * R * S;
}

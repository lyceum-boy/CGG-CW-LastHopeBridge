#ifndef OBJECT_H
#define OBJECT_H

#include "core/math3d.h"

class Mesh;
class Scene;
class Shader;
class Texture;
class QOpenGLFunctions_3_3_Core;

class Object
{
public:
    virtual ~Object() = default;

    Vec3 position{0,0,0};
    Vec3 rotation{0,0,0}; // Радианы
    Vec3 scale{1,1,1};

    virtual void update(Scene& scene, float dt) { (void)scene; (void)dt; }
    virtual void draw(QOpenGLFunctions_3_3_Core* f, const Shader& sh) const = 0;

    Mat4 modelMatrix() const;
};

#endif // OBJECT_H

#ifndef MESH_H
#define MESH_H

#include <vector>
#include <QOpenGLFunctions_3_3_Core>
#include "math3d.h"

struct Vertex {
    Vec3 pos;
    Vec3 nrm;
    Vec2 uv;
};

class Mesh
{
public:
    Mesh() = default;
    ~Mesh() = default;

    void upload(QOpenGLFunctions_3_3_Core* f, const std::vector<Vertex>& vertices, const std::vector<unsigned>& indices);
    void draw(QOpenGLFunctions_3_3_Core* f) const;

    bool isValid() const { return m_vao != 0; }

private:
    unsigned m_vao=0, m_vbo=0, m_ebo=0;
    int m_indexCount=0;
};

#endif // MESH_H

#include "mesh.h"

void Mesh::upload(QOpenGLFunctions_3_3_Core* f, const std::vector<Vertex>& vertices, const std::vector<unsigned>& indices)
{
    m_indexCount = (int)indices.size();
    if (!m_vao) f->glGenVertexArrays(1, &m_vao);
    if (!m_vbo) f->glGenBuffers(1, &m_vbo);
    if (!m_ebo) f->glGenBuffers(1, &m_ebo);

    f->glBindVertexArray(m_vao);

    f->glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    f->glBufferData(GL_ARRAY_BUFFER, (int)(vertices.size()*sizeof(Vertex)), vertices.data(), GL_STATIC_DRAW);

    f->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    f->glBufferData(GL_ELEMENT_ARRAY_BUFFER, (int)(indices.size()*sizeof(unsigned)), indices.data(), GL_STATIC_DRAW);

    // Вершинные атрибуты: 0 - позиция вершины, 1 - нормаль, 2 - текстурные координаты (UV)
    f->glEnableVertexAttribArray(0);
    f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));
    f->glEnableVertexAttribArray(1);
    f->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, nrm));
    f->glEnableVertexAttribArray(2);
    f->glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));

    f->glBindVertexArray(0);
}

void Mesh::draw(QOpenGLFunctions_3_3_Core* f) const
{
    f->glBindVertexArray(m_vao);
    f->glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, nullptr);
    f->glBindVertexArray(0);
}

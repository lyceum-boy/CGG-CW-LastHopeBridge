#ifndef TEXTURE_H
#define TEXTURE_H

#include <QString>
#include <QOpenGLFunctions_3_3_Core>

class Texture
{
public:
    Texture() = default;
    ~Texture() = default;

    bool load(QOpenGLFunctions_3_3_Core* f, const QString& path, bool srgb = false);
    void bind(QOpenGLFunctions_3_3_Core* f, int unit) const;

    unsigned id() const { return m_id; }

private:
    unsigned m_id = 0;
    int m_w = 0, m_h = 0;
};

#endif // TEXTURE_H

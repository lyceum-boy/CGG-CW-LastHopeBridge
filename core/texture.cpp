#include "texture.h"

#include <QFileInfo>
#include <QImage>

bool Texture::load(QOpenGLFunctions_3_3_Core* f, const QString& path, bool srgb)
{
    QImage img(path);
    if (img.isNull()) return false;
    img = img.mirrored(false, true);
    img = img.convertToFormat(QImage::Format_RGBA8888);
    m_w = img.width();
    m_h = img.height();

    if (!m_id) f->glGenTextures(1, &m_id);
    f->glBindTexture(GL_TEXTURE_2D, m_id);

    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    const int internal = srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8;
    f->glTexImage2D(GL_TEXTURE_2D, 0, internal, m_w, m_h, 0, GL_RGBA, GL_UNSIGNED_BYTE, img.constBits());
    f->glGenerateMipmap(GL_TEXTURE_2D);

    f->glBindTexture(GL_TEXTURE_2D, 0);
    return true;
}

void Texture::bind(QOpenGLFunctions_3_3_Core* f, int unit) const
{
    f->glActiveTexture(GL_TEXTURE0 + unit);
    f->glBindTexture(GL_TEXTURE_2D, m_id);
}

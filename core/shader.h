#ifndef SHADER_H
#define SHADER_H

#include <QString>
#include <QOpenGLFunctions_3_3_Core>

class Shader
{
public:
    Shader() = default;
    ~Shader();

    bool build(QOpenGLFunctions_3_3_Core* f, const char* vsSrc, const char* fsSrc, QString* log = nullptr);
    void use(QOpenGLFunctions_3_3_Core* f) const;

    unsigned id() const { return m_program; }

    // Uniform-переменные
    void setMat4(QOpenGLFunctions_3_3_Core* f, const char* name, const float* v) const;
    void setVec3(QOpenGLFunctions_3_3_Core* f, const char* name, float x, float y, float z) const;
    void setFloat(QOpenGLFunctions_3_3_Core* f, const char* name, float v) const;
    void setInt(QOpenGLFunctions_3_3_Core* f, const char* name, int v) const;

private:
    unsigned m_program = 0;
    unsigned compile(QOpenGLFunctions_3_3_Core* f, unsigned type, const char* src, QString* log);
};

#endif // SHADER_H

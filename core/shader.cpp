#include "shader.h"

#include <QByteArray>

Shader::~Shader() {}

unsigned Shader::compile(QOpenGLFunctions_3_3_Core* f, unsigned type, const char* src, QString* log)
{
    unsigned s = f->glCreateShader(type);
    f->glShaderSource(s, 1, &src, nullptr);
    f->glCompileShader(s);

    int ok = 0;
    f->glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok){
        int len = 0;
        f->glGetShaderiv(s, GL_INFO_LOG_LENGTH, &len);
        QByteArray buf(len, 0);
        f->glGetShaderInfoLog(s, len, nullptr, buf.data());
        if (log) *log += QString::fromUtf8(buf);
        f->glDeleteShader(s);
        return 0;
    }
    return s;
}

bool Shader::build(QOpenGLFunctions_3_3_Core* f, const char* vsSrc, const char* fsSrc, QString* log)
{
    unsigned vs = compile(f, GL_VERTEX_SHADER, vsSrc, log);
    if (!vs) return false;
    unsigned fs = compile(f, GL_FRAGMENT_SHADER, fsSrc, log);
    if (!fs){ f->glDeleteShader(vs); return false; }

    m_program = f->glCreateProgram();
    f->glAttachShader(m_program, vs);
    f->glAttachShader(m_program, fs);
    f->glLinkProgram(m_program);

    int ok = 0;
    f->glGetProgramiv(m_program, GL_LINK_STATUS, &ok);
    if (!ok){
        int len = 0;
        f->glGetProgramiv(m_program, GL_INFO_LOG_LENGTH, &len);
        QByteArray buf(len, 0);
        f->glGetProgramInfoLog(m_program, len, nullptr, buf.data());
        if (log) *log += QString::fromUtf8(buf);
        f->glDeleteProgram(m_program);
        m_program = 0;
    }

    f->glDeleteShader(vs);
    f->glDeleteShader(fs);
    return ok;
}

void Shader::use(QOpenGLFunctions_3_3_Core* f) const
{
    f->glUseProgram(m_program);
}

void Shader::setMat4(QOpenGLFunctions_3_3_Core* f, const char* name, const float* v) const
{
    int loc = f->glGetUniformLocation(m_program, name);
    if (loc >= 0) f->glUniformMatrix4fv(loc, 1, GL_FALSE, v);
}

void Shader::setVec3(QOpenGLFunctions_3_3_Core* f, const char* name, float x, float y, float z) const
{
    int loc = f->glGetUniformLocation(m_program, name);
    if (loc >= 0) f->glUniform3f(loc, x,y,z);
}

void Shader::setFloat(QOpenGLFunctions_3_3_Core* f, const char* name, float v) const
{
    int loc = f->glGetUniformLocation(m_program, name);
    if (loc >= 0) f->glUniform1f(loc, v);
}

void Shader::setInt(QOpenGLFunctions_3_3_Core* f, const char* name, int v) const
{
    int loc = f->glGetUniformLocation(m_program, name);
    if (loc >= 0) f->glUniform1i(loc, v);
}

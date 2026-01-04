#ifndef MATH3D_H
#define MATH3D_H

#include <array>
#include <cmath>

struct Vec2 {
    float x=0, y=0;
    Vec2() = default;
    Vec2(float X, float Y): x(X), y(Y) {}
    Vec2 operator+(const Vec2& o) const { return {x+o.x, y+o.y}; }
    Vec2 operator-(const Vec2& o) const { return {x-o.x, y-o.y}; }
    Vec2 operator*(float s) const { return {x*s, y*s}; }
};

struct Vec3 {
    float x=0, y=0, z=0;
    Vec3() = default;
    Vec3(float X, float Y, float Z): x(X), y(Y), z(Z) {}
    Vec3 operator+(const Vec3& o) const { return {x+o.x, y+o.y, z+o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x-o.x, y-o.y, z-o.z}; }
    Vec3 operator*(float s) const { return {x*s, y*s, z*s}; }
    Vec3& operator+=(const Vec3& o){ x+=o.x; y+=o.y; z+=o.z; return *this; }
};

inline float dot(const Vec3& a, const Vec3& b){ return a.x*b.x + a.y*b.y + a.z*b.z; }
inline Vec3 cross(const Vec3& a, const Vec3& b){
    return { a.y*b.z - a.z*b.y,
             a.z*b.x - a.x*b.z,
             a.x*b.y - a.y*b.x };
}
inline float length(const Vec3& v){ return std::sqrt(dot(v,v)); }
inline Vec3 normalize(const Vec3& v){
    float len = length(v);
    if (len < 1e-8f) return {0,0,0};
    return v*(1.0f/len);
}

struct Mat4 {
    std::array<float, 16> m{};

    static Mat4 identity(){
        Mat4 r; r.m = {1,0,0,0,
                       0,1,0,0,
                       0,0,1,0,
                       0,0,0,1};
        return r;
    }

    const float* data() const { return m.data(); }

    static Mat4 translate(const Vec3& t){
        Mat4 r = identity();
        r.m[12] = t.x;
        r.m[13] = t.y;
        r.m[14] = t.z;
        return r;
    }

    static Mat4 scale(const Vec3& s){
        Mat4 r = identity();
        r.m[0] = s.x;
        r.m[5] = s.y;
        r.m[10]= s.z;
        return r;
    }

    static Mat4 rotateX(float rad){
        Mat4 r = identity();
        float c = std::cos(rad), s = std::sin(rad);
        r.m[5] = c;  r.m[9]  = -s;
        r.m[6] = s;  r.m[10] = c;
        return r;
    }

    static Mat4 rotateY(float rad){
        Mat4 r = identity();
        float c = std::cos(rad), s = std::sin(rad);
        r.m[0] = c;  r.m[8]  = s;
        r.m[2] = -s; r.m[10] = c;
        return r;
    }

    static Mat4 rotateZ(float rad){
        Mat4 r = identity();
        float c = std::cos(rad), s = std::sin(rad);
        r.m[0] = c;  r.m[4] = -s;
        r.m[1] = s;  r.m[5] = c;
        return r;
    }

    static Mat4 perspective(float fovYRadians, float aspect, float zNear, float zFar){
        Mat4 r{};
        float f = 1.0f / std::tan(fovYRadians * 0.5f);
        r.m = { f/aspect,0,0,0,
                0,f,0,0,
                0,0,(zFar+zNear)/(zNear-zFar),-1,
                0,0,(2*zFar*zNear)/(zNear-zFar),0 };
        return r;
    }

    static Mat4 lookAt(const Vec3& eye, const Vec3& center, const Vec3& up){
        Vec3 f = normalize(center - eye);
        Vec3 s = normalize(cross(f, up));
        Vec3 u = cross(s, f);

        Mat4 r = identity();
        r.m[0] = s.x; r.m[4] = s.y; r.m[8]  = s.z;
        r.m[1] = u.x; r.m[5] = u.y; r.m[9]  = u.z;
        r.m[2] = -f.x; r.m[6] = -f.y; r.m[10] = -f.z;

        r.m[12] = -dot(s, eye);
        r.m[13] = -dot(u, eye);
        r.m[14] = dot(f, eye);
        return r;
    }
};

inline Mat4 operator*(const Mat4& a, const Mat4& b){
    Mat4 r{};
    for (int c=0;c<4;c++){
        for (int rrow=0;rrow<4;rrow++){
            r.m[c*4 + rrow] =
                a.m[0*4 + rrow]*b.m[c*4 + 0] +
                a.m[1*4 + rrow]*b.m[c*4 + 1] +
                a.m[2*4 + rrow]*b.m[c*4 + 2] +
                a.m[3*4 + rrow]*b.m[c*4 + 3];
        }
    }
    return r;
}

#endif // MATH3D_H

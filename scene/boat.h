#ifndef BOAT_H
#define BOAT_H

#include <memory>
#include <vector>

#include <QString>

#include "object.h"
#include "core/mesh.h"

class Scene;
class Shader;
class QOpenGLFunctions_3_3_Core;

class Boat : public Object
{
public:
    explicit Boat(const QString& objPath);

    void update(Scene& scene, float dt) override;
    void draw(QOpenGLFunctions_3_3_Core* f, const Shader& sh) const override;

    // Проверка, движется ли лодка в данный момент
    // (используется для одноразовых звуковых эффектов)
    bool isActive() const { return m_active; }

private:
    QString m_objPath;
    mutable Mesh m_mesh;
    struct ModelPart {
        Mesh mesh;
        Vec3 ka{0,0,0};
        Vec3 kd{1,1,1};
        Vec3 ks{0,0,0};
        float ns = 16.0f;
        float opacity = 1.0f;

        // Текстуры в MTL файле
        std::unique_ptr<Texture> mapKd; // map_Kd (диффузная текстура)
        std::unique_ptr<Texture> mapKs; // map_Ks (спекулярная / зеркальная текстура)
        std::unique_ptr<Texture> mapKn; // map_Kn / map_Bump / bump (карта нормалей)

        bool useMapKd = false;
        bool useMapKs = false;
        bool useMapKn = false;
    };

    mutable bool m_uploaded = false;
    mutable std::vector<ModelPart> m_parts;

    float m_targetSize = 7.0f; // Максимальный размер модели

    void ensureUploaded(QOpenGLFunctions_3_3_Core* f) const;

    float m_speed = 7.5f;

    // Однократный проход лодки под мостом (вдоль оси Z)
    bool m_active = false;
    bool m_passedThisNight = false;
    float m_startZ = -55.0f;
    float m_endZ   = +55.0f;
    float m_passX   = 0.0f; // Центр между опорами моста
};

#endif // BOAT_H

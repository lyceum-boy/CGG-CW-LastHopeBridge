#ifndef VEHICLE_H
#define VEHICLE_H

#include <memory>
#include <vector>

#include <QString>

#include "object.h"
#include "core/mesh.h"
#include "core/texture.h"

class Scene;
class Shader;
class QOpenGLFunctions_3_3_Core;

// Базовый класс объектов транспорта
class Vehicle : public Object
{
public:
    explicit Vehicle(const QString& objPath);
    virtual ~Vehicle() = default;

    // Параметры полосы и движения (задаются сценой)
    float speed = 5.0f;
    float laneZ = 0.0f;
    float direction = +1.0f; // +1 -> +X, -1 -> -X

    void update(Scene& scene, float dt) override;
    void draw(QOpenGLFunctions_3_3_Core* f, const Shader& sh) const override;

    // Цветовой множитель для случайных цветов
    void setTint(const Vec3& t) { m_tint = t; }

    bool isActive() const { return m_active; }

    // Приблизительная дистанция между машинами
    // Используется размер после нормализации (в мировых единицах)
    float approxLength() const { return m_targetSize; }

protected:
    struct ModelPart {
        Mesh mesh;
        Vec3 kd{1,1,1};
        std::unique_ptr<Texture> diffuse; // nullptr, если текстуры нет (map_Kd отсутствует)
        bool useTexture = false;
    };

    QString m_objPath;

    // Максимальный размер в мировых единицах после нормализации
    float m_targetSize = 2.6f;
    bool m_rotXNeg90 = false; // Исправление для моделей, лежащих на боку
    bool m_rotY180   = false; // Исправление для моделей, ориентированных назад
    Vec3 m_tint{1,1,1};

    // Когда генерация транспорта отключена (ночь),
    // машины, покидающие мост деактивируются
    bool m_active = true;

    mutable bool m_uploaded = false;
    mutable std::vector<ModelPart> m_parts;

    void ensureUploaded(QOpenGLFunctions_3_3_Core* f) const;
};

class Car : public Vehicle
{
public:
    explicit Car(const QString& objPath) : Vehicle(objPath) {
        m_targetSize = 2.6f;
        m_rotXNeg90 = true;
        m_rotY180   = true;
    }
};

class Bus : public Vehicle
{
public:
    explicit Bus(const QString& objPath) : Vehicle(objPath) {
        m_targetSize = 3.6f;
        m_rotXNeg90 = true;
        m_rotY180   = true;
        speed = 4.0f;
    }
};

#endif // VEHICLE_H

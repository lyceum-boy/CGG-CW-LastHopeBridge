#include "vehicle.h"

#include <algorithm>
#include <memory>

#include <QOpenGLFunctions_3_3_Core>
#include <QFileInfo>
#include <QDir>

#include "scene.h"
#include "core/shader.h"
#include "core/objloader.h"

static void normalizeVertices(std::vector<Vertex>& v, float targetMaxDim)
{
    if (v.empty()) return;

    Vec3 mn = v[0].pos, mx = v[0].pos;
    for (const auto& vx : v){
        mn.x = std::min(mn.x, vx.pos.x); mn.y = std::min(mn.y, vx.pos.y); mn.z = std::min(mn.z, vx.pos.z);
        mx.x = std::max(mx.x, vx.pos.x); mx.y = std::max(mx.y, vx.pos.y); mx.z = std::max(mx.z, vx.pos.z);
    }
    Vec3 size = {mx.x-mn.x, mx.y-mn.y, mx.z-mn.z};
    float maxDim = std::max(size.x, std::max(size.y, size.z));
    if (maxDim < 1e-6f) return;

    // Центрирование по XZ и установка на мост по minY = 0
    Vec3 center = {(mn.x+mx.x)*0.5f, mn.y, (mn.z+mx.z)*0.5f};

    float s = targetMaxDim / maxDim;
    for (auto& vx : v){
        vx.pos = (vx.pos - center) * s;
    }
}

static void rotateXNeg90(std::vector<Vertex>& v)
{
    // Поворот позиций и нормалей вокруг оси X на -90 градусов
    for (auto& vx : v){
        float y = vx.pos.y, z = vx.pos.z;
        vx.pos.y = z;
        vx.pos.z = -y;

        float ny = vx.nrm.y, nz = vx.nrm.z;
        vx.nrm.y = nz;
        vx.nrm.z = -ny;
    }
}

static void rotateY180(std::vector<Vertex>& v)
{
    // Поворот на 180 градусов вокруг оси Y (разворот модели)
    for (auto& vert : v){
        vert.pos.x = -vert.pos.x;
        vert.pos.z = -vert.pos.z;
        vert.nrm.x = -vert.nrm.x;
        vert.nrm.z = -vert.nrm.z;
    }
}

Vehicle::Vehicle(const QString& objPath)
    : m_objPath(objPath)
{
    position.y = 2.05f;
}

void Vehicle::ensureUploaded(QOpenGLFunctions_3_3_Core* f) const
{
    if (m_uploaded) return;

    std::vector<ObjLoader::ObjPart> parts;
    if (!ObjLoader::loadParts(m_objPath, parts)) {
        std::vector<Vertex> v;
        std::vector<unsigned> ind;
        ObjLoader::load(":/models/cube.obj", v, ind);

        parts.clear();
        ObjLoader::ObjPart p;
        p.vertices = std::move(v);
        p.indices  = std::move(ind);
        p.material.name = "default";
        p.material.kd = {1,1,1};
        parts.push_back(std::move(p));
    }

    m_parts.clear();
    m_parts.reserve(parts.size());

    for (auto& part : parts){
        if (m_rotXNeg90) rotateXNeg90(part.vertices);
        if (m_rotY180)   rotateY180(part.vertices);
        normalizeVertices(part.vertices, m_targetSize);

        ModelPart mp;
        mp.kd = part.material.kd;
        mp.useTexture = false; // map_Kd не используется
        mp.mesh.upload(f, part.vertices, part.indices);
        m_parts.push_back(std::move(mp));
    }

    m_uploaded = true;
}


void Vehicle::update(Scene& scene, float dt)
{
    // Если транспорт был деактивирован ночью (чтобы избежать повторного появления),
    // то он остается скрытым до тех пор, пока дневной трафик снова не будет разрешен
    if (!m_active){
        if (scene.trafficSpawnEnabled){
            m_active = true;
            // Возрождение у дальнего края в той же полосе
            position.x = (direction > 0.0f) ? -45.0f : 45.0f;
            position.y = 2.05f;
        } else {
            return;
        }
    }

    position.z = laneZ;

    // Во время развода моста транспорт не двигается
    if (scene.bridgeLift > 0.05f) {
        // Сохранение корректного направления для отрисовки
        rotation.y = (direction > 0) ? 0.0f : 3.1415926f;
        return;
    }

    float dx = direction * speed * dt;
    float newX = position.x + dx;

    // Если ночь, транспорт подъезжает к стоп-линии над опорами и ожидает окончания
    // развода моста. Машины, которые уже проехали стоп-линию, продолжают движение
    const bool holdTraffic = (scene.nightPhase != Scene::NightPhase::Day) && (scene.nightPhase != Scene::NightPhase::FadeToDay);

    if (holdTraffic) {
        const float eps = 0.001f;
        if (direction > 0.0f) {
            // Поток в сторону +X останавливается на stopLinePosX
            if (position.x <= scene.stopLinePosX + eps) {
                newX = std::min(newX, scene.stopLinePosX);
            }
        } else {
            // Поток в сторону -X останавливается на stopLineNegX
            if (position.x >= scene.stopLineNegX - eps) {
                newX = std::max(newX, scene.stopLineNegX);
            }
        }
    }

    position.x = newX;

    // Зацикленный трафик днем
    if (scene.trafficSpawnEnabled) {
        if (position.x > 45.0f) position.x = -45.0f;
        if (position.x < -45.0f) position.x = 45.0f;
    } else {
        // Ночью повторное появление машин отключено
        // Машины, которые уже проехали стоп-линию, могут покинуть мост
        const float nightDespawnX = 33.0f; // Сразу после края подъезда (~30)
        if ((direction > 0.0f && position.x > nightDespawnX) || (direction < 0.0f && position.x < -nightDespawnX)) {
            m_active = false;
            return;
        }
    }

    rotation.y = (direction > 0) ? 0.0f : 3.1415926f;
}

void Vehicle::draw(QOpenGLFunctions_3_3_Core* f, const Shader& sh) const
{
    if (!m_active) return;
    const_cast<Vehicle*>(this)->ensureUploaded(f);

    Mat4 M = modelMatrix();
    sh.setMat4(f, "uModel", M.data());

    for (const auto& p : m_parts){
        sh.setVec3(f, "uTint",
                   p.kd.x * m_tint.x,
                   p.kd.y * m_tint.y,
                   p.kd.z * m_tint.z);
        if (p.useTexture && p.diffuse){
            p.diffuse->bind(f, 0);
            sh.setInt(f, "uUseTexture", 1);
        } else {
            sh.setInt(f, "uUseTexture", 0);
        }
        p.mesh.draw(f);
    }

    sh.setVec3(f, "uTint", 1.0f, 1.0f, 1.0f);
    sh.setInt(f, "uUseTexture", 1);
}

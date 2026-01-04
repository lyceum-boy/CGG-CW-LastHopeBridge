#include "boat.h"

#include <algorithm>
#include <memory>

#include <QDir>
#include <QFileInfo>
#include <QOpenGLFunctions_3_3_Core>

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

    // Центрирование по осям XZ и выравнивание по ватерлинии
    Vec3 center = {(mn.x+mx.x)*0.5f, mn.y, (mn.z+mx.z)*0.5f};

    float s = targetMaxDim / maxDim;
    for (auto& vx : v){
        vx.pos = (vx.pos - center) * s;
    }
}

Boat::Boat(const QString& objPath) : m_objPath(objPath) {}

void Boat::ensureUploaded(QOpenGLFunctions_3_3_Core* f) const
{
    if (m_uploaded) return;

    std::vector<ObjLoader::ObjPart> parts;
    if (!ObjLoader::loadParts(m_objPath, parts)){
        std::vector<Vertex> vv; std::vector<unsigned> ii; ObjLoader::load(":/models/cube.obj", vv, ii);
        parts.clear();
        ObjLoader::ObjPart p; p.vertices = std::move(vv); p.indices = std::move(ii);
        p.material.name = "default"; p.material.kd = {1,1,1};
        parts.push_back(std::move(p));
    }

    m_parts.clear();
    m_parts.reserve(parts.size());

    for (auto& part : parts){
        normalizeVertices(part.vertices, m_targetSize);

        ModelPart mp;
        mp.ka = part.material.ka;
        mp.kd = part.material.kd;
        mp.ks = part.material.ks;
        mp.ns = part.material.ns;
        mp.opacity = part.material.d;

        // Текстуры, описанные в MTL файле, упакованы как ресурсы в :/models/...
        auto loadMap = [&](const QString& mapName) -> std::unique_ptr<Texture> {
            if (mapName.isEmpty()) return nullptr;
            const QString base = QFileInfo(mapName).fileName();
            const QString texPath = ":/models/" + base;
            auto t = std::make_unique<Texture>();
            if (!t->load(f, texPath, true)) return nullptr;
            return t;
        };

        mp.mapKd = loadMap(part.material.mapKd);
        mp.mapKs = loadMap(part.material.mapKs);
        mp.mapKn = loadMap(part.material.mapKn);
        mp.useMapKd = (mp.mapKd != nullptr);
        mp.useMapKs = (mp.mapKs != nullptr);
        mp.useMapKn = (mp.mapKn != nullptr);

        mp.mesh.upload(f, part.vertices, part.indices);
        m_parts.push_back(std::move(mp));
    }

    m_uploaded = true;
}

void Boat::update(Scene& scene, float dt)
{
    // Сброс флага при наступлении дня
    if (!scene.isNight){
        m_passedThisNight = false;
        m_active = false;
        // Убрать лодку за пределы сцены днем
        position = { -999.0f, 0.2f, -999.0f };
        return;
    }

    // Условие запуска: ночь + мост достаточно разведен
    const bool bridgeOpen = (scene.bridgeLift >= 0.95f);
    if (!m_active && !m_passedThisNight && bridgeOpen){
        m_active = true;
        // Движение вдоль реки
        position.x = m_passX;
        position.y = 0.2f;
        position.z = m_startZ;
        // Повернуть модель по направлению движения (+Z)
        rotation.y = 3.1415926f * 0.5f;
    }

    if (!m_active){
        // Лодка вне сцены до ее запуска
        position = { -999.0f, 0.2f, -999.0f };
        return;
    }

    // Движение лодки под мостом (перпендикулярно мосту)
    position.z += m_speed * dt;

    if (position.z >= m_endZ){
        // Завершение ночного прохода лодки
        m_active = false;
        m_passedThisNight = true;
        position = { -999.0f, 0.2f, -999.0f };
    }
}

void Boat::draw(QOpenGLFunctions_3_3_Core* f, const Shader& sh) const
{
    const_cast<Boat*>(this)->ensureUploaded(f);

    Mat4 M = modelMatrix();
    sh.setMat4(f, "uModel", M.data());

    for (const auto& p : m_parts){
        // Параметры материала
        sh.setVec3(f, "uMatKa", p.ka.x, p.ka.y, p.ka.z);
        sh.setVec3(f, "uMatKd", p.kd.x, p.kd.y, p.kd.z);
        sh.setVec3(f, "uMatKs", p.ks.x, p.ks.y, p.ks.z);
        sh.setFloat(f, "uMatNs", p.ns);
        sh.setFloat(f, "uMatOpacity", p.opacity);

        // Текстурные карты
        sh.setInt(f, "uUseTexture", p.useMapKd ? 1 : 0);
        sh.setInt(f, "uHasMapKs",  p.useMapKs ? 1 : 0);
        sh.setInt(f, "uHasMapKn",  p.useMapKn ? 1 : 0);

        if (p.useMapKd && p.mapKd) p.mapKd->bind(f, 0);
        if (p.useMapKs && p.mapKs) p.mapKs->bind(f, 1);
        if (p.useMapKn && p.mapKn) p.mapKn->bind(f, 2);

        p.mesh.draw(f);
    }
}

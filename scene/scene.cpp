#include "scene.h"

#include <algorithm>
#include <random>

#include <QOpenGLFunctions_3_3_Core>
#include <QDir>
#include <QCoreApplication>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QUrl>
#include <QtGlobal>

#include "object.h"
#include "bridge.h"
#include "vehicle.h"
#include "boat.h"

static const char* VS_LIT = R"GLSL(
#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNrm;
layout(location=2) in vec2 aUV;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;
uniform vec2 uUVMul;
uniform vec2 uUVOffset;

out vec3 vPos;
out vec3 vNrm;
out vec2 vUV;
out vec3 vObjPos;
out vec3 vObjNrm;

void main() {
    vec4 wpos = uModel * vec4(aPos, 1.0);
    vPos = wpos.xyz;
    vNrm = mat3(uModel) * aNrm;
    vUV = aUV * uUVMul + uUVOffset;
    // Значения в объектном пространстве используются для box-mapped UV
    // (это предотвращает плавание текстуры при вращении).
    vObjPos = aPos;
    vObjNrm = aNrm;
    gl_Position = uProj * uView * wpos;
}
)GLSL";

namespace {
static void resetAndPlay(QMediaPlayer* p)
{
    if (!p) return;
    p->stop();
    p->setPosition(0);
    p->play();
}
}

Scene::~Scene()
{
    if (m_playerRoad)   m_playerRoad->stop();
    if (m_playerRiver)  m_playerRiver->stop();
    if (m_playerBridge) m_playerBridge->stop();
    if (m_playerBoat)   m_playerBoat->stop();

    delete m_playerRoad;   m_playerRoad = nullptr;
    delete m_playerRiver;  m_playerRiver = nullptr;
    delete m_playerBridge; m_playerBridge = nullptr;
    delete m_playerBoat;   m_playerBoat = nullptr;

    delete m_outRoad;   m_outRoad = nullptr;
    delete m_outRiver;  m_outRiver = nullptr;
    delete m_outBridge; m_outBridge = nullptr;
    delete m_outBoat;   m_outBoat = nullptr;
}

static const char* FS_LIT = R"GLSL(
#version 330 core
in vec3 vPos;
in vec3 vNrm;
in vec2 vUV;
in vec3 vObjPos;
in vec3 vObjNrm;

uniform sampler2D uTex;
uniform vec3 uCamPos;

uniform vec3 uSunDir;
uniform vec3 uSunColor;
uniform float uAmbient;
uniform vec3 uTint;

uniform float uSpecularStrength;
uniform float uSpecularPower;
uniform int uUseTexture;
// Опциональная box-развертка для текстур.
// Если включено, UV генерируются из мировой позиции по доминирующей оси нормали.
uniform int  uUseBoxMap; // 0 = использовать vUV, 1 = box-map из OBJECT-space pos/nrm
uniform vec3 uBoxScale;  // Тайлинг (повторов на единицу объекта) для X, Y, Z

out vec4 FragColor;

void main() {
    vec3 texColor = vec3(1.0);
    if (uUseTexture != 0) {
        vec2 uv = vUV;
        if (uUseBoxMap != 0) {
            // Box mapping: выбираем плоскость проекции по доминирующей оси нормали
            // Используется объектное пространство, чтобы развертка приклеивалась к геометрии при вращении.
            vec3 an = abs(normalize(vObjNrm));
            if (an.y >= an.x && an.y >= an.z) {
                // Верх/низ -> XZ
                uv = vObjPos.xz * vec2(uBoxScale.x, uBoxScale.z);
            } else if (an.x >= an.y && an.x >= an.z){
                // +/-X -> ZY
                uv = vObjPos.zy * vec2(uBoxScale.z, uBoxScale.y);
            } else {
                // +/-Z -> XY
                uv = vObjPos.xy * vec2(uBoxScale.x, uBoxScale.y);
            }
        }
        texColor = texture(uTex, uv).rgb;
    }
    vec3 albedo = texColor * uTint;

    vec3 N = normalize(vNrm);
    vec3 L = normalize(-uSunDir);
    float ndl = max(dot(N, L), 0.0);

    vec3 diff = albedo * uSunColor * ndl;
    vec3 amb  = albedo * uAmbient;

    // Простая зеркальная составляющая
    vec3 V = normalize(uCamPos - vPos);
    vec3 H = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), max(uSpecularPower, 1.0)) * uSpecularStrength;

    vec3 col = amb + diff + spec*uSunColor;
    FragColor = vec4(col, 1.0);
}
)GLSL";

static const char* VS_WATER = R"GLSL(
#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNrm;
layout(location=2) in vec2 aUV;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;
uniform vec2 uUVOffset;

out vec2 vUV;
out vec3 vNrm;

void main() {
    vUV = aUV + uUVOffset;
    vNrm = mat3(uModel) * aNrm;
    gl_Position = uProj * uView * (uModel * vec4(aPos, 1.0));
}
)GLSL";

static const char* FS_WATER = R"GLSL(
#version 330 core
in vec2 vUV;
in vec3 vNrm;

uniform sampler2D uTex;
uniform vec3 uSunDir;
uniform vec3 uSunColor;
uniform float uAmbient;
uniform float uNight;

out vec4 FragColor;

void main() {
    vec3 tex = texture(uTex, vUV).rgb;
    vec3 N = normalize(vNrm);
    vec3 L = normalize(-uSunDir);
    float ndl = max(dot(N, L), 0.0);

    // Оттенок воды
    vec3 albedo = mix(tex, vec3(0.05, 0.08, 0.12), 0.35);
    vec3 col = albedo * (uAmbient + ndl) * uSunColor;

    // Темнее ночью
    col *= mix(1.0, 0.35, uNight);

    FragColor = vec4(col, 1.0);
}
)GLSL";

float Scene::dayNightFactor() const
{
    return nightBlend;
}

void Scene::init(QOpenGLFunctions_3_3_Core* f)
{
    QString log;
    shaderLit.build(f, VS_LIT, FS_LIT, &log);
    shaderWater.build(f, VS_WATER, FS_WATER, &log);

    // Загрузка текстур
    texRoad.load(f, ":/textures/road.png", true);
    texWater.load(f, ":/textures/water.png", false);
    texStone.load(f, ":/textures/stone.png", true);
    texBrick.load(f, ":/textures/brick.png", true);
    texSteel.load(f, ":/textures/steel.png", true);
    texRock.load(f, ":/textures/rock.png", true);
    texBank.load(f, ":/textures/brick.png", true);

    // Создание объектов
    auto br = std::make_unique<Bridge>();
    bridge = br.get();
    objects.push_back(std::move(br));

    // Транспорт (4 полосы: 2 в каждом направлении)
    {
        const float lane0 = -3.1f; // Крайняя левая
        const float lane1 = -1.4f; // Внутренняя левая
        const float lane2 =  1.4f; // Внутренняя правая
        const float lane3 =  3.1f; // Крайняя правая

        const int carsPerDir = 3;
        const float baseY = 2.05f;

        // Случайные цвета машин
        const Vec3 paletteCar[5] = {
            {1.00f, 0.85f, 0.10f}, // Желтый (такси)
            {0.85f, 0.10f, 0.10f}, // Красный
            {0.55f, 0.58f, 0.60f}, // Серебристый металлик
            {0.15f, 0.35f, 0.80f}, // Синий металлик
            {0.18f, 0.18f, 0.18f}  // Темно-серый
        };

        const int totalCars = carsPerDir * 2;
        std::mt19937 rng(1337);
        std::vector<Vec3> carColors;
        carColors.reserve(totalCars);
        for (int i = 0; i < totalCars; ++i) carColors.push_back(paletteCar[i % 5]);
        std::shuffle(carColors.begin(), carColors.end(), rng);
        int colorIdx = 0;

        // Правостороннее движение: направление +X использует правые полосы (2 и 3)
        for (int k = 0; k < carsPerDir; ++k){
            auto car = std::make_unique<Car>(":/models/car.obj");
            car->setTint(carColors[colorIdx++ % totalCars]);
            car->direction = +1.0f;
            car->laneZ = (k % 2 == 0) ? lane2 : lane3;
            car->position = { -45.0f + k * 12.0f, baseY, car->laneZ };
            car->speed = 5.8f + 0.3f * k;
            objects.push_back(std::move(car));
        }

        // Направление -X использует левые полосы (0 и 1)
        for (int k = 0; k < carsPerDir; ++k){
            auto car = std::make_unique<Car>(":/models/car.obj");
            car->setTint(carColors[colorIdx++ % totalCars]);
            car->direction = -1.0f;
            car->laneZ = (k % 2 == 0) ? lane0 : lane1;
            car->position = { 45.0f - k * 12.0f, baseY, car->laneZ };
            car->speed = 5.6f + 0.25f * k;
            objects.push_back(std::move(car));
        }

        // Автобусы: по одному автобусу в каждом направлении, во внутренних полосах
        {
            auto bus = std::make_unique<Bus>(":/models/bus.obj");
            bus->direction = +1.0f;
            bus->laneZ = lane2;
            bus->position = { -30.0f, baseY, bus->laneZ };
            bus->speed = 4.2f;
            objects.push_back(std::move(bus));
        }
        {
            auto bus = std::make_unique<Bus>(":/models/bus.obj");
            bus->direction = -1.0f;
            bus->laneZ = lane1;
            bus->position = { 30.0f, baseY, bus->laneZ };
            bus->speed = 4.0f;
            objects.push_back(std::move(bus));
        }
    }

    // Лодка
    {
        auto boat = std::make_unique<Boat>(":/models/boat.obj");
        boat->position = { -55.0f, 0.2f, 0.0f };
        objects.push_back(std::move(boat));
    }

    // Звук (Qt Multimedia)
    auto mkPlayer = [&](QMediaPlayer*& pl, QAudioOutput*& out, const QUrl& url, float volume, bool loop){
        if (pl) return; // Уже создано
        out = new QAudioOutput();
        out->setVolume(volume);
        pl = new QMediaPlayer();
        pl->setAudioOutput(out);
        pl->setSource(url);
        pl->setLoops(loop ? QMediaPlayer::Infinite : 1);
    };

    mkPlayer(m_playerRoad,   m_outRoad,   QUrl("qrc:/audio/road.mp3"),   0.55f, true);
    mkPlayer(m_playerRiver,  m_outRiver,  QUrl("qrc:/audio/river.mp3"),  0.55f, false);
    mkPlayer(m_playerBridge, m_outBridge, QUrl("qrc:/audio/bridge.mp3"),0.70f, false);
    mkPlayer(m_playerBoat,   m_outBoat,   QUrl("qrc:/audio/boat.mp3"),  0.75f, false);

    if (m_playerRoad) m_playerRoad->play();
}

void Scene::triggerNight()
{
    if (nightPhase != NightPhase::Day) return;
    nightPhase = NightPhase::StoppingTraffic;
    nightTimer = 0.0f;
    isNight = true;
    // Плавный уход в ночь
    nightBlendTarget = 1.0f;
    trafficSpawnEnabled = false;
}

static void enforceVehicleSpacing(Scene& scene)
{
    std::vector<Vehicle*> vehicles;
    vehicles.reserve(scene.objects.size());
    for (auto& o : scene.objects){
        if (auto* v = dynamic_cast<Vehicle*>(o.get())){
            if (v->isActive()) vehicles.push_back(v);
        }
    }

    struct Key { float laneZ; int dir; };
    auto sameLane = [](float a, float b){ return std::fabs(a - b) < 0.001f; };

    std::vector<Key> keys;
    for (auto* v : vehicles){
        Key k{v->laneZ, (v->direction > 0.0f) ? 1 : -1};
        bool exists = false;
        for (auto& kk : keys){
            if (kk.dir == k.dir && sameLane(kk.laneZ, k.laneZ)) { exists = true; break; }
        }
        if (!exists) keys.push_back(k);
    }

    for (auto& k : keys){
        std::vector<Vehicle*> lane;
        for (auto* v : vehicles){
            if (((v->direction > 0.0f) ? 1 : -1) == k.dir && sameLane(v->laneZ, k.laneZ)) lane.push_back(v);
        }

        if (lane.size() < 2) continue;

        if (k.dir > 0){
            std::sort(lane.begin(), lane.end(), [](Vehicle* a, Vehicle* b){ return a->position.x > b->position.x; });
            for (size_t idx = 1; idx < lane.size(); ++idx){
                Vehicle* front = lane[idx-1];
                Vehicle* back  = lane[idx];

                // Дистанция рассчитывается по приблизительным длинам (автобусу нужно больше места)
                // Позиции считаются центрами моделей по оси X
                const float buffer = 0.8f; // Небольшой запас
                float minCenterGap = 0.5f * (front->approxLength() + back->approxLength()) + buffer;
                minCenterGap = std::max(minCenterGap, scene.minVehicleGap);

                float maxX = front->position.x - minCenterGap;
                if (back->position.x > maxX) back->position.x = maxX;
            }
        } else {
            std::sort(lane.begin(), lane.end(), [](Vehicle* a, Vehicle* b){ return a->position.x < b->position.x; });
            for (size_t idx = 1; idx < lane.size(); ++idx){
                Vehicle* front = lane[idx-1];
                Vehicle* back  = lane[idx];

                const float buffer = 0.8f;
                float minCenterGap = 0.5f * (front->approxLength() + back->approxLength()) + buffer;
                minCenterGap = std::max(minCenterGap, scene.minVehicleGap);

                float minX = front->position.x + minCenterGap;
                if (back->position.x < minX) back->position.x = minX;
            }
        }
    }
}

void Scene::update(float dt)
{
    time += dt;

    NightPhase prevPhase = nightPhase;

    {
        const float step = (kNightBlendDuration <= 0.0f) ? 1.0f : (dt / kNightBlendDuration);
        if (nightBlend < nightBlendTarget) {
            nightBlend = std::min(nightBlendTarget, nightBlend + step);
        } else if (nightBlend > nightBlendTarget) {
            nightBlend = std::max(nightBlendTarget, nightBlend - step);
        }
    }

    // Активация сценария ночи
    const float liftSpeed = 0.35f;    // Скорость подъема (единиц в секунду)
    const float preLiftWait = 3.0f;   // Секунд ожидания перед подъемом
    const float openHold = 12.0f;     // Сколько держать в открытом состоянии (достаточно для прохода лодки)
    const float postCloseWait = 1.0f; // Секунд ожидания после закрытия

    switch (nightPhase){
        case NightPhase::Day:
            isNight = false;
            bridgeLift = 0.0f;
            trafficSpawnEnabled = true;
            nightBlendTarget = 0.0f;
            break;
        case NightPhase::StoppingTraffic:
            isNight = true;
            bridgeLift = 0.0f;
            trafficSpawnEnabled = false;
            // Ожидание, пока передняя машина в каждой полосе дойдет до своей стоп-линии
            // Машины позади остановятся раньше из-за ограничения дистанции
            {
                auto sameLane = [](float a, float b){ return std::fabs(a - b) < 0.001f; };
                struct LaneKey { float laneZ; int dir; };

                std::vector<Vehicle*> vehicles;
                vehicles.reserve(objects.size());
                for (auto& o : objects){
                    if (auto* v = dynamic_cast<Vehicle*>(o.get())){
                        if (v->isActive()) vehicles.push_back(v);
                    }
                }

                std::vector<LaneKey> keys;
                for (auto* v : vehicles){
                    LaneKey k{v->laneZ, (v->direction > 0.0f) ? 1 : -1};
                    bool exists = false;
                    for (auto& kk : keys){
                        if (kk.dir == k.dir && sameLane(kk.laneZ, k.laneZ)) { exists = true; break; }
                    }
                    if (!exists) keys.push_back(k);
                }

                const float eps = 0.06f;
                bool allFrontStopped = true;
                for (auto& k : keys){
                    Vehicle* front = nullptr;
                    for (auto* v : vehicles){
                        if (((v->direction > 0.0f) ? 1 : -1) != k.dir) continue;
                        if (!sameLane(v->laneZ, k.laneZ)) continue;
                        if (!front) { front = v; continue; }
                        if (k.dir > 0){
                            if (v->position.x > front->position.x) front = v;
                        } else {
                            if (v->position.x < front->position.x) front = v;
                        }
                    }

                    if (!front) continue; // В этой полосе нет транспорта
                    if (k.dir > 0){
                        if (std::fabs(front->position.x - stopLinePosX) > eps) { allFrontStopped = false; break; }
                    } else {
                        if (std::fabs(front->position.x - stopLineNegX) > eps) { allFrontStopped = false; break; }
                    }
                }

                if (allFrontStopped){
                    nightPhase = NightPhase::WaitBeforeLift;
                    nightTimer = preLiftWait;
                }
            }
            break;
        case NightPhase::WaitBeforeLift:
            isNight = true;
            bridgeLift = 0.0f;
            trafficSpawnEnabled = false;
            nightTimer -= dt;
            if (nightTimer <= 0.0f){
                nightPhase = NightPhase::LiftingOpen;
            }
            break;
        case NightPhase::LiftingOpen:
            isNight = true;
            trafficSpawnEnabled = false;
            bridgeLift += liftSpeed * dt;
            if (bridgeLift >= 1.0f){
                bridgeLift = 1.0f;
                nightPhase = NightPhase::HoldOpen;
                nightTimer = openHold;
            }
            break;
        case NightPhase::HoldOpen:
            isNight = true;
            trafficSpawnEnabled = false;
            bridgeLift = 1.0f;
            nightTimer -= dt;
            if (nightTimer <= 0.0f){
                nightPhase = NightPhase::Closing;
            }
            break;
        case NightPhase::Closing:
            isNight = true;
            trafficSpawnEnabled = false;
            bridgeLift -= liftSpeed * dt;
            if (bridgeLift <= 0.0f){
                bridgeLift = 0.0f;
                nightPhase = NightPhase::WaitAfterClose;
                nightTimer = postCloseWait;
            }
            break;
        case NightPhase::WaitAfterClose:
            isNight = true;
            trafficSpawnEnabled = false;
            bridgeLift = 0.0f;
            nightTimer -= dt;
            if (nightTimer <= 0.0f){
                // Запуск плавного возврата ко дню (3 секунды)
                nightPhase = NightPhase::FadeToDay;
                nightBlendTarget = 0.0f;
            }
            break;
        case NightPhase::FadeToDay:
            // Плавное возвращение ко дню, при этом трафик уже возобновляется
            // Трафик включен, чтобы машины ехали сразу после закрытия моста
            isNight = true;
            trafficSpawnEnabled = true;
            bridgeLift = 0.0f;
            if (nightBlend <= 0.001f){
                nightPhase = NightPhase::Day;
                isNight = false;
                trafficSpawnEnabled = true;
            }
            break;
    }

    // Управление звуком
    if (prevPhase != nightPhase){
        // Начинается движение моста
        if (nightPhase == NightPhase::LiftingOpen){
            if (m_playerRoad) m_playerRoad->pause();
            if (m_playerRiver) m_playerRiver->stop();
            resetAndPlay(m_playerBridge);
        }
        // Мост полностью открыт
        if (nightPhase == NightPhase::HoldOpen){
            // Шум дороги на паузе, запуск шума реки
            resetAndPlay(m_playerRiver);
        }
        // Начинается закрытие
        if (nightPhase == NightPhase::Closing){
            if (m_playerRiver) m_playerRiver->stop();
            resetAndPlay(m_playerBridge);
        }
        // Мост закрыт - возвращается звук дороги
        if (nightPhase == NightPhase::WaitAfterClose || nightPhase == NightPhase::FadeToDay || nightPhase == NightPhase::Day){
            if (m_playerRoad) m_playerRoad->play();
        }
        // Возвращение ко дню
        if (nightPhase == NightPhase::Day){
            if (m_playerRiver) m_playerRiver->stop();
        }
    }

    // Направление/цвет солнца - интерполяция по nightBlend
    {
        const Vec3 dayDir = normalize(Vec3{-0.4f, -1.0f, -0.2f});
        const Vec3 nightDir = normalize(Vec3{0.2f, -1.0f, 0.15f});
        const Vec3 dayCol{1.0f, 0.98f, 0.92f};
        const Vec3 nightCol{0.45f, 0.55f, 0.8f};
        const float dayAmb = 0.28f;
        const float nightAmb = 0.10f;

        // Линейное смешивание
        light.sunDir = normalize(dayDir * (1.0f - nightBlend) + nightDir * nightBlend);
        light.sunColor = dayCol * (1.0f - nightBlend) + nightCol * nightBlend;
        light.ambient = dayAmb * (1.0f - nightBlend) + nightAmb * nightBlend;
    }

    // Прокрутка воды
    waterUVOffset.x += dt * 0.002f;
    waterUVOffset.y += dt * 0.03f;
    if (waterUVOffset.x > 1000) waterUVOffset.x -= 1000;
    if (waterUVOffset.y > 1000) waterUVOffset.y -= 1000;

    // Обновление объектов
    for (auto& o : objects) o->update(*this, dt);

    // Одиночный гудок лодки при проходе центра
    {
        Boat* boat = nullptr;
        for (auto& o : objects){
            boat = dynamic_cast<Boat*>(o.get());
            if (boat) break;
        }
        if (boat && boat->isActive()){
            float z = boat->position.z;
            if (!m_boatFxPlayed){
                if ((m_lastBoatZ < 0.0f && z >= 0.0f) || (std::fabs(z) < 0.25f)){
                    resetAndPlay(m_playerBoat);
                    m_boatFxPlayed = true;
                }
            }
            m_lastBoatZ = z;
        } else {
            m_boatFxPlayed = false;
            m_lastBoatZ = 0.0f;
        }
    }

    // Контроль дистанции трафика
    enforceVehicleSpacing(*this);
}

void Scene::draw(QOpenGLFunctions_3_3_Core* f, int w, int h)
{
    float aspect = (h == 0) ? 1.0f : float(w)/float(h);
    Mat4 V = cam.view();
    Mat4 P = cam.proj(aspect);
    Vec3 camPos = cam.eye();

    // Отрисовка всех объектов
    for (auto& o : objects){
        // Мост рисуется отдельно (дорожное полотно и вода - разными шейдерами)
        (void)o;
    }

    // Рисование в два прохода: сначала непрозрачные объекты (мост, транспорт, лодка),
    // затем вода (все еще непрозрачная, но с отдельным шейдером)
    shaderLit.use(f);
    // Управление UV по умолчанию для шейдера освещения
    {
        int locMul = f->glGetUniformLocation(shaderLit.id(), "uUVMul");
        int locOff = f->glGetUniformLocation(shaderLit.id(), "uUVOffset");
        if (locMul >= 0) f->glUniform2f(locMul, 1.0f, 1.0f);
        if (locOff >= 0) f->glUniform2f(locOff, 0.0f, 0.0f);
    }
    shaderLit.setMat4(f, "uView", V.data());
    shaderLit.setMat4(f, "uProj", P.data());
    shaderLit.setVec3(f, "uCamPos", camPos.x, camPos.y, camPos.z);
    shaderLit.setVec3(f, "uSunDir", light.sunDir.x, light.sunDir.y, light.sunDir.z);
    shaderLit.setVec3(f, "uSunColor", light.sunColor.x, light.sunColor.y, light.sunColor.z);
    shaderLit.setFloat(f, "uAmbient", light.ambient);
    shaderLit.setFloat(f, "uSpecularStrength", 0.25f);
    shaderLit.setFloat(f, "uSpecularPower", 32.0f);
    shaderLit.setVec3(f, "uTint", 1.0f, 1.0f, 1.0f);
    shaderLit.setInt(f, "uUseTexture", 1);
    // По умолчанию используется UV из меша
    shaderLit.setInt(f, "uUseBoxMap", 0);
    shaderLit.setVec3(f, "uBoxScale", 1.0f, 1.0f, 1.0f);
    shaderLit.setInt(f, "uTex", 0);

    // Непрозрачные объекты, кроме воды (мост сам отдельно вызовет drawOpaque)
    for (auto& o : objects){
        if (auto br = dynamic_cast<Bridge*>(o.get())){
            br->drawOpaque(f, shaderLit, texRoad, texStone, texBrick, texSteel, texRock, texBank);
        } else {
            o->draw(f, shaderLit);
        }
    }

    // Проход для воды
    shaderWater.use(f);
    shaderWater.setMat4(f, "uView", V.data());
    shaderWater.setMat4(f, "uProj", P.data());
    shaderWater.setVec3(f, "uSunDir", light.sunDir.x, light.sunDir.y, light.sunDir.z);
    shaderWater.setVec3(f, "uSunColor", light.sunColor.x, light.sunColor.y, light.sunColor.z);
    shaderWater.setFloat(f, "uAmbient", light.ambient);
    shaderWater.setFloat(f, "uNight", dayNightFactor());
    shaderWater.setInt(f, "uTex", 0);

    for (auto& o : objects){
        if (auto br = dynamic_cast<Bridge*>(o.get())){
            br->drawWater(f, shaderWater, texWater, waterUVOffset);
        }
    }
}

static std::array<float,4> mulMat4Vec4(const Mat4& m, float x, float y, float z, float w)
{
    const auto& a = m.m;
    return {
        a[0]*x + a[4]*y + a[8]*z  + a[12]*w,
        a[1]*x + a[5]*y + a[9]*z  + a[13]*w,
        a[2]*x + a[6]*y + a[10]*z + a[14]*w,
        a[3]*x + a[7]*y + a[11]*z + a[15]*w
    };
}

void Scene::handleClick(int x, int y, int viewportW, int viewportH)
{
    if (viewportW <= 0 || viewportH <= 0) return;

    const float aspect = float(viewportW) / float(viewportH);
    const Mat4 V = cam.view();
    const Mat4 P = cam.proj(aspect);
    const Mat4 VP = P * V;

    Vehicle* best = nullptr;
    float bestDist2 = 1e30f;

    for (auto& obj : objects)
    {
        auto* v = dynamic_cast<Vehicle*>(obj.get());
        if (!v) continue;

        const Vec3 wp = v->position;
        auto clip = mulMat4Vec4(VP, wp.x, wp.y, wp.z, 1.0f);
        if (clip[3] <= 0.0001f) continue; // За камерой

        const float invW = 1.0f / clip[3];
        const float ndcX = clip[0] * invW;
        const float ndcY = clip[1] * invW;

        // За пределами видимой области
        if (ndcX < -1.2f || ndcX > 1.2f || ndcY < -1.2f || ndcY > 1.2f) continue;

        const float sx = (ndcX * 0.5f + 0.5f) * float(viewportW);
        const float sy = (1.0f - (ndcY * 0.5f + 0.5f)) * float(viewportH);

        // Радиус выбора в мировых единицах (грубая оценка по длине транспорта)
        const float rW = std::max(0.35f, 0.55f * v->approxLength());
        auto clipR = mulMat4Vec4(VP, wp.x + rW, wp.y, wp.z, 1.0f);
        float rPx = 18.0f;
        if (clipR[3] > 0.0001f){
            const float invWR = 1.0f / clipR[3];
            const float ndcXR = clipR[0] * invWR;
            const float sxR = (ndcXR * 0.5f + 0.5f) * float(viewportW);
            rPx = std::max(10.0f, std::abs(sxR - sx));
        }

        const float dx = float(x) - sx;
        const float dy = float(y) - sy;
        const float d2 = dx*dx + dy*dy;

        if (d2 <= rPx*rPx && d2 < bestDist2){
            bestDist2 = d2;
            best = v;
        }
    }

    if (!best) return;

    // Определение типа транспорта через dynamic_cast
    if (dynamic_cast<Car*>(best)) audioOnCarClicked();
    else if (dynamic_cast<Bus*>(best)) audioOnBusClicked();
}

void Scene::audioPlayClickFx(const QString& file)
{
    if (!m_playerClick){
        m_audioOutClick = new QAudioOutput();
        m_audioOutClick->setVolume(0.75f);
        m_playerClick = new QMediaPlayer();
        m_playerClick->setAudioOutput(m_audioOutClick);
    }

    m_playerClick->stop();
    // Проигрывается как Qt-ресурс: qrc:/audio/<file>
    m_playerClick->setSource(QUrl(QStringLiteral("qrc:/audio/") + file));
    m_playerClick->play();
}

void Scene::audioOnCarClicked()
{
    // Цикличное проигрывание car-1..car-3
    const int idx = (m_carClickIdx % 3) + 1;
    m_carClickIdx = (m_carClickIdx + 1) % 3;
    audioPlayClickFx(QString("car-%1.mp3").arg(idx));
}

void Scene::audioOnBusClicked()
{
    // Цикличное проигрывание bus-1..bus-3
    const int idx = (m_busClickIdx % 3) + 1;
    m_busClickIdx = (m_busClickIdx + 1) % 3;
    audioPlayClickFx(QString("bus-%1.mp3").arg(idx));
}

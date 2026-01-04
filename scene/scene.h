#ifndef SCENE_H
#define SCENE_H

#include <memory>
#include <vector>

#include "camera.h"
#include "object.h"
#include "core/math3d.h"
#include "core/shader.h"
#include "core/texture.h"

class Bridge;
class QAudioOutput;
class QMediaPlayer;
class QOpenGLFunctions_3_3_Core;

struct Lighting {
    Vec3 sunDir{ -0.4f, -1.0f, -0.2f }; // Направление от источника света к сцене
    Vec3 sunColor{ 1.0f, 0.98f, 0.92f };
    float ambient = 0.25f;
};

class Scene
{
public:
    ~Scene();
    Camera cam;
    Lighting light;

    float time = 0.0f; // Время сцены в секундах

    float nightBlend = 0.0f;
    float nightBlendTarget = 0.0f;
    static constexpr float kNightBlendDuration = 3.0f; // Секунды

    bool isNight = false; // Используется для пуска/останова трафика

    // Прокрутка воды
    Vec2 waterUVOffset{0,0};

    // Состояние моста
    float bridgeLift = 0.0f;
    bool bridgeOpening = false;

    // Сценарий ночи после нажатия пробела
    enum class NightPhase { Day, StoppingTraffic, WaitBeforeLift, LiftingOpen, HoldOpen, Closing, WaitAfterClose, FadeToDay };
    NightPhase nightPhase = NightPhase::Day;
    float nightTimer = 0.0f;
    bool trafficSpawnEnabled = true;

    // Стоп-линии трафика перед разводом (для обоих направлений)
    float stopLinePosX = -9.0f; // Транспорт, движущийся в сторону +X, останавливается на этом X
    float stopLineNegX = +9.0f; // Транспорт, движущийся в сторону -X, останавливается на этом X
    float minVehicleGap = 3.2f; // Минимальная дистанция по X между машинами в одной полосе

    bool canTriggerNight() const { return nightPhase == NightPhase::Day; }
    void triggerNight();

    // Ресурсные файлы
    Shader shaderLit;
    Shader shaderWater;

    Texture texRoad;
    Texture texWater;

    Texture texStone;
    Texture texBrick;
    Texture texSteel;
    Texture texRock;
    Texture texBank;

    // Аудио
    QMediaPlayer* m_playerRoad   = nullptr;
    QMediaPlayer* m_playerRiver  = nullptr;
    QMediaPlayer* m_playerBridge = nullptr;
    QMediaPlayer* m_playerBoat   = nullptr;

    // Звуковые эффекты (циклично)
    QAudioOutput* m_audioOutClick = nullptr;
    QMediaPlayer* m_playerClick   = nullptr;
    int m_carClickIdx = 0;
    int m_busClickIdx = 0;
    QString m_audioBase;

    QAudioOutput* m_outRoad   = nullptr;
    QAudioOutput* m_outRiver  = nullptr;
    QAudioOutput* m_outBridge = nullptr;
    QAudioOutput* m_outBoat   = nullptr;

    NightPhase m_prevNightPhase = NightPhase::Day;
    bool m_boatFxPlayed = false;
    float m_lastBoatZ = 0.0f;

    std::vector<std::unique_ptr<Object>> objects;
    Bridge* bridge = nullptr;

    void init(QOpenGLFunctions_3_3_Core* f);
    void update(float dt);
    void handleClick(int x, int y, int viewportW, int viewportH);
    void draw(QOpenGLFunctions_3_3_Core* f, int w, int h);

    // Вспомогательные методы
    float dayNightFactor() const; // 0 - день -> 1 - ночь
    void initAudio();
    void updateAudio(float dt);

    // Вспомогательные методы для цикличных SFX
    void audioPlayClickFx(const QString& file);
    void audioOnCarClicked();
    void audioOnBusClicked();
};

#endif // SCENE_H

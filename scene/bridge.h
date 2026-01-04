#ifndef BRIDGE_H
#define BRIDGE_H

#include "object.h"
#include "core/mesh.h"
#include "core/texture.h"

class Scene;
class Shader;
class QOpenGLFunctions_3_3_Core;

// Стилизованный Володарский мост:
// - Неподвижные подъезды и центральный разводной пролет;
// - Четыре арочных пролета по обеим сторонам полотна;
// - Плоскость воды (анимация смещения UV-координат).

class Bridge : public Object
{
public:
    Bridge();

    void update(Scene& scene, float dt) override;
    void draw(QOpenGLFunctions_3_3_Core* f, const Shader& sh) const override;

    // Вспомогательные методы для отрисовки с текстурами/шейдерами
    void drawOpaque(QOpenGLFunctions_3_3_Core* f, const Shader& sh,
                    const Texture& roadTex,
                    const Texture& stoneTex,
                    const Texture& brickTex,
                    const Texture& steelTex,
                    const Texture& rockTex,
                    const Texture& bankTex) const;
    void drawWater(QOpenGLFunctions_3_3_Core* f, const Shader& sh, const Texture& waterTex, const Vec2& uvOffset) const;

    void buildGeometry(QOpenGLFunctions_3_3_Core* f);

private:
    Mesh makeBox(QOpenGLFunctions_3_3_Core* f, float sx, float sy, float sz, const Vec2& uvScale);

    Mesh makePierBox(QOpenGLFunctions_3_3_Core* f, float sx, float sy, float sz);
    Mesh makeBankBox(QOpenGLFunctions_3_3_Core* f, float sx, float sy, float sz, const Vec2& uvTopScale, const Vec2& uvSideScale);
    Mesh makeRibUnit(QOpenGLFunctions_3_3_Core* f);

    void makeLeaf(QOpenGLFunctions_3_3_Core* f);
    void makeCurbs(QOpenGLFunctions_3_3_Core* f);
    void makeArches(QOpenGLFunctions_3_3_Core* f);
    void makePiers(QOpenGLFunctions_3_3_Core* f);
    void makeWater(QOpenGLFunctions_3_3_Core* f);
    void makeBanks(QOpenGLFunctions_3_3_Core* f);

private:
    Mesh m_leaf;
    Mesh m_curb;
    Mesh m_archUnit; // Базовая арка единичного радиуса для матрицы преобразований
    Mesh m_ribUnit;  // Базовый вертикальный параллелепипед для опор под аркой
    Mesh m_pier;
    Mesh m_water;
    Mesh m_bank;

    // Кешированное состояние из Scene
    float m_lift = 0.0f;
};

#endif // BRIDGE_H

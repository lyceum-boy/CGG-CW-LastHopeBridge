#include "bridge.h"

#include <algorithm>
#include <cmath>

#include <QOpenGLFunctions_3_3_Core>

#include "scene.h"
#include "core/shader.h"

static void addQuad(std::vector<Vertex>& v, std::vector<unsigned>& i,
                    const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& d,
                    const Vec3& n, const Vec2& uva, const Vec2& uvb, const Vec2& uvc, const Vec2& uvd)
{
    unsigned start = (unsigned)v.size();
    v.push_back({a,n,uva});
    v.push_back({b,n,uvb});
    v.push_back({c,n,uvc});
    v.push_back({d,n,uvd});
    i.insert(i.end(), {start, start+1, start+2, start, start+2, start+3});
}

Bridge::Bridge()
{
    position = {0,0,0};
    rotation = {0,0,0};
    scale    = {1,1,1};
}

void Bridge::buildGeometry(QOpenGLFunctions_3_3_Core* f)
{
    makeLeaf(f);
    makeCurbs(f);
    makeArches(f);
    makePiers(f);
    makeWater(f);
    makeBanks(f);
}

Mesh Bridge::makeBox(QOpenGLFunctions_3_3_Core* f, float sx, float sy, float sz, const Vec2& uvScale)
{
    std::vector<Vertex> v;
    std::vector<unsigned> i;
    Vec3 p000{-sx, -sy, -sz}, p001{-sx,-sy, sz}, p010{-sx, sy,-sz}, p011{-sx, sy, sz};
    Vec3 p100{ sx, -sy, -sz}, p101{ sx,-sy, sz}, p110{ sx, sy,-sz}, p111{ sx, sy, sz};

    addQuad(v,i, p100,p101,p111,p110, {1,0,0}, {0,0},{uvScale.x,0},{uvScale.x,uvScale.y},{0,uvScale.y});  // +X
    addQuad(v,i, p000,p010,p011,p001, {-1,0,0}, {0,0},{uvScale.x,0},{uvScale.x,uvScale.y},{0,uvScale.y}); // -X

    // +Y (верх): поворот UV так, чтобы разметка дороги шла вдоль оси X
    // U вдоль Z (uvScale.y), V вдоль X (uvScale.x)
    addQuad(v,i, p010,p110,p111,p011, {0,1,0}, {0,0},{0,uvScale.x}, {uvScale.y,uvScale.x}, {uvScale.y,0});
    addQuad(v,i, p000,p001,p101,p100, {0,-1,0}, {0,0},{uvScale.x,0},{uvScale.x,uvScale.y},{0,uvScale.y}); // -Y

    addQuad(v,i, p001,p011,p111,p101, {0,0,1}, {0,0},{uvScale.x,0},{uvScale.x,uvScale.y},{0,uvScale.y});  // +Z
    addQuad(v,i, p000,p100,p110,p010, {0,0,-1}, {0,0},{uvScale.x,0},{uvScale.x,uvScale.y},{0,uvScale.y}); // -Z

    Mesh m; m.upload(f, v, i);
    return m;
}

Mesh Bridge::makeBankBox(QOpenGLFunctions_3_3_Core* f, float sx, float sy, float sz,
                         const Vec2& uvTopScale, const Vec2& uvSideScale)
{
    // Аналогично makeBox(), но плотное повторение текстуры только на верхней грани.
    // Боковые грани повторены реже во избежание замыленного или растянутого вида.
    std::vector<Vertex> v;
    std::vector<unsigned> i;

    auto addQuad = [&](Vec3 a, Vec3 b, Vec3 c, Vec3 d, Vec3 n, Vec2 ta, Vec2 tb, Vec2 tc, Vec2 td){
        unsigned base = (unsigned)v.size();
        v.push_back({a,n,ta});
        v.push_back({b,n,tb});
        v.push_back({c,n,tc});
        v.push_back({d,n,td});
        i.insert(i.end(), {base, base+1, base+2, base, base+2, base+3});
    };

    // Верх (+Y): сохраняются повернутые UV как в makeBox (U вдоль X, V вдоль Z)
    addQuad({-sx,+sy,-sz},{+sx,+sy,-sz},{+sx,+sy,+sz},{-sx,+sy,+sz},{0,1,0},
            {0,0},{0,uvTopScale.y},{uvTopScale.x,uvTopScale.y},{uvTopScale.x,0});

    // Низ (-Y): почти не виден, оставлено минимальное повторение
    addQuad({-sx,-sy,+sz},{+sx,-sy,+sz},{+sx,-sy,-sz},{-sx,-sy,-sz},{0,-1,0},
            {0,0},{uvSideScale.x,0},{uvSideScale.x,uvSideScale.y},{0,uvSideScale.y});

    // Тайлинг на боковых гранях: V вдоль высоты, U по горизонтальному размеру соответствующей грани
    const float uZ = uvSideScale.x * (sz / sx); // Для граней, где U соответствует оси Z (длинная сторона)
    const float vY = uvSideScale.y;

    // Грани +Z / -Z: U вдоль X, V вдоль Y
    addQuad({-sx,-sy,+sz},{+sx,-sy,+sz},{+sx,+sy,+sz},{-sx,+sy,+sz},{0,0,1},
            {0,0},{uvSideScale.x,0},{uvSideScale.x,uvSideScale.y},{0,uvSideScale.y});
    addQuad({+sx,-sy,-sz},{-sx,-sy,-sz},{-sx,+sy,-sz},{+sx,+sy,-sz},{0,0,-1},
            {0,0},{uvSideScale.x,0},{uvSideScale.x,uvSideScale.y},{0,uvSideScale.y});

    // Грани +X / -X: U вдоль Z, V вдоль Y
    addQuad({+sx,-sy,+sz},{+sx,-sy,-sz},{+sx,+sy,-sz},{+sx,+sy,+sz},{1,0,0},
            {0,0},{uZ,0},{uZ,vY},{0,vY});
    addQuad({-sx,-sy,-sz},{-sx,-sy,+sz},{-sx,+sy,+sz},{-sx,+sy,-sz},{-1,0,0},
            {0,0},{uZ,0},{uZ,vY},{0,vY});

    Mesh m; m.upload(f, v, i);
    return m;
}

Mesh Bridge::makePierBox(QOpenGLFunctions_3_3_Core* f, float sx, float sy, float sz)
{
    // Аналогично makeBox(), но со специальными UV: для длинных боковых
    // граней (нормаль +/-X) 2 повтора вдоль Z, чтобы избежать растяжения
    std::vector<Vertex> v;
    std::vector<unsigned> i;
    Vec3 p000{-sx, -sy, -sz}, p001{-sx,-sy, sz}, p010{-sx, sy,-sz}, p011{-sx, sy, sz};
    Vec3 p100{ sx, -sy, -sz}, p101{ sx,-sy, sz}, p110{ sx, sy,-sz}, p111{ sx, sy, sz};

    const float uLong = 2.0f; // 2 повтора вдоль Z на длинных боковых гранях
    const float vH = 1.0f;    // 1 повтор по высоте

    // +X (U вдоль Z, V вдоль Y) => длинная грань
    addQuad(v,i, p100,p101,p111,p110, {1,0,0}, {0,0},{uLong,0},{uLong,vH},{0,vH});
    // -X (длинная грань), первая грань должна идти вдоль +Z, иначе на этой грани U/V поменяются
    addQuad(v,i, p000,p001,p011,p010, {-1,0,0}, {0,0},{uLong,0},{uLong,vH},{0,vH});

    // Остальные грани с UV 1x1
    const float u1 = 1.0f, v1 = 1.0f;

    // +Y (верх) - 2 повтора вдоль длинного направления Z (по V)
    addQuad(v,i, p010,p110,p111,p011, {0,1,0}, {0,0},{u1,0},{u1,2.0f},{0,2.0f});
    // -Y (низ) - то же повторение
    addQuad(v,i, p000,p001,p101,p100, {0,-1,0}, {0,0},{u1,0},{u1,2.0f},{0,2.0f});

    addQuad(v,i, p001,p011,p111,p101, {0,0,1}, {0,0},{u1,0},{u1,v1},{0,v1});  // +Z
    addQuad(v,i, p000,p100,p110,p010, {0,0,-1}, {0,0},{u1,0},{u1,v1},{0,v1}); // -Z

    Mesh m; m.upload(f, v, i);
    return m;
}

// Для вертикальных ребер для всех боковых граней V соответствует Y (высота),
// а U соответствует горизонтальной оси (X или Z).
Mesh Bridge::makeRibUnit(QOpenGLFunctions_3_3_Core* f)
{
    std::vector<Vertex> v;
    std::vector<unsigned> i;

    // Куб единичного размера в локальном пространстве (масштабируется при отрисовке)
    float sx = 0.5f, sy = 0.5f, sz = 0.5f;
    Vec3 p000{-sx,-sy,-sz}, p001{-sx,-sy, sz}, p010{-sx, sy,-sz}, p011{-sx, sy, sz};
    Vec3 p100{ sx,-sy,-sz}, p101{ sx,-sy, sz}, p110{ sx, sy,-sz}, p111{ sx, sy, sz};

    auto uv_from_yz = [&](const Vec3& p){ return Vec2{ (p.z + sz) / (2*sz), (p.y + sy) / (2*sy) }; }; // U=Z, V=Y
    auto uv_from_yx = [&](const Vec3& p){ return Vec2{ (p.x + sx) / (2*sx), (p.y + sy) / (2*sy) }; }; // U=X, V=Y
    auto uv_from_xz = [&](const Vec3& p){ return Vec2{ (p.x + sx) / (2*sx), (p.z + sz) / (2*sz) }; }; // top/bottom

    // Грань +X: U из Z, V из Y
    addQuad(v,i, p100,p101,p111,p110, {1,0,0}, uv_from_yz(p100),uv_from_yz(p101),uv_from_yz(p111),uv_from_yz(p110));
    // Грань -X
    addQuad(v,i, p000,p010,p011,p001, {-1,0,0}, uv_from_yz(p000),uv_from_yz(p010),uv_from_yz(p011),uv_from_yz(p001));
    // Грань +Z: U из X, V из Y
    addQuad(v,i, p001,p011,p111,p101, {0,0,1}, uv_from_yx(p001),uv_from_yx(p011),uv_from_yx(p111),uv_from_yx(p101));
    // Грань -Z
    addQuad(v,i, p000,p100,p110,p010, {0,0,-1}, uv_from_yx(p000),uv_from_yx(p100),uv_from_yx(p110),uv_from_yx(p010));
    // Верх +Y (не виден): U из X, V из Z
    addQuad(v,i, p010,p110,p111,p011, {0,1,0}, uv_from_xz(p010),uv_from_xz(p110),uv_from_xz(p111),uv_from_xz(p011));
    // Низ -Y
    addQuad(v,i, p000,p001,p101,p100, {0,-1,0}, uv_from_xz(p000),uv_from_xz(p001),uv_from_xz(p101),uv_from_xz(p100));

    Mesh m; m.upload(f, v, i);
    return m;
}

void Bridge::makeLeaf(QOpenGLFunctions_3_3_Core* f)
{
    // Полотно моста (разводная часть / сегмент дороги)
    m_leaf = makeBox(f, 12.0f, 0.25f, 4.5f, {6.0f, 2.0f});
}

void Bridge::makeCurbs(QOpenGLFunctions_3_3_Core* f)
{
    const float hx = 12.0f;
    const float hy = 0.45f;
    const float hz = 0.45f;
    const float repX = 6.0f; // Повторы вдоль длинной оси бордюра (X)
    const float repY = 1.0f;
    const float repZ = 1.0f;

    std::vector<Vertex> v;
    std::vector<unsigned> i;

    Vec3 p000{-hx, -hy, -hz}, p001{-hx,-hy, hz}, p010{-hx, hy,-hz}, p011{-hx, hy, hz};
    Vec3 p100{ hx, -hy, -hz}, p101{ hx,-hy, hz}, p110{ hx, hy,-hz}, p111{ hx, hy, hz};

    // Торец +X (Z x Y)
    addQuad(v,i, p100,p101,p111,p110, {1,0,0},  {0,0},{repZ,0},{repZ,repY},{0,repY});
    // Торец -X
    addQuad(v,i, p000,p010,p011,p001, {-1,0,0}, {0,0},{repZ,0},{repZ,repY},{0,repY});

    // Верх +Y (X x Z): U вдоль X, V вдоль Z
    addQuad(v,i, p010,p110,p111,p011, {0,1,0},  {0,0},{repX,0},{repX,repZ},{0,repZ});
    // Низ -Y (X x Z): U вдоль X, V вдоль Z
    addQuad(v,i, p000,p100,p101,p001, {0,-1,0}, {0,0},{repX,0},{repX,repZ},{0,repZ});

    // Внешняя сторона +Z (X x Y)
    addQuad(v,i, p001,p011,p111,p101, {0,0,1},  {0,0},{repX,0},{repX,repY},{0,repY});
    // Внутренняя сторона -Z
    addQuad(v,i, p000,p100,p110,p010, {0,0,-1}, {0,0},{repX,0},{repX,repY},{0,repY});

    m_curb.upload(f, v, i);
}

void Bridge::makePiers(QOpenGLFunctions_3_3_Core* f)
{
    // Опоры моста - два параллелепипеда в реке
    m_pier = makePierBox(f, 1.75f, 2.0f, 4.7f);
}

void Bridge::makeWater(QOpenGLFunctions_3_3_Core* f)
{
    // Большая плоскость воды
    std::vector<Vertex> v;
    std::vector<unsigned> i;
    float sx = 120.0f;
    float sz = 80.0f;
    Vec3 n{0,1,0};
    v.push_back({{-sx,0,-sz}, n, {0,0}});
    v.push_back({{ sx,0,-sz}, n, {30,0}});
    v.push_back({{ sx,0, sz}, n, {30,20}});
    v.push_back({{-sx,0, sz}, n, {0,20}});
    i = {0,2,1, 0,3,2};
    m_water.upload(f, v, i);
}

void Bridge::makeBanks(QOpenGLFunctions_3_3_Core* f)
{
    // Два простых берега по краям моста
    // Для верхней грани используем более плотный тайлинг,
    // для боковых - меньше повторов, чтобы избежать деформаций текстуры
    m_bank = makeBankBox(f, 22.0f, 1.5f, 28.0f, {7.0f, 7.0f}, {6.0f, 0.5f});
}

void Bridge::makeArches(QOpenGLFunctions_3_3_Core* f)
{
    // Строится единичная арка (радиуса 1) в плоскости X–Y с выдавливанием вдоль Z
    // Затем рисуются 4 экземпляра с разными преобразованиями (два пролета, две стороны)
    std::vector<Vertex> v;
    std::vector<unsigned> ind;

    const int seg = 32;
    const float radiusOuter = 1.0f;
    const float thickness = 0.03f; // Относительная толщина кольца
    const float radiusInner = radiusOuter - thickness;
    const float depth = 0.18f; // Полуглубина вдоль Z (в единичном пространстве)

    auto ring = [&](float radius, float z)->std::vector<Vec3>{
        std::vector<Vec3> pts;
        pts.reserve(seg+1);
        for (int k=0;k<=seg;k++){
            float t = float(k)/float(seg);
            float ang = 3.1415926f * t;
            float x = std::cos(ang) * radius;
            float y = std::sin(ang) * radius * 0.666f; // Сплюснутая арка (овал)
            pts.push_back({x,y,z});
        }
        return pts;
    };

    auto emitQuad = [&](const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& d){
        Vec3 n = normalize(cross(b-a, d-a));
        unsigned start = (unsigned)v.size();
        v.push_back({a,n,{0,0}});
        v.push_back({b,n,{1,0}});
        v.push_back({c,n,{1,1}});
        v.push_back({d,n,{0,1}});
        ind.insert(ind.end(), {start,start+1,start+2, start,start+2,start+3});
    };

    auto outerFront = ring(radiusOuter, -depth);
    auto outerBack  = ring(radiusOuter, +depth);
    auto innerFront = ring(radiusInner, -depth);
    auto innerBack  = ring(radiusInner, +depth);

    // Внешняя поверхность
    for (int k=0;k<seg;k++){
        emitQuad(outerFront[k], outerFront[k+1], outerBack[k+1], outerBack[k]);
    }
    // Внутренняя поверхность (в обратном порядке)
    for (int k=0;k<seg;k++){
        emitQuad(innerBack[k], innerBack[k+1], innerFront[k+1], innerFront[k]);
    }
    // Передняя стенка (z=-depth)
    for (int k=0;k<seg;k++){
        emitQuad(outerFront[k], outerFront[k+1], innerFront[k+1], innerFront[k]);
    }
    // Задняя стенка (z=+depth)
    for (int k=0;k<seg;k++){
        emitQuad(innerBack[k], innerBack[k+1], outerBack[k+1], outerBack[k]);
    }
    // Закрытие торцов (на x=-r и x=+r)
    emitQuad(outerFront[0], outerBack[0], innerBack[0], innerFront[0]);
    emitQuad(outerBack[seg], outerFront[seg], innerFront[seg], innerBack[seg]);

    m_archUnit.upload(f, v, ind);

    // Базовое ребро, масштабируется для каждого экземпляра
    m_ribUnit = makeRibUnit(f);
}

void Bridge::update(Scene& scene, float dt)
{
    (void)dt;
    m_lift = scene.bridgeLift;
}

void Bridge::draw(QOpenGLFunctions_3_3_Core* f, const Shader& sh) const
{
    // Не используется - отрисовку делают вспомогательные функции,
    // которые корректно привязывают текстуры
    (void)f; (void)sh;
}

void Bridge::drawOpaque(QOpenGLFunctions_3_3_Core* f, const Shader& sh,
                        const Texture& roadTex,
                        const Texture& stoneTex,
                        const Texture& brickTex,
                        const Texture& steelTex,
                        const Texture& rockTex,
                        const Texture& bankTex) const
{
    if (!m_leaf.isValid()){
        const_cast<Bridge*>(this)->buildGeometry(f);
    }

    roadTex.bind(f, 0);

    auto setUV = [&](float mulX, float mulY){
        int loc = f->glGetUniformLocation(sh.id(), "uUVMul");
        if (loc >= 0) f->glUniform2f(loc, mulX, mulY);
    };
    setUV(1.0f, 1.0f);

    auto setAsphalt = [&](){
        sh.setFloat(f, "uSpecularStrength", 0.05f);
        sh.setFloat(f, "uSpecularPower",    12.0f);
    };
    auto setStone = [&](){
        sh.setFloat(f, "uSpecularStrength", 0.15f);
        sh.setFloat(f, "uSpecularPower",    28.0f);
    };
    auto setSteel = [&](){
        sh.setFloat(f, "uSpecularStrength", 0.75f);
        sh.setFloat(f, "uSpecularPower",    100.0f);
    };

    setAsphalt(); // Значения по умолчанию для полотна моста (асфальт)

    // Рисование твердых частей моста как двусторонних, чтобы не было прозрачности,
    // если направление обхода граней отличается
    f->glDisable(GL_CULL_FACE);

    // Макет моста:
    // левый берег -> неподвижный пролет -> левая опора -> разводной пролет ->
    // -> правая опора -> неподвижный пролет -> правый берег
    // Ось моста: X (слева->вправо), Z поперек, Y вверх

    const float deckY = 2.0f;

    // Positions of piers along X
    const float pierX_L = -6.0f;
    const float pierX_R = +6.0f;

    // Берега
    {
        setStone();
        bankTex.bind(f, 0);
        setUV(4.0f, 4.0f); // Умеренное повторение

        // Левый берег
        Mat4 M = Mat4::translate({-50.0f, 0.0f, 0.0f});
        sh.setMat4(f, "uModel", M.data());
        m_bank.draw(f);

        // Правый берег
        M = Mat4::translate({+50.0f, 0.0f, 0.0f});
        sh.setMat4(f, "uModel", M.data());
        m_bank.draw(f);
    }

    // Возвращение текстуры дороги и стандартного UV-повторение для полотна моста после отрисовки берегов
    // В противном случае последующая отрисовка полотна продолжит использовать текстуру берегов
    roadTex.bind(f, 0);
    setUV(1.0f, 1.0f);
    setAsphalt();

    // Неподвижные пролеты
    {
        setAsphalt();
        // Левый подъезд: от левого берега к левой опоре
        // Базовый m_leaf имеет половину длины 12, X масштабируется так, чтобы покрыть ~30 единиц
        float leftA = -43.0f;  // Край левого берега (совпадает с арками)
        float leftB = pierX_L; // Конец на левой опоре
        float leftCenter = 0.5f * (leftA + leftB);
        float leftHalf = 0.5f * (leftB - leftA);
        Mat4 M = Mat4::translate({leftCenter, deckY, 0.0f}) * Mat4::scale({leftHalf / 12.0f, 1.0f, 1.0f});
        // leftHalf уже вычислен: объекты симметричны, поэтому имя переменной можно переиспользовать
        setUV(1.0f, leftHalf / 12.0f);
        sh.setMat4(f, "uModel", M.data());
        m_leaf.draw(f);

        // Правый подъезд
        float rightA = pierX_R; // Начало на правой опоре
        float rightB = +43.0f;  // Край правого берега
        float rightCenter = 0.5f * (rightA + rightB);
        float rightHalf = 0.5f * (rightB - rightA);
        M = Mat4::translate({rightCenter, deckY, 0.0f}) * Mat4::scale({rightHalf / 12.0f, 1.0f, 1.0f});
        setUV(1.0f, rightHalf / 12.0f);
        sh.setMat4(f, "uModel", M.data());
        m_leaf.draw(f);
    }

    // Разводной пролет между опорами
    {
        // Жесткий элемент - длина не должна меняться при подъеме
        // Меш полотна центрирован в начале координат и имеет половину длины 12 (полная длина 24)
        // Один раз масштабируется по X, чтобы получить нужную длину пролета
        const float movableHalf = 6.0f; // Половина длины в мировых единицах (полная длина = 12)
        float scaleX = movableHalf / 12.0f;

        // Точка шарнира находится на левой опоре, на краю разводного пролета. В локальных координатах
        // край находится в x = -12, поэтому для переноса шарнира в (0,0,0) нужно сдвинуться на (+12, 0, 0)
        const Vec3 pivotLocal{-12.0f, 0.0f, 0.0f};
        const Vec3 worldPivot{pierX_L, deckY, 0.0f};

        const float liftAng = m_lift * (75.0f * 3.1415926f/180.0f); // Угол подъема

        // Масштабирование в локальном пространстве должно быть до поворота,
        // чтобы избежать растяжения объекта и текстуры при вращении
        // M = T(worldPivot) * R * S * T(-pivotLocal)
        Mat4 M = Mat4::translate(worldPivot)
               * Mat4::rotateZ(+liftAng)
               * Mat4::scale({scaleX, 1.0f, 1.0f})
               * Mat4::translate({-pivotLocal.x, -pivotLocal.y, -pivotLocal.z});

        setUV(1.0f, scaleX);
        sh.setMat4(f, "uModel", M.data());
        m_leaf.draw(f);
    }

    // Текстура скалы для опор
    rockTex.bind(f, 0);
    setUV(1.0f, 1.0f);

    // Опоры под мостом
    {
        setStone();
        // Опоры прямо под полотном моста слева и справа
        Mat4 M = Mat4::translate({pierX_L, 0.0f, 0.0f});
        sh.setMat4(f, "uModel", M.data());
        m_pier.draw(f);

        M = Mat4::translate({pierX_R, 0.0f, 0.0f});
        sh.setMat4(f, "uModel", M.data());
        m_pier.draw(f);
    }

    // Боковые арки (4 штуки: два пролета × две стороны)
    {
        const float roadHalfW = 4.5f;
        const float curbHalfW = 0.35f; // Должно совпадать с makeCurbs
        const float zSide = roadHalfW + curbHalfW; // Арки стоят по центру бордюра
        const float archHalfDepth = 0.40f; // Половина толщины по оси Z
        const float deckHalfH = 0.25f;
        const float archBaseY = deckY + deckHalfH; // Старт от поверхности дороги / верха бордюра

        auto drawArchSpan = [&](float x0, float x1, float z){
            const float cx = 0.5f * (x0 + x1);
            const float radius = 0.5f * (x1 - x0);

            // Сама арка (единичная арка, масштабируемая до нужного радиуса)
            Mat4 M = Mat4::translate({cx, archBaseY, z}) * Mat4::scale({radius, radius, archHalfDepth / 0.18f});
            sh.setMat4(f, "uModel", M.data());
            setSteel();
            steelTex.bind(f, 0);
            setUV(2.0f, 2.0f);
            m_archUnit.draw(f);

            // Вертикальные ребра внутри арки
            // Больше повторов UV, чтобы сталь не выглядела растянутой на высоких ребрах
            setUV(1.0f, 10.0f);
            const int ribs = 9;
            for (int r = 1; r <= ribs; ++r) {
                float t = float(r) / float(ribs + 1);
                float xLocal = -1.0f + 2.0f * t;
                float yLocal = std::sqrt(std::max(0.0f, 1.0f - xLocal * xLocal)) * 0.65f;
                float ribH = yLocal * radius;

                Mat4 Rm = Mat4::translate({cx + xLocal * radius, archBaseY + ribH * 0.5f, z})
                        * Mat4::scale({0.18f, ribH, 0.22f});
                sh.setMat4(f, "uModel", Rm.data());
                m_ribUnit.draw(f);
            }
        };

        // Левый пролет: от края подъезда левого берега до левой опоры
        drawArchSpan(-30.0f, pierX_L, +zSide);
        drawArchSpan(-30.0f, pierX_L, -zSide);

        // Правый пролет: от правой опоры до края подъезда правого берега
        drawArchSpan(pierX_R, +30.0f, +zSide);
        drawArchSpan(pierX_R, +30.0f, -zSide);
    }

    // Бордюры вдоль краев полотна (камень)
    setStone();
    stoneTex.bind(f, 0);
    // Для бордюров используется box-mapped UV, чтобы плотность текстуры была одинаковой на каждой грани
    sh.setInt(f, "uUseBoxMap", 1);
    // Базовая плотность для box mapping
    sh.setVec3(f, "uBoxScale", 0.5f, 0.5f, 0.5f);

    auto drawCurbsFor = [&](const Mat4& baseModel, float uvMulX){
        // Box-mapped UV вычисляются в пространстве объекта,
        // поэтому при масштабировании сегмента вдоль X нужно пропорционально увеличить
        // тайлинг по X, чтобы сохранить одинаковую плотность текстуры в мировом пространстве
        const float sx = (uvMulX <= 0.0001f) ? 1.0f : uvMulX;
        sh.setVec3(f, "uBoxScale", 0.5f * sx, 0.5f, 0.5f);

        const float roadHalfW = 4.5f;  // половина ширины m_leaf (sz)
        const float curbHalfW = 0.35f; // половина ширины m_curb (sz)
        const float deckHalfH = 0.25f; // половина высоты m_leaf (sy)
        const float curbHalfH = 0.36f; // половина высоты m_curb (sy)

        const float curbY = (curbHalfH - deckHalfH) - 0.02f; // sink a bit to cover full side thicknessn above road; overlaps side faces

        // Левый край
        Mat4 ML = baseModel * Mat4::translate({0.0f, curbY, -(roadHalfW + curbHalfW + 0.02f)});
        setUV(1.0f, 1.0f);
        sh.setMat4(f, "uModel", ML.data());
        m_curb.draw(f);

        // Правый край
        Mat4 MR = baseModel * Mat4::translate({0.0f, curbY, +(roadHalfW + curbHalfW + 0.02f)});
        setUV(1.0f, 1.0f);
        sh.setMat4(f, "uModel", MR.data());
        m_curb.draw(f);
    };

    // Левый подъезд
    {
        float leftA = -43.0f;
        float leftB = pierX_L;
        float leftCenter = 0.5f * (leftA + leftB);
        float leftHalf = 0.5f * (leftB - leftA);
        Mat4 base = Mat4::translate({leftCenter, deckY, 0.0f}) * Mat4::scale({leftHalf / 12.0f, 1.0f, 1.0f});
        drawCurbsFor(base, leftHalf / 12.0f);
    }

    // Разводной пролет
    {
        float movableA = pierX_L;
        float movableB = pierX_R;
        float movableHalf = 0.5f * (movableB - movableA);
        const float liftAng = m_lift * (75.0f * 3.1415926f/180.0f);

        Vec3 worldPivot{pierX_L, deckY, 0.0f};
        Vec3 pivotLocal{-12.0f, 0.0f, 0.0f};

        Mat4 M = Mat4::translate(worldPivot) * Mat4::rotateZ(+liftAng) * Mat4::scale({movableHalf/12.0f, 1.0f, 1.0f}) * Mat4::translate({+12.0f, 0.0f, 0.0f}); // T(pivot)*R*S*T(-pivotLocal)
        // Для бордюров переиспользуется матрица, как у разводного полотна
        drawCurbsFor(M, movableHalf/12.0f);
    }

    // Правый подъезд
    {
        float rightA = pierX_R;
        float rightB = +43.0f;
        float rightCenter = 0.5f * (rightA + rightB);
        float rightHalf = 0.5f * (rightB - rightA);
        Mat4 base = Mat4::translate({rightCenter, deckY, 0.0f}) * Mat4::scale({rightHalf / 12.0f, 1.0f, 1.0f});
        drawCurbsFor(base, rightHalf / 12.0f);
    }

    roadTex.bind(f, 0);
    sh.setInt(f, "uUseBoxMap", 0);

    // Опоры - кирпич
    brickTex.bind(f, 0);
    f->glEnable(GL_CULL_FACE);
}

void Bridge::drawWater(QOpenGLFunctions_3_3_Core* f, const Shader& sh, const Texture& waterTex, const Vec2& uvOffset) const
{
    if (!m_water.isValid()){
        const_cast<Bridge*>(this)->buildGeometry(f);
    }
    waterTex.bind(f, 0);

    Mat4 M = Mat4::translate({0.0f, 0.0f, 0.0f});
    sh.setMat4(f, "uModel", M.data());
    sh.setFloat(f, "uUVOffset.x", 0.0f); // Не используется
    int loc = f->glGetUniformLocation(sh.id(), "uUVOffset");
    if (loc >= 0) f->glUniform2f(loc, uvOffset.x, uvOffset.y);

    m_water.draw(f);

    // Сброс материала к значениям по умолчанию
    // (чтобы остальные объекты сохранили исходный вид)
    sh.setFloat(f, "uSpecularStrength", 0.25f);
    sh.setFloat(f, "uSpecularPower",    32.0f);
}

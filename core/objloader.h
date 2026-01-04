#ifndef OBJLOADER_H
#define OBJLOADER_H

#include <vector>
#include <QString>
#include "mesh.h"

// Загрузчик файлов OBJ с базовой поддержкой MTL (mtllib/usemtl)

class ObjLoader
{
public:
    struct Material
    {
        QString name;
        Vec3 ka{0.0f,0.0f,0.0f};
        Vec3 kd{1.0f,1.0f,1.0f};
        Vec3 ks{0.0f,0.0f,0.0f};
        float ns = 16.0f;
        float d = 1.0f;
        int illum = 2;

        QString mapKd;
        QString mapKs;
        QString mapKn;

        bool useTexture = false;
    };

    struct ObjPart
    {
        std::vector<Vertex> vertices;
        std::vector<unsigned> indices;
        Material material;
    };

    // Загрузка полной сетки
    static bool load(const QString& path, std::vector<Vertex>& outV, std::vector<unsigned>& outI, QString* err = nullptr);

    // Загрузка модели с разбиением по материалам
    static bool loadParts(const QString& path, std::vector<ObjPart>& parts, QString* err = nullptr);
};

#endif // OBJLOADER_H

#include "objloader.h"

#include <algorithm>
#include <unordered_map>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QRegularExpression>

struct Idx { int v=-1, t=-1, n=-1; };

static bool parseIdx(const QString& token, Idx& idx)
{
    // Форматы индексов: v, v/t, v//n, v/t/n (индексация в OBJ начинается с 1)
    QStringList parts = token.split('/');
    if (parts.size() < 1) return false;

    idx.v = parts[0].toInt() - 1;
    if (parts.size() >= 2 && !parts[1].isEmpty()) idx.t = parts[1].toInt() - 1;
    if (parts.size() >= 3 && !parts[2].isEmpty()) idx.n = parts[2].toInt() - 1;
    return true;
}

static QString resolveSiblingPath(const QString& baseObjPath, const QString& fileName)
{
    // Нормализация пути
    QFileInfo fi(fileName);
    QString baseName = fi.fileName();
    if (baseName.isEmpty()) baseName = fileName;

    if (baseName.startsWith(":/")) return baseName;

    if (baseObjPath.startsWith(":/")){
        int slash = baseObjPath.lastIndexOf('/');
        QString dir = (slash >= 0) ? baseObjPath.left(slash+1) : QString(":/");
        return dir + baseName;
    }

    QFileInfo info(baseObjPath);
    return info.dir().filePath(baseName);
}


static bool loadMtlFile(const QString& mtlPath, std::unordered_map<QString, ObjLoader::Material>& outMats)
{
    QFile f(mtlPath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return false;
    QTextStream in(&f);

    ObjLoader::Material cur;
    bool has = false;

    while (!in.atEnd()){
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith("#")) continue;

        auto parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (parts.isEmpty()) continue;

        const QString key = parts[0];

        if (key == "newmtl" && parts.size() >= 2){
            if (has) outMats[cur.name] = cur;
            cur = ObjLoader::Material{};
            cur.name = parts[1];
            cur.ka = {0,0,0};
            cur.kd = {1,1,1};
            cur.ks = {0,0,0};
            cur.ns = 16.0f;
            cur.d = 1.0f;
            cur.illum = 2;
            cur.mapKd.clear();
            cur.mapKs.clear();
            cur.mapKn.clear();
            cur.useTexture = false;
            has = true;
        } else if (key == "Kd" && parts.size() >= 4 && has){
            cur.kd = {parts[1].toFloat(), parts[2].toFloat(), parts[3].toFloat()};
        } else if (key == "Ka" && parts.size() >= 4 && has){
            cur.ka = {parts[1].toFloat(), parts[2].toFloat(), parts[3].toFloat()};
        } else if (key == "Ks" && parts.size() >= 4 && has){
            cur.ks = {parts[1].toFloat(), parts[2].toFloat(), parts[3].toFloat()};
        } else if (key == "Ns" && parts.size() >= 2 && has){
            cur.ns = parts[1].toFloat();
        } else if (key == "d" && parts.size() >= 2 && has){
            cur.d = parts[1].toFloat();
        } else if (key == "Tr" && parts.size() >= 2 && has){
            cur.d = 1.0f - parts[1].toFloat();
        } else if (key == "illum" && parts.size() >= 2 && has){
            cur.illum = parts[1].toInt();
        } else if (key == "map_Kd" && parts.size() >= 2 && has){
            cur.mapKd = parts[1];
            cur.useTexture = true;
        } else if (key == "map_Ks" && parts.size() >= 2 && has){
            cur.mapKs = parts[1];
        } else if ((key == "map_Kn" || key == "map_Bump" || key == "bump") && parts.size() >= 2 && has){
            cur.mapKn = parts[1];
        }
    }

    if (has) outMats[cur.name] = cur;
    return !outMats.empty();
}

bool ObjLoader::loadParts(const QString& path, std::vector<ObjPart>& partsOut, QString* err)
{
    partsOut.clear();

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)){
        if (err) *err = "Cannot open OBJ: " + path;
        return false;
    }
    QTextStream in(&file);

    std::vector<Vec3> pos;
    std::vector<Vec3> nrm;
    std::vector<Vec2> uv;

    std::unordered_map<QString, Material> mats;
    Material currentMat;
    currentMat.name = "default";
    currentMat.kd = {1,1,1};

    struct Tmp
    {
        std::vector<Vertex> v;
        std::vector<unsigned> ind;
        std::unordered_map<long long, unsigned> remap;
        Material mat;
    };
    std::unordered_map<QString, Tmp> buckets;

    auto getBucket = [&](const Material& m)->Tmp&{
        auto it = buckets.find(m.name);
        if (it == buckets.end()){
            Tmp t;
            t.mat = m;
            auto res = buckets.emplace(m.name, std::move(t));
            return res.first->second;
        }
        return it->second;
    };

    auto makeKey = [](int vi, int ti, int ni)->long long {
        long long a = (long long)(vi + 1);
        long long b = (long long)(ti + 1);
        long long c = (long long)(ni + 1);
        return (a << 42) ^ (b << 21) ^ c;
    };

    while (!in.atEnd()){
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith("#")) continue;

        auto p = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (p.isEmpty()) continue;

        if (p[0] == "mtllib" && p.size() >= 2){

    for (int mi = 1; mi < p.size(); ++mi){
        QString mtlPath = resolveSiblingPath(path, p[mi]);
        loadMtlFile(mtlPath, mats);
    }
} else if (p[0] == "usemtl" && p.size() >= 2){
            QString name = p[1];
            auto it = mats.find(name);
            if (it != mats.end()){
                currentMat = it->second;
            } else {
                currentMat.name = name;
                currentMat.kd = {1,1,1};
                currentMat.mapKd.clear();
            }
        } else if (p[0] == "v" && p.size() >= 4){
            pos.push_back({p[1].toFloat(), p[2].toFloat(), p[3].toFloat()});
        } else if (p[0] == "vn" && p.size() >= 4){
            nrm.push_back({p[1].toFloat(), p[2].toFloat(), p[3].toFloat()});
        } else if (p[0] == "vt" && p.size() >= 3){
            uv.push_back({p[1].toFloat(), p[2].toFloat()});
        } else if (p[0] == "f" && p.size() >= 4){
            Tmp& bucket = getBucket(currentMat);

            auto addVertex = [&](const QString& tok)->unsigned{
                Idx idx;
                parseIdx(tok, idx);

                auto fix = [](int i, int count)->int { return (i < 0) ? (count + i) : i; };
                int vi = fix(idx.v, (int)pos.size());
                int ti = (idx.t == -1) ? -1 : fix(idx.t, (int)uv.size());
                int ni = (idx.n == -1) ? -1 : fix(idx.n, (int)nrm.size());

                long long key = makeKey(vi, ti, ni);
                auto it = bucket.remap.find(key);
                if (it != bucket.remap.end()) return it->second;

                Vertex vx{};
                vx.pos = (vi >= 0 && vi < (int)pos.size()) ? pos[vi] : Vec3{0,0,0};
                vx.uv  = (ti >= 0 && ti < (int)uv.size())  ? uv[ti]  : Vec2{0,0};
                vx.nrm = (ni >= 0 && ni < (int)nrm.size()) ? nrm[ni] : Vec3{0,1,0};

                unsigned outIdx = (unsigned)bucket.v.size();
                bucket.v.push_back(vx);
                bucket.remap[key] = outIdx;
                return outIdx;
            };

            // Веерная триангуляция полигона
            unsigned v0 = addVertex(p[1]);
            for (int k=2; k < p.size()-1; ++k){
                unsigned v1 = addVertex(p[k]);
                unsigned v2 = addVertex(p[k+1]);
                bucket.ind.push_back(v0);
                bucket.ind.push_back(v1);
                bucket.ind.push_back(v2);
            }
        }
    }

    for (auto& kv : buckets){
        if (kv.second.v.empty() || kv.second.ind.empty()) continue;
        ObjPart part;
        part.vertices = std::move(kv.second.v);
        part.indices  = std::move(kv.second.ind);
        part.material = kv.second.mat;
        partsOut.push_back(std::move(part));
    }

    std::sort(partsOut.begin(), partsOut.end(), [](const ObjPart& a, const ObjPart& b){
        if (a.material.name == "default") return true;
        if (b.material.name == "default") return false;
        return a.material.name < b.material.name;
    });

    if (partsOut.empty()){
        if (err) *err = "OBJ has no faces: " + path;
        return false;
    }
    return true;
}

bool ObjLoader::load(const QString& path, std::vector<Vertex>& outV, std::vector<unsigned>& outI, QString* err)
{
    outV.clear();
    outI.clear();

    std::vector<ObjPart> parts;
    if (!loadParts(path, parts, err)) return false;

    unsigned base = 0;
    for (const auto& p : parts){
        outV.insert(outV.end(), p.vertices.begin(), p.vertices.end());
        for (unsigned idx : p.indices) outI.push_back(idx + base);
        base += (unsigned)p.vertices.size();
    }
    return true;
}

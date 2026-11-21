/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#ifndef SFILE_H
#define	SFILE_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QMatrix4x4>
#include <QString>
#include <QHash>
#include <QVector>
#include <QVarLengthArray>
#include <QSet>
#include <QMap>
#include <QVector3D>
#include <array>
#include <memory>
#include <limits>
#include <vector>

class FileBuffer;
class ShapeTextureInfo;
class ShapeHierarchyInfo;
class ContentHierarchyInfo;
class RenderItem;
class QOpenGLExtraFunctions;
class StaticShapeRenderBatch;
struct SFileReadResult;

struct OpeningViewLoad;

struct SFileGeometry {

    struct SObjHeader {
        QVector<int> geometryNodeMap;
    };
    
    struct fshader {
        QString name;
        float alpha;
    };
    
    struct czes {
        int iloscv;
        int prim_state_idx;
        int offset;
        std::unique_ptr<int[]> idx;
        bool enabled = true;
    };

    struct SubGraphics {
        QOpenGLBuffer VBO;
        QOpenGLVertexArrayObject VAO;
    };

    struct sub {
        int iloscc = 0;
        SObjHeader header;
        std::unique_ptr<czes[]> czesci;
        std::vector<float> vertices;
        std::unique_ptr<SubGraphics> graphics;
    };

    struct dist {
        int levelSelection;
        int ilosch;
        std::unique_ptr<int[]> hierarchia;
        int iloscs;
        std::unique_ptr<sub[]> subobiekty;
    };

    struct matrt {
        QString name;
        float param[16];
        float fixed[16];
        bool isFixed = false;
        long long int hash = 0;
    };

    struct primst {
        int vtx_state, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8;
    };

    struct text {
        int image, arg1, arg2, arg3;
    };

    struct vtxs {
        int arg1, matrix, arg2, arg3, arg4;
    };

    struct imgs {
        QString name;
        int tex;
        int texAddr = -1;
    };

    struct fpoint {
        float x, y, z;
    };

    struct punlist {
        std::unique_ptr<fpoint[]> points;
        std::unique_ptr<fpoint[]> uv_points;
        std::unique_ptr<fpoint[]> normals;
        int ipoints;
        int iuv_points;
        int inormals;
    };
    
    // zmienne
    //struct VBO {
    //    int w[];
    //};
    
    struct EsdBoundingBox {
        float shape[6];
        float rotation[3];
        float translation[3];
        
        EsdBoundingBox(){
            rotation[0] = 0;
            rotation[1] = 0;
            rotation[2] = 0;
            translation[0] = 0;
            translation[1] = 0;
            translation[2] = 0;
        }
    };
    
    struct AnimFrameId {
        unsigned int id1 = 0;
        unsigned int id2 = 0;
        float offset = 0;
    };
    
    struct AnimNode {
        struct TcbKey {
            int frame;
            float quat[4];
            float param[5];
        };
        struct SlerpRot {
            int frame;
            float quat[4];
        };
        struct LinearKey {
            int frame;
            float pos[3];
        };
        QVector<TcbKey> tcbKey;
        QVector<AnimFrameId> tcbId;
        QVector<SlerpRot> slerpRot;
        QVector<AnimFrameId> slerpbId;
        QVector<LinearKey> linearKey;
        QVector<AnimFrameId> linearId;
    };
    
    struct Animation {
        int frames;
        float fps;
        QVector<AnimNode> node;
        void loadC(FileBuffer* data, int length, bool debugOutput);
        void loadX(FileBuffer* data);
    };
    
    QVector<Animation> animations;
    punlist tpoints{};
    int iloscd = 0;
    int iloscm = 0;
    std::unique_ptr<matrt[]> macierz;
    int ilosci = 0;
    std::unique_ptr<imgs[]> image;
    int ilosct = 0;
    std::unique_ptr<text[]> texture;
    int iloscv = 0;
    std::unique_ptr<vtxs[]> vtxstate;
    int iloscps = 0;
    std::unique_ptr<primst[]> primstate;
    std::unique_ptr<dist[]> distancelevel;
    int ishaders = 0;
    std::unique_ptr<fshader[]> shader;
    float size = 0;
    float bound[6]{};

    bool parse(FileBuffer &source, float alphaTest, bool debugOutput);
    void getSize();
    void buildFrameIds();

    template <typename Vertex>
    bool packVertices(sub &object, const Vertex *source, int sourceCount, float defaultAlphaTest) {
        std::size_t vertexCount = 0;
        for (int i = 0; i < object.iloscc; ++i) {
            if (object.czesci[i].iloscv < 0)
                return false;
            vertexCount += object.czesci[i].iloscv;
        }
        if (vertexCount > std::numeric_limits<int>::max() / (9 * sizeof(float)))
            return false;
        object.vertices.resize(vertexCount * 9);
        int offset = 0;
        for (int i = 0; i < object.iloscc; ++i) {
            auto &part = object.czesci[i];
            part.offset = offset;
            if (part.iloscv == 0)
                continue;
            if (part.prim_state_idx < 0 || part.prim_state_idx >= iloscps)
                return false;
            const auto &material = primstate[part.prim_state_idx];
            const bool fullyOpaque = material.arg2 >= 0 && material.arg2 < ishaders
                && shader[material.arg2].alpha == 1;
            const float alpha = fullyOpaque ? 1.0f
                : (material.arg6 == 1 ? -0.51f : -defaultAlphaTest);
            for (int j = 0; j < part.iloscv; ++j) {
                const int index = part.idx[j];
                if (index < 0 || index >= sourceCount)
                    return false;
                const auto &vertex = source[index];
                if (vertex.point >= unsigned(tpoints.ipoints))
                    return false;
                const auto &point = tpoints.points[vertex.point];
                const auto normal = vertex.normal < unsigned(tpoints.inormals)
                    ? tpoints.normals[vertex.normal] : fpoint{0, 1, 0};
                const auto uv = vertex.uvpoint < unsigned(tpoints.iuv_points)
                    ? tpoints.uv_points[vertex.uvpoint] : fpoint{};
                float *packed = object.vertices.data() + std::size_t(offset + j) * 9;
                packed[0] = point.x;
                packed[1] = point.y;
                packed[2] = point.z;
                packed[3] = normal.x;
                packed[4] = normal.y;
                packed[5] = normal.z;
                packed[6] = uv.x;
                packed[7] = uv.y;
                packed[8] = alpha;
            }
            offset += part.iloscv;
            part.idx.reset();
        }
        return true;
    }
};

class SFile : public SFileGeometry {
public:
    bool animated = false;
    
    QString pathid;
    QString sciezka;
    QString nazwa;
    QString baseTexPath;
    bool rollingStock = false;
    QString texPath;
    QString sdName;
    int isinit = 0;
    int loaded = 0;
    bool loadedSd = false;
    int texloaded = 0;
    int ref = 0;
    int esdDetailLevel = -1;
    int esdAlternativeTexture = -1;
    QVector<EsdBoundingBox> esdBoundingBox;
    
    int currentDistanceLevel = 0;
    
    QString evaluatePathId();   /// EFO to fix relative paths
    
    SFile();
    SFile(QString pathid, QString name, QString texp );
    SFile(const SFile& orig);
    virtual ~SFile();
    void load();
    void enablePart(unsigned int uid, unsigned int stateId = 0);
    void disablePart(unsigned int uid, unsigned int stateId = 0);
    void updateSim(float deltaTime, unsigned int stateId = 0);
    void render();
    void render(int selectionColor, unsigned int stateId);
    void prepareForOpeningView(unsigned int stateId, OpeningViewLoad &opening);
    void pushRenderItem();
    void pushRenderItem(int selectionColor, unsigned int stateId);
    bool getBoxPoints(QVector<float> &points);
    void getFloorBorderLinePoints(float *&punkty);
    bool isSnapable();
    bool attachmentOrigin(const QString &name, float *origin);
    QMap<int, QVector3D> wireAttachmentOrigins();
    void addSnapablePoints(QVector<float> &out);
    void reload();
    void invalidatePendingRead();
    void refreshSeasonTextures();
    unsigned int newState();
    void releaseState(unsigned int stateId);
    void compactReleasedStates();
    void setAnimated(unsigned int stateId, bool animated);
    void setEnabledSubObjs(unsigned int stateId, unsigned int enabledSubObjs);
    void setCurrentDistanceLevel(unsigned int stateId, int level);
    void enableSubObjByName(unsigned int stateId, QString name, bool val);
    void enableSubObjByNameQueue(unsigned int stateId, QString name, bool val);
    void fillShapeTextureInfo(QHash<int, ShapeTextureInfo*> &list, unsigned int stateId = 0);
    void fillShapeHierarchyInfo(ShapeHierarchyInfo* info, unsigned int stateId = 0);
    void fillContentHierarchyInfo(QVector<ContentHierarchyInfo*> &list, int parent);
    void altPathFix(QString pathid);
    
private:
    friend class StaticShapeRenderBatch;
    bool staticBatchState(unsigned int stateId, int &lod, int &subObjects) const;
    bool polyVegBatchState(int lod, int subObjects) const;
    bool opaqueBatchState(int lod, int subObjects) const;
    void render(int selectionColor, unsigned int stateId,
                QOpenGLExtraFunctions *instancing, int instanceCount,
                bool staticScenery = false);

    struct State {
        bool animated = false; 
        int enabledSubObjs = 0xFFFFFFFF;
        float frameCount = 0;
        unsigned long long int lastTime = 0;
        QMap<QString, bool> enableSubObjQueue;
        int distanceLevel = 0;
    };
    QVector<State> state;
    QSet<unsigned int> releasedStateIds;
    
    void applyPendingSubObjects(unsigned int stateId);
    bool loadForRender();
    void loadDecoded(SFileReadResult result);
    bool uploadGeometry();
    void loadSd(FileBuffer *data);
    QString resolveTexturePath(QString textureName) const;
    int loadTexture(const QString &textureName) const;
    float* getPmatrix(int currentDlevel, float* pmatrix, int matrix);
    float* getPmatrixAnimated(int currentDlevel, float* pmatrix, int matrix, float frame);
    bool snapable = false;
    //float *mvMatrix = NULL;
    bool requiresUpdate = false;
    QHash<unsigned int, QVector<RenderItem *>> renderItems;
};

class StaticShapeRenderBatch {
public:
    void begin(bool vegetation, bool scenery);
    bool enqueue(SFile &shape, unsigned int stateId, float *tileMatrix,
                 float *objectMatrix, const QString &fileName, bool rawPreview);
    void flush();

private:
    struct Key {
        SFile *shape = nullptr;
        int lod = 0;
        int subObjects = 0;
        bool operator==(const Key &other) const {
            return shape == other.shape && lod == other.lod && subObjects == other.subObjects;
        }
        friend size_t qHash(const Key &key, size_t seed) noexcept {
            return qHashMulti(seed, key.shape, key.lod, key.subObjects);
        }
    };
    struct Group {
        Key key;
        unsigned int state = 0;
        QVarLengthArray<std::array<float, 16>, 1> matrices;
    };
    void draw(const Group &group, QOpenGLExtraFunctions *functions,
              bool rawPreview, bool staticScenery);
    void flushVegetation();
    void flushScenery();

    bool vegetationEnabled = false;
    bool sceneryEnabled = false;
    bool rawPreview = false;
    Group plants;
    QSet<Key> nonOpaque;
    QHash<Key, qsizetype> sceneryIndices;
    QVector<Group> sceneryGroups;
    QOpenGLBuffer buffer;
};

#endif	/* SFILE_H */


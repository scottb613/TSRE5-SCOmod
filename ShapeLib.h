#ifndef SHAPELIB_H
#define	SHAPELIB_H

#include <unordered_map>
#include <QString>
#include <QHash>
#include <memory>

class SFile;
class GltfPreview;

class ShapeLib {
public:
    int jestshape = 0;
    std::unordered_map<int, SFile*> shape;
    ShapeLib();
    ShapeLib(const ShapeLib& orig);
    virtual ~ShapeLib();
    void reset();
    void delRef(int texx);
    void addRef(int texx);
    int addShape(QString path);
    int addShape(QString path, QString texPath);
    bool reloadShapeIfCached(QString path);
    void refreshSeasonTextures();
    std::shared_ptr<GltfPreview> getGltfShape(const QString &path, QString &error);
    void releaseGltfGraphics();
private:
    QHash<QString, std::shared_ptr<GltfPreview>> gltfShapes;
};

#endif	/* SHAPELIB_H */


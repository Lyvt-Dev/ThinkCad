#pragma once
#include "thinkercad/circuit.hpp"
#include <QMap>
#include <QString>
struct SketchProject {
    QString name="sketch",sourceFolder;
    QStringList order;
    QMap<QString,QString> files;
    static SketchProject openIno(const QString& path);
    QString combinedSource() const;
    QMap<QString,QString> auxiliaryFiles() const;
};
struct SavedProject {thinkercad::CircuitDocument circuit;SketchProject sketch;QMap<QString,SketchProject> boardSketches;};
class ProjectIO {
public:
    static void save(const QString& path,const SavedProject& project);
    static SavedProject load(const QString& path);
    static bool validFileName(const QString& name);
};

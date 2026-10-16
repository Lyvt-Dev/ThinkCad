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
struct SavedProject {thinkercad::CircuitDocument circuit;SketchProject sketch;QMap<QString,SketchProject> boardSketches;QMap<QString,QByteArray> bundledLibraries;thinkercad::ExecutionMode mode=thinkercad::ExecutionMode::Simulation;};
class ProjectIO {
public:
    static QMap<QString,QByteArray> bundleLibraries(const QStringList& directories);
    static QStringList extractLibraries(const QMap<QString,QByteArray>& files,const QString& destination);
    static QByteArray serialize(const SavedProject& project);
    static SavedProject deserialize(const QByteArray& data);
    static void save(const QString& path,const SavedProject& project);
    static SavedProject load(const QString& path);
    static bool validFileName(const QString& name);
};

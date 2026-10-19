#pragma once
#include "thinkercad/circuit.hpp"
#include <QMap>
#include <QJsonObject>
#include <QJsonArray>
#include <QString>
#include <QSet>
struct SketchProject {
    QString name="sketch",sourceFolder;
    QString language="arduino";
    QJsonObject visual; // Opaque preserved metadata; BlockPersistence validates before editing.
    QStringList order;
    QMap<QString,QString> files;
    QMap<QString,QSet<int>> breakpoints;
    static SketchProject openIno(const QString& path);
    static SketchProject openPython(const QString& path);
    QString entryPoint() const;
    QString combinedSource() const;
    QMap<QString,QString> auxiliaryFiles() const;
};
struct SavedProject {thinkercad::CircuitDocument circuit;SketchProject sketch;QMap<QString,SketchProject> boardSketches;QMap<QString,QByteArray> bundledLibraries;thinkercad::ExecutionMode mode=thinkercad::ExecutionMode::Simulation;QJsonArray libraryReferences;};
class ProjectIO {
public:
    static QMap<QString,QByteArray> bundleLibraries(const QStringList& directories);
    static QStringList extractLibraries(const QMap<QString,QByteArray>& files,const QString& destination);
    static QByteArray serialize(const SavedProject& project);
    // Preview may validate embedded descriptor models without retaining registry changes.
    static SavedProject deserialize(const QByteArray& data,bool retainEmbeddedModels=true);
    static void save(const QString& path,const SavedProject& project);
    static SavedProject load(const QString& path);
    static bool validFileName(const QString& name);
};

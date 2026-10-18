#pragma once
#include "projectio.hpp"
class CircuitCanvas;
class ZipWriter {
  public:
    static QByteArray archive(const QMap<QString, QByteArray> &files);
};
class BomBuilder {
  public:
    static QByteArray csv(const thinkercad::CircuitDocument &);
};
class ProjectExporter {
  public:
    static QMap<QString, QByteArray> files(SavedProject, const QStringList &libraries,
                                           bool includeLocalLibraries = false);
    static void saveZip(const QString &, SavedProject, const QStringList &,
                        bool includeLocalLibraries = false);
};
class CircuitExporter {
  public:
    static void save(CircuitCanvas &, const QString &path, bool wholeCircuit = false);
};

#pragma once
#include "thinkercad/modules.hpp"
#include <QJsonArray>
#include <QJsonObject>
#include <QStringList>
namespace thinkercad {class CircuitDocument;}
namespace ModulePackages {
QString directory();
QJsonObject encode(const thinkercad::ModuleDefinition&);
thinkercad::ModuleDefinition decode(const QJsonObject&);
QJsonArray usedDefinitions(const thinkercad::CircuitDocument&);
QStringList loadInstalled();
QStringList importFile(const QString& path);
QStringList importRepository(const QString& directory);
// Embedded project descriptors use the same data-only validation as local packages.
QStringList installEmbedded(const QJsonArray& definitions);
}
class QWidget;
namespace ModulePackages {bool manage(QWidget* parent);}

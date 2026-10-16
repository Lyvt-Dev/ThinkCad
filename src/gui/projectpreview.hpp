#pragma once
#include <QImage>
#include <QString>
class CircuitCanvas;
namespace ProjectPreview {
// Circuit-only snapshots; generating a preview never runs the project's code.
QImage capture(CircuitCanvas& canvas);
void update(const QString& path, CircuitCanvas& canvas);
QImage load(const QString& path);
}

#pragma once
#include "thinkercad/circuit.hpp"
#include <QPointF>
#include <QRectF>
#include <QString>
class QPainter;
namespace ComponentDesign {
QString current();
void set(const QString& design);
bool realistic();
QRectF bounds(const thinkercad::CircuitPart& part);
QPointF pinPosition(const thinkercad::CircuitPart& part,const std::string& pin);
void paint(QPainter& painter,const thinkercad::CircuitPart& part,
           const thinkercad::CircuitState& state,const thinkercad::Frame& frame,
           double brightness,bool selected);
}

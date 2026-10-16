#pragma once
#include "thinkercad/circuit.hpp"
#include "language.hpp"
#include <QFont>
#include <QFontMetricsF>
#include <QRectF>
#include <algorithm>
#include <cmath>
inline QFont noteFont(){return QFont("Sans Serif",10);}
inline QString noteText(const thinkercad::CircuitPart& part){return part.model.empty()?Language::text("Notiz doppelklicken, um Text zu bearbeiten."):QString::fromStdString(part.model);}
inline QRectF noteBounds(const thinkercad::CircuitPart& part){
    const auto text=noteText(part);QFontMetricsF metrics(noteFont());double width=0;
    for(const auto& line:text.split('\n'))width=std::max(width,metrics.horizontalAdvance(line));
    width=std::clamp(std::ceil(width),16.,600.);
    const auto measured=metrics.boundingRect(QRectF(0,0,width,100000),Qt::TextWordWrap|Qt::TextWrapAnywhere,text);
    return {0,0,thinkercad::parameter(part,"noteWidth",std::clamp(width+16,32.,4096.)),thinkercad::parameter(part,"noteHeight",std::clamp(std::ceil(measured.height())+16,32.,4096.))};
}

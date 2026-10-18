#pragma once
#include "thinkercad/visual/blockprogram.hpp"
#include <QJsonObject>
namespace BlockPersistence {
QJsonObject encode(const thinkercad::visual::BlockProgram &);
thinkercad::visual::BlockProgram decode(const QJsonObject &);
QString fingerprint(const QString &source);
} // namespace BlockPersistence

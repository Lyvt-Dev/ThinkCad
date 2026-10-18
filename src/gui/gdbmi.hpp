#pragma once
#include <QByteArray>
#include <QString>
#include <QVariantMap>
#include <optional>
namespace GdbMi {
struct Record {int token=-1;char type=0;QString kind,text;QVariantMap values;};
std::optional<Record> parse(const QByteArray& line,QString* error=nullptr);
QByteArray quote(const QString& value);
}

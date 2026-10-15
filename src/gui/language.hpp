#pragma once
#include <QString>
class QObject;
namespace Language {
QString current();
void set(const QString& language);
QString text(const QString& source);
void translateTree(QObject* root);
}

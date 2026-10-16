#pragma once
#include <QString>
#include <QStringList>
class QObject;
namespace Language {
QStringList codes();
QStringList sourceTexts();
QString nativeName(const QString& code);
QString current();
void set(const QString& language);
QString text(const QString& source);
QString tutorialHtml();
void translateTree(QObject* root);
}

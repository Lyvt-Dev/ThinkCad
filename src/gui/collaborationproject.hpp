#pragma once
#include "projectio.hpp"
#include <QJsonObject>
namespace CollaborationProject {
QJsonObject encode(const SavedProject& project);
SavedProject decode(const QJsonObject& document);
QString validate(const QJsonObject& document);
}

#pragma once
#include <QJsonObject>
#include <QStringList>
namespace ProjectMerge {
struct Result {QJsonObject document;QStringList conflicts;};
// Merges only data. Conflicting edits are reported and the local proposal stays
// available to the caller; resolution is never an implicit last-writer choice.
Result merge(const QJsonObject& base,const QJsonObject& current,const QJsonObject& proposed,bool preferProposedConflicts=false);
}

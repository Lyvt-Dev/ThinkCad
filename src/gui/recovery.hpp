#pragma once
#include "projectio.hpp"
#include <QLockFile>
#include <QDateTime>
#include <memory>
struct RecoveryEntry {QString path,name,source;QDateTime modified;};
class ProjectRecovery {
public:
    ProjectRecovery();
    void save(const SavedProject& project,const QString& source);
    void clear();
    QString path()const{return path_;}
    static QString directory();
    static QList<RecoveryEntry> available();
    static void discard(const QString& path);
private:QString path_;std::unique_ptr<QLockFile> lock_;
};

#include "recovery.hpp"
#include <QDir>
#include <QDateTime>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>
#include <QCryptographicHash>
#include <QUuid>
#include <stdexcept>
QString ProjectRecovery::directory(){auto root=qEnvironmentVariable("THINKCAD_RECOVERY_DIR");if(root.isEmpty())root=QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)+"/recovery";if(!QDir().mkpath(root)||!QFileInfo(root).isWritable()){auto key=QCryptographicHash::hash(root.toUtf8(),QCryptographicHash::Sha256).toHex().left(16);root=QDir::tempPath()+"/thinkcad-recovery-"+QString::fromLatin1(key);QDir().mkpath(root);}return root;}
ProjectRecovery::ProjectRecovery(){QDir().mkpath(directory());path_=QDir(directory()).filePath(QUuid::createUuid().toString(QUuid::WithoutBraces)+".thinkcad");lock_=std::make_unique<QLockFile>(path_+".lock");if(!lock_->tryLock(0)){lock_.reset();path_.clear();}}
void ProjectRecovery::save(const SavedProject& project,const QString& source){if(path_.isEmpty())throw std::runtime_error("Recovery directory is not writable");ProjectIO::save(path_,project);QSaveFile meta(path_+".json");auto data=QJsonDocument(QJsonObject{{"name",project.sketch.name},{"source",source}}).toJson();if(!meta.open(QIODevice::WriteOnly)||meta.write(data)!=data.size()||!meta.commit())throw std::runtime_error("Cannot save recovery metadata");}
void ProjectRecovery::clear(){if(!path_.isEmpty())discard(path_);}
void ProjectRecovery::discard(const QString& path){QFile::remove(path);QFile::remove(path+".json");}
QList<RecoveryEntry> ProjectRecovery::available(){QList<RecoveryEntry> entries;QDir dir(directory());for(const auto& file:dir.entryInfoList({"*.thinkcad"},QDir::Files,QDir::Time)){QLockFile lock(file.absoluteFilePath()+".lock");if(!lock.tryLock(0))continue;QFile meta(file.absoluteFilePath()+".json");QJsonObject info;if(meta.open(QIODevice::ReadOnly)&&meta.size()<16384)info=QJsonDocument::fromJson(meta.readAll()).object();entries.append({file.absoluteFilePath(),info["name"].toString(file.baseName()),info["source"].toString(),file.lastModified()});}return entries;}

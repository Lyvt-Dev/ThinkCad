#include "appimageupdate.hpp"
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QCryptographicHash>
#include <QLockFile>
bool replaceAppImage(const QString& original,const QString& downloaded,const QByteArray& expected,QString* error,const std::function<void(qint64,qint64)>& progress){
    auto fail=[&](const QString& message){if(error)*error=message;return false;};
    QFileInfo current(original);if(!current.isFile()||current.isSymLink()||!current.isWritable()||expected.size()!=32)return fail("AppImage or update directory is not writable, or the verified checksum is missing.");
    QLockFile lock(original+".update-lock");if(!lock.tryLock(0))return fail("Another AppImage update is running.");
    QFile source(downloaded);if(!source.open(QIODevice::ReadOnly))return fail(source.errorString());auto header=source.peek(20);if(header.size()<20||header.left(4)!=QByteArray("\x7f" "ELF",4)||header.mid(8,3)!=QByteArray("AI\x02",3)||static_cast<unsigned char>(header[18])!=62||header[19]!=0)return fail("Update is not an x86_64 type-2 AppImage.");
    QFile old(original);QSaveFile backup(original+".previous");backup.setDirectWriteFallback(false);
    if(QFileInfo(original+".previous").isSymLink()||!old.open(QIODevice::ReadOnly)||!backup.open(QIODevice::WriteOnly))return fail("Cannot create AppImage rollback backup.");
    while(!old.atEnd()){const auto chunk=old.read(65536);if((chunk.isEmpty()&&old.error()!=QFileDevice::NoError)||backup.write(chunk)!=chunk.size())return fail("Cannot write AppImage rollback backup.");}
    if(!backup.setPermissions(current.permissions())||!backup.commit())return fail("Cannot commit AppImage rollback backup.");
    QSaveFile target(original);target.setDirectWriteFallback(false);if(!target.open(QIODevice::WriteOnly))return fail(target.errorString());QCryptographicHash hash(QCryptographicHash::Sha256);qint64 total=0;while(!source.atEnd()){auto chunk=source.read(65536);if(chunk.isEmpty()&&source.error()!=QFileDevice::NoError){target.cancelWriting();return fail(source.errorString());}total+=chunk.size();if(total>512ll*1024*1024||target.write(chunk)!=chunk.size()){target.cancelWriting();return fail(target.errorString());}hash.addData(chunk);if(progress)progress(total,source.size());}if(hash.result()!=expected){target.cancelWriting();return fail("AppImage changed after verification; original was kept.");}if(!target.setPermissions(current.permissions()|QFileDevice::ExeOwner)){target.cancelWriting();return fail(target.errorString());}if(!target.commit())return fail(target.errorString());return true;
}

bool restoreAppImage(const QString& original,QString* error){
    QFile backup(original+".previous");QSaveFile target(original);target.setDirectWriteFallback(false);
    if(!backup.open(QIODevice::ReadOnly)||!target.open(QIODevice::WriteOnly)){if(error)*error="Cannot open AppImage rollback files.";return false;}
    while(!backup.atEnd()){const auto chunk=backup.read(65536);if((chunk.isEmpty()&&backup.error()!=QFileDevice::NoError)||target.write(chunk)!=chunk.size()){if(error)*error="Cannot restore AppImage backup.";return false;}}
    if(!target.setPermissions(QFileInfo(backup).permissions())||!target.commit()){if(error)*error=target.errorString();return false;}return true;
}

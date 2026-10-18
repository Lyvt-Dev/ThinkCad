#pragma once
#include <QString>
#include <QByteArray>
#include <functional>
// Atomically replace a user-owned AppImage after verifying the exact downloaded bytes.
bool replaceAppImage(const QString& original,const QString& downloaded,const QByteArray& expectedSha256,QString* error, const std::function<void(qint64,qint64)>& progress = {});
bool restoreAppImage(const QString& original,QString* error);

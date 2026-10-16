#pragma once
#include <QString>
#include <QByteArray>
// Atomically replace a user-owned AppImage after verifying the exact downloaded bytes.
bool replaceAppImage(const QString& original,const QString& downloaded,const QByteArray& expectedSha256,QString* error);

#pragma once
#include "projectio.hpp"
#include <QImage>
#include <QJsonArray>
struct DecodedShare {
    QByteArray project;
    QJsonArray libraries;
    bool encrypted = false;
};
class ProjectShare {
  public:
    static constexpr int MaximumProjectBytes = 32 * 1024 * 1024;
    static constexpr int MaximumCodeBytes = 12 * 1024 * 1024;
    static constexpr int MaximumQrBytes = 1200;
    static QString encode(SavedProject, const QJsonArray &libraries = {}, QByteArray password = {});
    static DecodedShare decode(const QString &, QByteArray password = {});
    static bool encrypted(const QString &);
    static QImage qrImage(const QString &, int scale = 6);
    static QString readQr(const QImage &);
};

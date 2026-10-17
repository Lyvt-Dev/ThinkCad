#pragma once
#include <QNetworkRequest>
#include <QRegularExpression>

namespace thinkcad_update {
// GitHub's default commit response includes patches and can exceed 1 MiB.
// This representation resolves annotated tags directly to the commit SHA.
inline QNetworkRequest commitRequest(QNetworkRequest request) {
    request.setRawHeader("Accept", "application/vnd.github.sha");
    return request;
}
inline QString commitSha(const QByteArray& response) {
    if (response.size() > 128) return {};
    const auto sha = QString::fromLatin1(response).trimmed();
    return QRegularExpression("^[a-f0-9]{40}$").match(sha).hasMatch() ? sha : QString{};
}
}

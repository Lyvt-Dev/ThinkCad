#pragma once

#include <QPixmap>
#include <QSettings>
#include <QString>
#include <array>

inline void initializeAssistantSkinResources() {
    Q_INIT_RESOURCE(thinkcad_assistant);
}

namespace AssistantSkins {
struct Skin {
    const char *id;
    const char *name;
    const char *resource;
};
inline constexpr std::array<Skin, 3> skins{{
    {"quack", "Quack", ":/assistant/duck.png"},
    {"nyra-quack", "Nyra Quack", ":/assistant/nyra-quack.png"},
    {"nyra", "Nyra", ":/assistant/nyra.png"},
}};
inline const Skin &find(const QString &id) {
    for (const auto &skin : skins)
        if (id == QLatin1String(skin.id))
            return skin;
    return skins.front();
}
inline QString savedId() {
    return QString::fromLatin1(find(QSettings().value("duckAssistantSkin", "quack").toString()).id);
}
inline QPixmap pixmap(const QString &id) {
    static const bool initialized = [] {
        initializeAssistantSkinResources();
        return true;
    }();
    Q_UNUSED(initialized);
    QPixmap result(QString::fromLatin1(find(id).resource));
    if (result.isNull())
        result.load(QString::fromLatin1(skins.front().resource));
    return result;
}
} // namespace AssistantSkins

#pragma once
#include <QWidget>
class QVBoxLayout;
class QStackedWidget;
class StartPage final:public QWidget {
    Q_OBJECT
public:
    explicit StartPage(QWidget* parent=nullptr);
    void refresh();
    static void remember(const QString& path);
signals:
    void newRequested();
    void openRequested();
    void recentRequested(const QString& path);
    void resumeRequested();
    void examplesRequested();
    void tutorialRequested();
    void darkModeChanged(bool enabled);
    void languageChanged(const QString& language);
private:
    QVBoxLayout* recentLayout_;
    QStackedWidget* pages_;
};

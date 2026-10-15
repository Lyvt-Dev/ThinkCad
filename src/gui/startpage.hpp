#pragma once
#include <QWidget>
class QVBoxLayout;
class QStackedWidget;
class WelcomePage final:public QWidget {
    Q_OBJECT
public:
    explicit WelcomePage(QWidget* parent=nullptr);
    void refresh();
    static void remember(const QString& path);
signals:
    void newRequested();
    void openRequested();
    void recentRequested(const QString& path);
    void resumeRequested();
    void examplesRequested();
    void tutorialRequested();
    void chooseLanguageRequested();
    void chooseDesignRequested();
    void darkModeChanged(bool enabled);
private:
    QVBoxLayout* recentLayout_;
    QStackedWidget* pages_;
};

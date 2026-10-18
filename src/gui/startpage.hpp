#pragma once
#include <QWidget>
class QVBoxLayout;
class QStackedWidget;
class WelcomePage final:public QWidget {
    Q_OBJECT
public:
    explicit WelcomePage(QWidget* parent=nullptr);
    void refresh();
    void showCourse();
    static void remember(const QString& path);
signals:
    void newRequested();
    void openRequested();
    void recentRequested(const QString& path);
    void resumeRequested();
    void examplesRequested();
    void tutorialRequested();
    void preferencesRequested();
    void changelogRequested();
    void pythonRequested();
    void lessonRequested(const QString& id);
    void chooseLanguageRequested();
    void chooseDesignRequested();
    void darkModeChanged(bool enabled);
private:
    QVBoxLayout* recentLayout_;
    QStackedWidget* pages_;
};

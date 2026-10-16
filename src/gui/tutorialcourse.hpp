#pragma once
#include <QWidget>
class QListWidget;class QTextBrowser;class QPushButton;class QProgressBar;
class TutorialCourse final:public QWidget {
    Q_OBJECT
public:
    explicit TutorialCourse(QWidget* parent=nullptr);
    void refresh();
    static QString instructions(const QString& id);
    static bool complete(const QString& id);
    static bool available(const QString& id);
signals:void lessonRequested(const QString& example);
private:
    QListWidget* lessons_;QTextBrowser* text_;QPushButton* start_;QProgressBar* progress_;
    void showLesson();
};

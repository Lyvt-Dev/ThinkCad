#pragma once
#include "gdbsession.hpp"
#include <QWidget>
#include <QPointer>
class QComboBox;
class QLabel;
class QTreeWidget;
class DebugTools final:public QWidget {
    Q_OBJECT
public:
    explicit DebugTools(QWidget* parent=nullptr);
    void clear();
    void addBoard(const QString& board,GdbSession* session);
    void selectBoard(const QString& board);
signals:
    void startRequested();
    void stopRequested();
    void continueRequested(const QString& board);
    void sourceStopped(const QString& board,const QString& file,int line);
private:
    QComboBox* boards_;
    QLabel* status_;
    QTreeWidget *locals_,*registers_;
    QMap<QString,QPointer<GdbSession>> sessions_;
    GdbSession* selected()const;
};

#pragma once
#include "localcollaboration.hpp"
#include <QWidget>
#include <QStringList>
class QLineEdit;
class QSpinBox;
class QLabel;
class QPlainTextEdit;
class QPushButton;
class CollaborationTools final:public QWidget {
    Q_OBJECT
public:
    explicit CollaborationTools(LocalCollaboration* session,QWidget* parent=nullptr);
    void showConflict(const QStringList& paths);
    void clearConflict();
    void refresh();
signals:
    void hostRequested();
    void joinRequested(const QString& address,quint16 port,const QString& ticket);
    void acceptRemoteRequested();
    void keepLocalRequested();
    void compareRequested();
private:
    LocalCollaboration* session_;
    QLineEdit *address_,*ticket_;
    QSpinBox* port_;
    QLabel *status_,*conflicts_;
    QPushButton *accept_,*keep_,*compare_;
    QStringList conflictPaths_;
};

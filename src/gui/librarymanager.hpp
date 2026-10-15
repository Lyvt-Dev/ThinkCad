#pragma once
#include <QDialog>
#include <QJsonArray>
#include <QNetworkAccessManager>
#include <QTemporaryDir>
#include <memory>
class QLineEdit;class QTreeWidget;class QLabel;class QPushButton;
struct LibraryPackage {QString name,version,description,url,sha256;QStringList architectures;QJsonArray dependencies;};
class ArduinoLibraries {
public:
    static QList<LibraryPackage> parse(const QByteArray& json);
    static QList<LibraryPackage> resolve(const QList<LibraryPackage>& index,const QString& name,const QString& version);
    static void extract(const QByteArray& zip,const QString& destination);
};
class LibraryManagerDialog final:public QDialog {
    Q_OBJECT
public:explicit LibraryManagerDialog(QWidget* parent=nullptr);
private:
    QNetworkAccessManager network_;
    QList<LibraryPackage> index_,plan_;
    QTreeWidget* list_;QLineEdit* search_;QLabel* status_;QPushButton *install_,*refresh_;
    QString root_;QStringList staged_;
    std::unique_ptr<QTemporaryDir> staging_;
    bool busy_=false;
    void fetchIndex();void populate();void startInstall();void nextPackage();void fail(const QString& message);
};

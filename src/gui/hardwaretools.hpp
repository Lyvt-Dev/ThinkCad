#pragma once
#include "projectio.hpp"
#include <QObject>
#include <QProcess>
#include <QTemporaryDir>
#include <memory>
class HardwareBuild final:public QObject {
    Q_OBJECT
public:
    explicit HardwareBuild(QObject* parent=nullptr);
    ~HardwareBuild()override;
    void compile(const SketchProject& project,const QString& cli,const QString& fqbn,const QStringList& libraries={});
    void upload(const QString& port);
    void cancel();
    bool busy()const{return process_.state()!=QProcess::NotRunning;}
    bool compiled()const{return compiled_;}
    static QString suggestedFqbn(const std::string& model);
signals:
    void log(const QString& text);
    void finished(bool success);
private:
    QProcess process_;std::unique_ptr<QTemporaryDir> temp_;QString cli_,fqbn_,sketchPath_,buildPath_;bool compiled_=false,uploading_=false;
};
class QWidget;
void showHardwareTools(QWidget* parent,const SketchProject& project,const std::string& model,const QStringList& projectLibraries={});

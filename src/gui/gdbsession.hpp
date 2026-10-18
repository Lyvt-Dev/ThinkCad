#pragma once
#include "gdbmi.hpp"
#include <QObject>
#include <QProcess>
#include <QProcessEnvironment>
#include <QMap>
#include <functional>

// Controls the native host process. These are host registers, not MCU registers.
class GdbSession final : public QObject {
    Q_OBJECT
public:
    struct Breakpoint {QString file;int line=0;};
    explicit GdbSession(QObject* parent=nullptr);
    ~GdbSession() override;
    void start(const QString& debugger,const QString& executable,const QProcessEnvironment& environment,const QList<Breakpoint>& breakpoints={});
    void stop();
    void run();
    void resume();
    void interrupt();
    void next();
    void step();
    void finish();
    void evaluate(const QString& expression);
    void inspectVariables();
    void inspectRegisters();
    void insertBreakpoint(const QString& file,int line);
    void removeBreakpoint(const QString& number);
    bool active()const{return process_.state()!=QProcess::NotRunning;}
    bool stopped()const{return stopped_;}
    qint64 inferiorPid()const{return inferiorPid_;}
    int command(const QByteArray& command,std::function<void(const GdbMi::Record&)> reply={});
signals:
    void ready();
    void running();
    void halted(const QString& file,int line,const QString& reason);
    void exited();
    void log(const QString& text);
    void error(const QString& text);
    void variables(const QVariantList& values);
    void registers(const QVariantList& values);
    void evaluated(const QString& expression,const QString& value);
    void breakpointInserted(const QString& number,const QString& file,int line);
private:
    QProcess process_;
    QByteArray pending_;
    QMap<int,std::function<void(const GdbMi::Record&)>> replies_;
    int token_=0;
    bool stopped_=false,closing_=false;
    qint64 inferiorPid_=0;
    void read();
    void handle(const GdbMi::Record& record);
};

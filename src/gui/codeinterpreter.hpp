#pragma once
#include <QObject>
#include <QProcess>
#include <QTemporaryDir>
#include <QTimer>
#include <QMap>
#include <QSet>
#include <memory>
#include <cstdint>
// A compiler-backed runner. Native sketch code executes in a child process, never in the GUI.
// Process isolation is NOT a security sandbox: run only trusted code/libraries.
class CodeInterpreter final : public QObject {
    Q_OBJECT
public:
    explicit CodeInterpreter(QObject* parent = nullptr);
    ~CodeInterpreter() override;
    void start(const QString& sketch, const QString& compiler, const QString& sdk, const QStringList& libraries,const QMap<QString,QString>& projectFiles = {},const QString& boardModel="esp8266",bool deferExecution=false);
    void stop();
    void setPaused(bool paused);
    void stepOnce();
    void runPrepared();
    bool prepared()const{return prepared_;}
    void answer(int value);
    void setSimulationSpeed(double multiplier);
    double simulationSpeed() const {return speedPermille_/1000.0;}
    bool active() const { return active_; }
signals:
    void protocol(const QByteArray& line);
    void log(const QString& message);
    void activeChanged(bool active);
    void readyToRun();
private:
    QProcess compiler_, worker_;
    QTimer watchdog_, rateWindow_;
    std::unique_ptr<QTemporaryDir> temp_;
    QByteArray pending_;
    int linesThisSecond_ = 0;
    int speedPermille_ = 1000;
    bool paused_=false,waitingForSpeed_=false,stepping_=false;
    std::uint64_t protocolTime_=0,stepStart_=0;
    bool active_ = false,prepared_=false,deferExecution_=false;
    void setActive(bool active);
    void readProtocol();
    struct LibraryUnit {QString root,base;QStringList sources;bool builtin=false;};
    QList<LibraryUnit> libraries_;QSet<int> selectedLibraries_;
    QStringList compileArguments_;QString compilerExecutable_,outputPath_;
    QByteArray dependencyOutput_;bool scanning_=false;int dependencyPasses_=0;
    void launchCompilation(bool scan);
    void compilationFinished(int code,QProcess::ExitStatus status);
};

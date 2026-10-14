#include "codeinterpreter.hpp"
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QProcessEnvironment>
CodeInterpreter::CodeInterpreter(QObject* parent) : QObject(parent) {
    watchdog_.setSingleShot(true);
    connect(&watchdog_, &QTimer::timeout, this, [this] { emit log("Ausführung abgebrochen: Zeitlimit / fehlende Rückmeldung."); stop(); });
    rateWindow_.setInterval(1000);
    connect(&rateWindow_, &QTimer::timeout, this, [this] { linesThisSecond_=0; });
    connect(&compiler_, &QProcess::readyReadStandardError, this, [this] { emit log(QString::fromLocal8Bit(compiler_.readAllStandardError()).left(16000)); });
    connect(&compiler_, &QProcess::readyReadStandardOutput, this, [this] { emit log(QString::fromLocal8Bit(compiler_.readAllStandardOutput()).left(16000)); });
    connect(&compiler_, &QProcess::errorOccurred, this, [this](QProcess::ProcessError e) { if (e==QProcess::FailedToStart) { emit log("Compiler konnte nicht gestartet werden. Unter Einstellungen einen g++-Compiler wählen."); stop(); } });
    connect(&compiler_, qOverload<int,QProcess::ExitStatus>(&QProcess::finished), this, [this](int code,QProcess::ExitStatus status) {
        if (!active_) return;
        if (code || status!=QProcess::NormalExit) { emit log(QString("Kompilierung fehlgeschlagen (Exit %1).").arg(code)); stop(); return; }
        emit log("Kompiliert. Starte setup() und loop().");
        watchdog_.start(10000); rateWindow_.start();
        worker_.start(temp_->filePath(
#ifdef Q_OS_WIN
            "sketch.exe"
#else
            "sketch"
#endif
        ));
    });
    connect(&worker_, &QProcess::readyReadStandardOutput, this, &CodeInterpreter::readProtocol);
    connect(&worker_, &QProcess::readyReadStandardError, this, [this] { emit log(QString::fromLocal8Bit(worker_.readAllStandardError()).left(16000)); });
    connect(&worker_, &QProcess::errorOccurred, this, [this](QProcess::ProcessError e) { if (e==QProcess::FailedToStart) { emit log("Sketch-Prozess konnte nicht gestartet werden."); stop(); } });
    connect(&worker_, qOverload<int,QProcess::ExitStatus>(&QProcess::finished), this, [this](int code,QProcess::ExitStatus status) { emit log(QString("Sketch beendet (%1, %2).").arg(code).arg(status==QProcess::NormalExit?"normal":"Abbruch")); watchdog_.stop(); rateWindow_.stop(); setActive(false); });
}
CodeInterpreter::~CodeInterpreter() { stop(); }
void CodeInterpreter::setActive(bool active) { if (active_ != active) {active_=active; emit activeChanged(active);} }
void CodeInterpreter::stop() {
    setActive(false); watchdog_.stop(); rateWindow_.stop();
    for (auto* p : {&compiler_,&worker_}) if (p->state()!=QProcess::NotRunning) { p->kill(); p->waitForFinished(1000); }
    pending_.clear();
}
void CodeInterpreter::start(const QString& sketch,const QString& compiler,const QString& sdk,const QStringList& libraries) {
    stop(); temp_=std::make_unique<QTemporaryDir>();
    if (!temp_->isValid() || !QFileInfo::exists(sdk+"/runner.cpp")) { emit log("Temporäres Verzeichnis oder Simulator-SDK fehlt."); return; }
    QFile file(temp_->filePath("sketch.ino"));
    if (!file.open(QIODevice::WriteOnly) || file.write(sketch.toUtf8()) < 0) {emit log("Sketch konnte nicht gespeichert werden."); return;} file.close();
    QString source=QDir::fromNativeSeparators(file.fileName());
    QStringList args{"-std=c++20","-O0","-g","-pthread","-I"+sdk,"-DTHINKERCAD_SKETCH=\""+source+"\"",sdk+"/runner.cpp"};
    int sources=0;
    for (const auto& lib : libraries) {
        const QString root=QFileInfo::exists(lib+"/src")?lib+"/src":lib;
        args << "-I"+root << "-I"+lib;
        // Traditional libraries compile root/utility; modern Arduino libraries compile src recursively.
        const auto flags = root.endsWith("/src") ? QDirIterator::Subdirectories : QDirIterator::NoIteratorFlags;
        QDirIterator files(root,{"*.cpp","*.cc","*.cxx"},QDir::Files,flags);
        while(files.hasNext()) { if (++sources>200) {emit log("Zu viele Bibliotheksquellen (max. 200).");return;} args << files.next(); }
        if (root==lib && QDir(lib+"/utility").exists()) {
            args << "-I"+lib+"/utility";
            QDirIterator utility(lib+"/utility",{"*.cpp","*.cc","*.cxx"},QDir::Files,QDirIterator::Subdirectories);
            while(utility.hasNext()) {if(++sources>200){emit log("Zu viele Bibliotheksquellen.");return;}args<<utility.next();}
        }
    }
#ifdef Q_OS_WIN
    // MSYS2's native headers live below the toolchain sysroot. Explicitly relocate
    // it when using an absolute compiler path (including the bundled compiler).
    if(QFileInfo(compiler).isAbsolute()) {
        const auto root=QDir(QFileInfo(compiler).absolutePath()+"/..").absolutePath();
        if(QFileInfo::exists(root+"/include/stdlib.h"))args<<"--sysroot="+QDir::fromNativeSeparators(root);
    }
    args << "-static-libgcc" << "-static-libstdc++";
#endif
    args << "-o" << temp_->filePath(
#ifdef Q_OS_WIN
    "sketch.exe"
#else
    "sketch"
#endif
    );
    pending_.clear(); linesThisSecond_=0; setActive(true); watchdog_.start(120000);
    compiler_.setWorkingDirectory(temp_->path()); worker_.setWorkingDirectory(temp_->path());
    // MinGW runtime DLLs (e.g. winpthread) must remain discoverable for child sketches.
    auto environment=QProcessEnvironment::systemEnvironment();
    const auto compilerDir=QFileInfo(compiler).absolutePath();
    if (QFileInfo(compiler).isAbsolute()) environment.insert("PATH",compilerDir+QDir::listSeparator()+environment.value("PATH"));
    compiler_.setProcessEnvironment(environment);
    worker_.setProcessEnvironment(environment);
    compiler_.start(compiler,args);
}
void CodeInterpreter::answer(int value) {worker_.write(QByteArray::number(value)+'\n');}
void CodeInterpreter::readProtocol() {
    watchdog_.start(10000); pending_+=worker_.readAllStandardOutput();
    if (pending_.size()>1048576) {emit log("Sketch-Protokoll überschreitet das Pufferlimit.");stop();return;}
    while (true) {
        const auto newline=pending_.indexOf('\n'); if(newline<0) break;
        if (++linesThisSecond_>5000 || newline>4096) {emit log("Sketch-Protokoll überlastet. delay() oder yield() verwenden.");stop();return;}
        auto line=pending_.left(newline).trimmed(); pending_.remove(0,newline+1);
        emit protocol(line); if(!active_) break;
    }
}

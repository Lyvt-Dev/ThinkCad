#include "codeinterpreter.hpp"
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <algorithm>
#include <cmath>
#include <QRegularExpression>
CodeInterpreter::CodeInterpreter(QObject* parent) : QObject(parent) {
    watchdog_.setSingleShot(true);
    connect(&watchdog_, &QTimer::timeout, this, [this] { emit log("Ausführung abgebrochen: Zeitlimit / fehlende Rückmeldung."); stop(); });
    rateWindow_.setInterval(1000);
    connect(&rateWindow_, &QTimer::timeout, this, [this] { linesThisSecond_=0; });
    connect(&compiler_, &QProcess::readyReadStandardError, this, [this] { emit log(QString::fromLocal8Bit(compiler_.readAllStandardError()).left(16000)); });
    connect(&compiler_,&QProcess::readyReadStandardOutput,this,[this]{const auto bytes=compiler_.readAllStandardOutput();if(!active_)return;if(scanning_){dependencyOutput_+=bytes;if(dependencyOutput_.size()>8*1024*1024){emit log("Compiler-Abhängigkeiten zu groß.");stop();}}else emit log(QString::fromLocal8Bit(bytes).left(16000));});
    connect(&compiler_, &QProcess::errorOccurred, this, [this](QProcess::ProcessError e) { if (e==QProcess::FailedToStart) { emit log("Compiler konnte nicht gestartet werden. Unter Einstellungen einen g++-Compiler wählen."); stop(); } });
    connect(&compiler_,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),this,&CodeInterpreter::compilationFinished);
    connect(&worker_, &QProcess::readyReadStandardOutput, this, &CodeInterpreter::readProtocol);
    connect(&worker_, &QProcess::readyReadStandardError, this, [this] { emit log(QString::fromLocal8Bit(worker_.readAllStandardError()).left(16000)); });
    connect(&worker_, &QProcess::errorOccurred, this, [this](QProcess::ProcessError e) { if (e==QProcess::FailedToStart) { emit log("Sketch-Prozess konnte nicht gestartet werden."); stop(); } });
    connect(&worker_, qOverload<int,QProcess::ExitStatus>(&QProcess::finished), this, [this](int code,QProcess::ExitStatus status) { emit log(QString("Sketch beendet (%1, %2).").arg(code).arg(status==QProcess::NormalExit?"normal":"Abbruch")); watchdog_.stop(); rateWindow_.stop(); setActive(false); });
}
CodeInterpreter::~CodeInterpreter() { stop(); }
void CodeInterpreter::setActive(bool active) { if (active_ != active) {active_=active; emit activeChanged(active);} }
void CodeInterpreter::stop() {
    paused_=false;waitingForSpeed_=false;stepping_=false;protocolTime_=0;prepared_=false;setActive(false); watchdog_.stop(); rateWindow_.stop();
    for (auto* p : {&compiler_,&worker_}) if (p->state()!=QProcess::NotRunning) { p->kill(); p->waitForFinished(1000); }
    pending_.clear();
}
void CodeInterpreter::start(const QString& sketch,const QString& compiler,const QString& sdk,const QStringList& libraries,const QMap<QString,QString>& projectFiles,const QString& boardModel,bool deferExecution) {
    stop();deferExecution_=deferExecution;libraries_.clear();selectedLibraries_.clear();dependencyPasses_=0;temp_=std::make_unique<QTemporaryDir>();
    if (!temp_->isValid() || !QFileInfo::exists(sdk+"/runner.cpp")) { emit log("Temporäres Verzeichnis oder Simulator-SDK fehlt."); return; }
    QFile file(temp_->filePath("sketch.ino"));
    if (!file.open(QIODevice::WriteOnly) || file.write(sketch.toUtf8()) < 0) {emit log("Sketch konnte nicht gespeichert werden."); return;} file.close();
    QString source=QDir::fromNativeSeparators(file.fileName());
    QStringList args{"-std=c++20","-DTHINKERCAD_SIMULATION=1","-O0","-g","-pthread","-I"+sdk,"-DTHINKERCAD_SKETCH=\""+source+"\"",sdk+"/runner.cpp"};
    if(boardModel=="arduino-uno")args<<"-DTHINKERCAD_ARDUINO_UNO=1";else if(boardModel=="arduino-nano")args<<"-DTHINKERCAD_ARDUINO_NANO=1";else if(boardModel=="esp32")args<<"-DTHINKERCAD_ESP32=1";else if(boardModel=="esp32-s3")args<<"-DTHINKERCAD_ESP32_S3=1";
    args << "-I"+temp_->path() << "-I"+temp_->filePath("src") << "-I"+temp_->filePath("include");
    int sources=0;
    for(auto it=projectFiles.begin();it!=projectFiles.end();++it) {
        const auto name=it.key();const auto clean=QDir::cleanPath(name);
        if(name.isEmpty()||name.startsWith('/')||name.contains('\\')||name.contains(':')||name.split('/').contains("..")||clean!=name||++sources>100||it.value().toUtf8().size()>2*1024*1024){emit log("Ungültige Projektdateien.");return;}
        const auto path=temp_->filePath(name);QDir().mkpath(QFileInfo(path).absolutePath());QFile auxiliary(path);
        const auto bytes=it.value().toUtf8();if(!auxiliary.open(QIODevice::WriteOnly)||auxiliary.write(bytes)!=bytes.size()){emit log("Projektdatei konnte nicht vorbereitet werden.");return;}auxiliary.close();
        if(QStringList{"cpp","cc","cxx"}.contains(QFileInfo(name).suffix().toLower()))args<<path;
    }
    for(const auto& lib:libraries){
        if(libraries_.size()>=500){emit log("Zu viele registrierte Bibliotheken.");return;}
        const auto root=QFileInfo::exists(lib+"/src")?lib+"/src":lib;args<<"-I"+root<<"-I"+lib;
        QFile metadata(lib+"/library.properties");QString name;if(metadata.open(QIODevice::ReadOnly)){auto match=QRegularExpression("(?:^|\\n)name\\s*=\\s*([^\\r\\n]+)").match(QString::fromUtf8(metadata.readAll()));name=match.captured(1).trimmed();}
        const bool builtin=QStringList{"Adafruit GFX Library","Adafruit SSD1306","Adafruit BusIO","LiquidCrystal","LiquidCrystal I2C","LiquidCrystal_I2C","Servo","IRremote","Adafruit PN532","RF24","rc-switch"}.contains(name,Qt::CaseInsensitive);
        LibraryUnit unit{QDir::fromNativeSeparators(QFileInfo(root).absoluteFilePath()),QDir::fromNativeSeparators(QFileInfo(lib).absoluteFilePath()),{},builtin};
        if(!builtin){const auto flags=root.endsWith("/src")?QDirIterator::Subdirectories:QDirIterator::NoIteratorFlags;QDirIterator files(root,{"*.cpp","*.cc","*.cxx"},QDir::Files,flags);while(files.hasNext()){unit.sources<<files.next();if(unit.sources.size()>200){emit log("Bibliothek enthält zu viele Quelldateien.");return;}}
            if(root==lib&&QDir(lib+"/utility").exists()){args<<"-I"+lib+"/utility";QDirIterator files(lib+"/utility",{"*.cpp","*.cc","*.cxx"},QDir::Files,QDirIterator::Subdirectories);while(files.hasNext()){unit.sources<<files.next();if(unit.sources.size()>200){emit log("Bibliothek enthält zu viele Quelldateien.");return;}}}}
        libraries_<<unit;
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
    const auto output=args.lastIndexOf("-o");outputPath_=args.takeAt(output+1);args.removeAt(output);compileArguments_=args;compilerExecutable_=compiler;launchCompilation(true);
}
void CodeInterpreter::setSimulationSpeed(double multiplier) {
    if(std::isfinite(multiplier))speedPermille_=static_cast<int>(std::lround(std::clamp(multiplier,0.1,10.0)*1000));
}
void CodeInterpreter::answer(int value) {worker_.write(QByteArray::number(value)+'\n');}
void CodeInterpreter::readProtocol() {
    if(!active_)return;
    const auto received=worker_.readAllStandardOutput();if(!received.isEmpty()&&!waitingForSpeed_)watchdog_.start(10000);pending_+=received;
    if (pending_.size()>1048576) {emit log("Sketch-Protokoll überschreitet das Pufferlimit.");stop();return;}
    int dispatched=0;
    while (true) {
        if(dispatched++>=64){QTimer::singleShot(0,this,&CodeInterpreter::readProtocol);break;}
        const auto newline=pending_.indexOf('\n'); if(newline<0) break;
        if (++linesThisSecond_>5000 || newline>4096) {emit log("Sketch-Protokoll überlastet. delay() oder yield() verwenden.");stop();return;}
        auto line=pending_.left(newline).trimmed(); pending_.remove(0,newline+1);
        if(line=="SPEED"){if(paused_&&!stepping_){waitingForSpeed_=true;watchdog_.stop();}else answer(speedPermille_);}else{if(line.startsWith("TIME ")){bool ok=false;auto time=line.mid(5).toULongLong(&ok);if(ok){protocolTime_=time;if(stepping_&&time>stepStart_)stepping_=false;}}emit protocol(line);} if(!active_) break;
    }
}

void CodeInterpreter::launchCompilation(bool scan){
    scanning_=scan;dependencyOutput_.clear();auto arguments=compileArguments_;int count=0;for(int i:selectedLibraries_)for(const auto& source:libraries_.at(i).sources){if(++count>200){emit log("Zu viele verwendete Bibliotheksquellen.");stop();return;}arguments<<source;}
    if(scan)arguments<<"-MM"<<"-MT"<<"sketch";else arguments<<"-o"<<outputPath_;compiler_.start(compilerExecutable_,arguments);
}
void CodeInterpreter::compilationFinished(int code,QProcess::ExitStatus status){
    if(!active_)return;if(code||status!=QProcess::NormalExit){emit log(QString("Kompilierung fehlgeschlagen (Exit %1).").arg(code));stop();return;}
    if(scanning_){dependencyOutput_+=compiler_.readAllStandardOutput();auto paths=QString::fromLocal8Bit(dependencyOutput_).replace("\\ "," ").replace("\\:",":");bool changed=false;
        for(int i=0;i<libraries_.size();++i){const auto& lib=libraries_.at(i);
#ifdef Q_OS_WIN
            constexpr auto sensitivity=Qt::CaseInsensitive;
#else
            constexpr auto sensitivity=Qt::CaseSensitive;
#endif
            if(!selectedLibraries_.contains(i)&&(paths.contains(lib.root+"/",sensitivity)||paths.contains(lib.base+"/",sensitivity))){selectedLibraries_.insert(i);changed=true;}}
        if(changed){if(++dependencyPasses_>100){emit log("Bibliotheks-Abhängigkeitsauflösung überschritten.");stop();return;}launchCompilation(true);}else launchCompilation(false);return;
    }
    prepared_=true;emit readyToRun();if(!deferExecution_)runPrepared();
}

void CodeInterpreter::runPrepared(){if(!active_||!prepared_)return;prepared_=false;emit log("Kompiliert. Starte setup() und loop().");watchdog_.start(10000);rateWindow_.start();worker_.start(outputPath_);}

void CodeInterpreter::setPaused(bool paused){paused_=paused;if(!paused_)stepping_=false;if(!paused_&&waitingForSpeed_){waitingForSpeed_=false;watchdog_.start(10000);answer(speedPermille_);}}
void CodeInterpreter::stepOnce(){paused_=true;stepping_=true;stepStart_=protocolTime_;if(waitingForSpeed_){waitingForSpeed_=false;watchdog_.start(10000);answer(speedPermille_);}}

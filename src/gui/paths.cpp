#include <QSettings>
#include "paths.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
QString AppPaths::dataRoot(){
    const auto executable=QCoreApplication::applicationDirPath();
    // Build tree and Windows installation keep resources next to the executable.
    if(QFileInfo::exists(executable+"/sdk/runner.cpp"))return executable;
#ifdef THINKCAD_RESOURCE_PATH
    return QDir(executable).absoluteFilePath(QStringLiteral(THINKCAD_RESOURCE_PATH));
#else
    return executable;
#endif
}
QString AppPaths::sdk(){return dataRoot()+"/sdk";}
QString AppPaths::examples(){return dataRoot()+"/examples";}

QString AppPaths::bundledCompiler(){auto bundled=QCoreApplication::applicationDirPath()+
#ifdef Q_OS_WIN
"/toolchain/bin/g++.exe";
#else
"/toolchain/bin/g++";
#endif
return QFileInfo::exists(bundled)?bundled:QString();}
QString AppPaths::compiler(){const auto bundled=bundledCompiler();return QSettings().value("compiler",bundled.isEmpty()?QString("g++"):bundled).toString();}

QString AppPaths::debugger(){
    const auto chosen=QSettings().value("debugger").toString().trimmed();if(!chosen.isEmpty())return chosen;
    const auto compilerPath=compiler();const auto bundled=QFileInfo(compilerPath).absolutePath()+
#ifdef Q_OS_WIN
        "/gdb.exe";
#else
        "/gdb";
#endif
    if(QFileInfo(compilerPath).isAbsolute()&&QFileInfo::exists(bundled))return bundled;
    return QStandardPaths::findExecutable("gdb");
}

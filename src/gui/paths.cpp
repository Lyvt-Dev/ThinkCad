#include "paths.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
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

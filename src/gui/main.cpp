#include "mainwindow.hpp"
#include "videoimport.hpp"
#include <QTemporaryDir>
#include <QFile>
#include <QDebug>
#include <QApplication>
#include <QFileInfo>
#include <QPalette>
#include <QStyleFactory>
#include <QTimer>
#include <QSettings>
#include "theme.hpp"
#ifdef Q_OS_WIN
#define NOMINMAX
#include <windows.h>
#endif
int main(int argc,char** argv){
#ifdef Q_OS_WIN
    HANDLE mutex=CreateMutexW(nullptr,FALSE,L"ThinkerCadDesktopMain");
    if(!mutex||GetLastError()==ERROR_ALREADY_EXISTS){if(mutex)CloseHandle(mutex);return 1;}
#endif
    QApplication app(argc,argv);app.setOrganizationName("Lyvt-Dev");app.setApplicationName("ThinkCad");{QSettings current;if(current.allKeys().isEmpty()){QSettings legacy("Lyvt-Dev","ThinkerCad");for(const auto& key:legacy.allKeys())current.setValue(key,legacy.value(key));}}app.setApplicationVersion(THINKERCAD_VERSION);app.setWindowIcon(QIcon(":/brand/logo.png"));app.setFont(QFont("Segoe UI",10));
#ifdef Q_OS_WIN
    for(const auto& candidate:QStringList{"windows11","windowsvista","Windows"})if(QStyleFactory::keys().contains(candidate,Qt::CaseInsensitive)){app.setStyle(QStyleFactory::create(candidate));break;}
#else
    app.setStyle(QStyleFactory::create("Fusion"));
#endif
#ifdef Q_OS_LINUX
    app.setDesktopFileName("io.github.LyvtDev.ThinkCad");
#endif
    Theme::apply(QSettings().value("darkMode",false).toBool());
    if(app.arguments().contains("--smoke-test")){
        QTemporaryDir data;const auto source=data.filePath("frames.webm");QString error;
        if(!QFile::copy(":/smoke/frames.webm",source)||!videoImportSmokeTest(source,&error)){qCritical()<<"Video import smoke failed:"<<error;return 1;}
        qInfo()<<"Video import smoke passed";
    }
    MainWindow window(app.arguments().contains("--smoke-test"));window.show();
    if(!app.arguments().contains("--smoke-test")){
        QString path;for(const auto& argument:app.arguments().mid(1))if(QFileInfo(argument).isFile()){path=argument;break;}
        QTimer::singleShot(0,&window,[&window,path]{if(path.isEmpty()||!window.openPath(path))window.showStartPage();});
    }
    const int result=app.exec();
#ifdef Q_OS_WIN
    CloseHandle(mutex);
#endif
    return result;
}

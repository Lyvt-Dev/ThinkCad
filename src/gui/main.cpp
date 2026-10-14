#include "mainwindow.hpp"
#include <QApplication>
#include <QStyleFactory>
#ifdef Q_OS_WIN
#define NOMINMAX
#include <windows.h>
#endif
int main(int argc,char** argv) {
#ifdef Q_OS_WIN
    HANDLE mutex=CreateMutexW(nullptr,FALSE,L"ThinkerCadDesktopMain");
    if(!mutex || GetLastError()==ERROR_ALREADY_EXISTS){if(mutex)CloseHandle(mutex);return 1;}
#endif
    QApplication app(argc,argv);app.setOrganizationName("Lyvt-Dev");app.setApplicationName("ThinkerCad");app.setApplicationVersion(THINKERCAD_VERSION);
    app.setStyle(QStyleFactory::create("Fusion"));
    app.setStyleSheet("QToolBar {spacing:10px;padding:9px;background:#ffffff;border-bottom:1px solid #dbe1e8;} QToolButton {padding:8px;color:#203247;} QDockWidget {font-weight:600;color:#203247;} QPlainTextEdit {background:#fff;color:#183047;border:1px solid #dbe1e8;} QStatusBar {background:#e9eef5;} QPushButton {padding:8px;} ");
    MainWindow window(app.arguments().contains("--smoke-test"));window.show();const int result=app.exec();
#ifdef Q_OS_WIN
    CloseHandle(mutex);
#endif
    return result;
}

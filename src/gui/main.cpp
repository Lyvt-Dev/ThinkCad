#include "mainwindow.hpp"
#include <QApplication>
#include <QFileInfo>
#include <QPalette>
#include <QStyleFactory>
#include <QTimer>
#ifdef Q_OS_WIN
#define NOMINMAX
#include <windows.h>
#endif
int main(int argc,char** argv){
#ifdef Q_OS_WIN
    HANDLE mutex=CreateMutexW(nullptr,FALSE,L"ThinkerCadDesktopMain");
    if(!mutex||GetLastError()==ERROR_ALREADY_EXISTS){if(mutex)CloseHandle(mutex);return 1;}
#endif
    QApplication app(argc,argv);app.setOrganizationName("Lyvt-Dev");app.setApplicationName("ThinkerCad");app.setApplicationVersion(THINKERCAD_VERSION);app.setWindowIcon(QIcon(":/brand/logo.png"));app.setFont(QFont("Segoe UI",10));
#ifdef Q_OS_WIN
    for(const auto& candidate:QStringList{"windows11","windowsvista","Windows"})if(QStyleFactory::keys().contains(candidate,Qt::CaseInsensitive)){app.setStyle(QStyleFactory::create(candidate));break;}
#else
    app.setStyle(QStyleFactory::create("Fusion"));
#endif
    QPalette palette;palette.setColor(QPalette::Window,QColor("#f4f7fb"));palette.setColor(QPalette::WindowText,QColor("#27364b"));palette.setColor(QPalette::Base,Qt::white);palette.setColor(QPalette::Text,QColor("#27364b"));palette.setColor(QPalette::Button,QColor("#ffffff"));palette.setColor(QPalette::ButtonText,QColor("#27364b"));palette.setColor(QPalette::Highlight,QColor("#0d9488"));palette.setColor(QPalette::HighlightedText,Qt::white);app.setPalette(palette);
    app.setStyleSheet("QMainWindow {background:#f4f7fb;} QMenuBar {background:#ffffff;padding:3px;color:#334155;} QMenuBar::item {padding:5px 9px;} QToolBar#speedBar {background:white;padding:4px;border-bottom:1px solid #e2e8f0;} QToolBar#commandBar {background:white;spacing:5px;padding:9px;border-bottom:1px solid #e2e8f0;} QToolButton {padding:7px 9px;border-radius:6px;color:#334155;} QToolButton:hover {background:#edf4f5;} QToolButton:checked {background:#e0f4f1;color:#0d9488;} QToolButton#runButton {background:#0d9488;color:white;padding:9px 15px;font-weight:600;} QToolButton#runButton:hover {background:#0f766e;} QLabel#brandName {font-size:18px;font-weight:700;color:#173747;} QLabel#sectionTitle {font-weight:600;color:#334155;} QLabel#muted {color:#7b8ba0;font-size:11px;} QDockWidget {color:#64748b;font-weight:600;} QDockWidget::title {background:#f4f7fb;padding:8px;} QMainWindow::separator {width:5px;height:5px;background:#e8edf4;} QPushButton {background:white;border:1px solid #dce4ee;border-radius:7px;padding:8px 10px;color:#334155;} QPushButton:hover {background:#edf8f6;border-color:#b3d8d2;} QPushButton#componentCard {text-align:left;padding:10px 12px;font-weight:500;} QPlainTextEdit {background:white;border:0;color:#27364b;selection-background-color:#cdebe7;selection-color:#173747;} QTabWidget::pane {border:0;background:white;} QTabBar::tab {background:#f1f5f9;padding:9px 12px;color:#718096;border-bottom:2px solid transparent;} QTabBar::tab:selected {background:white;color:#0d9488;border-bottom:2px solid #0d9488;} QDoubleSpinBox,QComboBox {background:white;border:1px solid #dce4ee;border-radius:5px;padding:7px;} QStatusBar {background:#ffffff;border-top:1px solid #e2e8f0;color:#748398;} QStatusBar::item {border:0;} ");
    MainWindow window(app.arguments().contains("--smoke-test"));window.show();
    if(!app.arguments().contains("--smoke-test"))for(const auto& argument:app.arguments().mid(1))if(QFileInfo(argument).isFile())QTimer::singleShot(0,&window,[&window,argument]{window.openPath(argument);});
    const int result=app.exec();
#ifdef Q_OS_WIN
    CloseHandle(mutex);
#endif
    return result;
}

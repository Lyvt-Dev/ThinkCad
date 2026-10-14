#pragma once
#include "thinkercad/simulation.hpp"
#include "codeinterpreter.hpp"
#include "updater.hpp"
#include <QMainWindow>
class QPlainTextEdit;
class QLabel;
class QAction;
class QGraphicsScene;
class MainWindow final : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(bool smokeTest=false);
private:
    thinkercad::Simulator simulator_;
    thinkercad::Esp8266Board board_;
    thinkercad::OledDisplay oled_;
    CodeInterpreter interpreter_;
    Updater updater_;
    QPlainTextEdit *editor_, *console_;
    QLabel* status_;
    QAction* run_;
    QGraphicsScene* scene_;
    class CircuitItem* boardItem_;
    class CircuitItem* oledItem_;
    QString currentFile_;
    bool smokeTest_;
    bool smokeDone_ = false;
    int writes_ = 0;
    void runSketch();
    void process(const QByteArray& line);
    void libraries();
    void appendLog(const QString& message);
    void refreshScene();
    bool saveSketch();
    bool maybeSave();
    void closeEvent(QCloseEvent* event) override;
};

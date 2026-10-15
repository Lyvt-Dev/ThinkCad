#pragma once
#include "thinkercad/circuit.hpp"
#include "codeinterpreter.hpp"
#include "circuitcanvas.hpp"
#include "projectio.hpp"
#include "updater.hpp"
#include <QMainWindow>
class QPlainTextEdit;
class QLabel;
class QAction;
class QTabWidget;
class QDoubleSpinBox;
class QComboBox;
class QCheckBox;
class QPushButton;
class QStackedWidget;
class WelcomePage;
class MainWindow final : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(bool smokeTest=false);
    ~MainWindow() override;
    bool openPath(const QString& path);
    void loadDemo();
    bool loadExample(const QString& id);
    void startSimulation();
    void resetSimulation();
    void showStartPage();
    bool startPageVisible() const;
    bool saveProjectTo(const QString& path);
    CircuitCanvas* canvas(){return canvas_;}
    const thinkercad::CircuitDocument& circuit()const{return circuit_;}
    const thinkercad::CircuitState& circuitState()const{return circuitState_;}
    bool simulationActive()const{return interpreter_.active();}
    QString sketchText()const;
private:
    thinkercad::Simulator simulator_;
    thinkercad::Esp8266Board board_;
    thinkercad::CircuitDocument circuit_;
    thinkercad::CircuitState circuitState_;
    std::map<std::string,thinkercad::OledDisplay> displays_;
    std::map<int,std::string> displayBindings_;
    int busSda_=4,busScl_=5;
    bool busStarted_=false;
    CodeInterpreter interpreter_;
    Updater updater_;
    CircuitCanvas* canvas_;
    QStackedWidget* workspace_;
    WelcomePage* startPage_;
    QByteArray editorLayout_;
    void showEditor();
    QTabWidget* codeTabs_;
    QPlainTextEdit* console_;
    QLabel *status_,*projectLabel_,*selectionLabel_;
    QDoubleSpinBox* propertyValue_;
    QComboBox* propertyAddress_;
    QCheckBox* propertyPressed_;
    QPushButton* activeBoardButton_;
    QAction *run_,*undo_,*redo_;
    SketchProject sketch_;
    QString projectFile_;
    bool inputDirty_=false;
    bool smokeTest_,smokeDone_=false,renderDirty_=true;
    int writes_=0;
    void process(const QByteArray& line);
    void libraries();
    void examplesDialog();
    void appendLog(const QString& message);
    void updateCircuit(bool stopOnError=false);
    void updateInspector();
    void refreshTitle();
    void setSketch(const SketchProject& sketch);
    SketchProject currentSketch()const;
    void newProject();
    void openDialog();
    bool saveProject();
    bool exportSketch();
    bool maybeSave();
    bool dirty()const;
    void closeEvent(QCloseEvent* event) override;
};

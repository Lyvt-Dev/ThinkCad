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
    bool simulationActive()const;
    void stopSimulation();
    QString sketchText()const;
private:
    struct BoardRuntime {
        explicit BoardRuntime(QObject* parent):interpreter(parent){}
        thinkercad::Simulator clock;
        thinkercad::Esp8266Board board;
        CodeInterpreter interpreter;
        std::map<int,std::string> displayBindings;
        int busSda=4,busScl=5,writes=0;
        bool busStarted=false;
        std::string id;
    };
    std::map<std::string,std::unique_ptr<BoardRuntime>> runtimes_;
    QMap<QString,SketchProject> boardSketches_;
    std::string sketchBoard_;
    bool sketchesDirty_=false,starting_=false,stopping_=false;
    double speed_=1.0;
    void syncBoardSketches();
    void runtimeActiveChanged(bool active);
    thinkercad::Microseconds virtualTime()const;
    thinkercad::CircuitDocument circuit_;
    thinkercad::CircuitState circuitState_;
    std::map<std::string,thinkercad::OledDisplay> displays_;
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
    QComboBox* sketchBoards_;
    void process(BoardRuntime& runtime,const QByteArray& line);
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

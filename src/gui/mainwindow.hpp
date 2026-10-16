#pragma once
#include "thinkercad/circuit.hpp"
#include "codeinterpreter.hpp"
#include "circuitcanvas.hpp"
#include "projectio.hpp"
#include "updater.hpp"
#include "soundoutput.hpp"
#include "instruments.hpp"
#include "recovery.hpp"
#include "serialtools.hpp"
#include "peripheralhub.hpp"
#include "i2cdevices.hpp"
#include "thinkercad/spibus.hpp"
#include "thinkercad/sensorbus.hpp"
#include "thinkercad/pixelbus.hpp"
#include "thinkercad/uartbus.hpp"
#include "thinkercad/interrupts.hpp"
#include "localcollaboration.hpp"
#include "thinkercad/bustraffic.hpp"
#include <QMainWindow>
class QPlainTextEdit;
class QLineEdit;
class QLabel;
class QAction;
class QTabWidget;
class QDoubleSpinBox;
class QComboBox;
class QCheckBox;
class QPushButton;
class QStackedWidget;
class WelcomePage;
class QListWidget;
class QDockWidget;
class ModuleControls;
class DebugTools;
class CollaborationTools;
class SimulationInspector;
class QMenu;
class MainWindow final : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(bool smokeTest=false);
    ~MainWindow() override;
    bool openPath(const QString& path);
    void loadDemo();
    bool loadExample(const QString& id);
    void startSimulation();
    void startDebugging();
    bool hostCollaboration(const QHostAddress& address=QHostAddress::AnyIPv4,quint16 port=0);
    void joinCollaboration(const QString& address,quint16 port,const QString& ticket);
    LocalCollaboration* collaboration(){return &collaboration_;}
    thinkercad::BusTraffic& busTraffic(){return busTraffic_;}
    void resetSimulation();
    void showStartPage();
    bool startPageVisible() const;
    void autosave();
    void saveLocalAutomatically();
    bool recoverProject(const QString& path);
    void recoveryDialog();
    bool saveProjectTo(const QString& path);
    CircuitCanvas* canvas(){return canvas_;}
    const thinkercad::CircuitDocument& circuit()const{return circuit_;}
    const thinkercad::CircuitState& circuitState()const{return circuitState_;}
    bool simulationActive()const;
    thinkercad::Microseconds simulationTime()const{return virtualTime();}
    const thinkercad::Frame& displayFrame(const std::string& id)const{return displays_.at(id).frame();}
    void stopSimulation();
    thinkercad::ExecutionMode executionMode()const{return executionMode_;}
    bool setExecutionMode(thinkercad::ExecutionMode mode);
    QString sketchText()const;
private:
    thinkercad::BusTraffic busTraffic_;
    SimulationInspector* simulationInspector_=nullptr;
    void setupSimulationInspector(QMenu* view);
    LocalCollaboration collaboration_{this};
    CollaborationTools* collaborationTools_=nullptr;
    QTimer collaborationDebounce_;
    QJsonObject collaborationBase_,submittedLocal_;
    QString collaborationDraftPath_;
    bool saveCollaborationDraft();
    bool collaborationApplying_=false,collaborationJoining_=false,collaborationSubmitted_=false,collaborationConflict_=false,collaborationQueued_=false;
    void setupCollaboration(QMenu* view);
    void scheduleCollaboration();
    void publishCollaboration();
    void receiveCollaboration(const QJsonObject& document,quint64 revision,const QString& origin);
    void applyCollaboration(const QJsonObject& document);
    void flagCollaborationConflict(const QStringList& paths);
    void resolveCollaboration(bool preferLocal);
    void compareCollaboration();
    struct BoardRuntime {
        explicit BoardRuntime(QObject* parent):interpreter(parent){}
        thinkercad::Simulator clock;
        thinkercad::Esp8266Board board;
        CodeInterpreter interpreter;
        std::map<int,std::string> displayBindings;
        struct LcdBinding{std::string id;std::vector<int> pins;int address=-1;};
        std::map<int,LcdBinding> lcdBindings;
        int busSda=4,busScl=5,writes=0;
        bool busStarted=false,busHeld=false;
        std::string id;
    };
    std::map<std::string,std::unique_ptr<BoardRuntime>> runtimes_;
    QMap<QString,SketchProject> boardSketches_;
    std::string sketchBoard_;
    bool sketchesDirty_=false,starting_=false,stopping_=false;
    QString activeLesson_;
    qsizetype lessonSerialStart_=0;
    void checkLesson();
    thinkercad::ExecutionMode executionMode_=thinkercad::ExecutionMode::Simulation;
    QComboBox* executionModeChoice_=nullptr;
    void updateExecutionMode();
    double speed_=1.0;
    bool analogRunning_=false,paused_=false;
    thinkercad::Microseconds runUntil_=0;
    QAction* pause_;
    void pauseSimulation(bool on);
    void stepSimulation();
    thinkercad::Microseconds analogTarget_=0;
    thinkercad::TransientSimulation transient_;
    InstrumentPanel* instruments_;
    SerialTools* serial_;
    QListWidget* diagnostics_;
    QDockWidget* inspectorDock_=nullptr;
    bool hadInstruments_=false;
    QDoubleSpinBox* powerRating_;
    ModuleControls* moduleEditor_=nullptr;
    DebugTools* debugTools_=nullptr;
    bool debuggingRequested_=false,sourceDebugging_=false;
    void showDebugLocation(const QString& board,const QString& file,int line);
    void refreshModulePalette();
    PeripheralHub peripherals_;
    I2cDevices i2cDevices_;
    thinkercad::SensorBus sensors_;
    thinkercad::PixelBus pixels_;
    thinkercad::UartBus uart_;
    thinkercad::InterruptController interrupts_;
    thinkercad::SpiBus spiBus_{i2cDevices_.modules()};
    QLineEdit* netName_;
    QWidget* generatorProperties_;
    QComboBox* generatorShape_;
    QDoubleSpinBox *generatorAmplitude_,*generatorOffset_,*generatorDuty_;

    void syncBoardSketches();
    void runtimeActiveChanged(bool active);
    thinkercad::Microseconds virtualTime()const;
    thinkercad::CircuitDocument circuit_;
    thinkercad::CircuitState circuitState_;
    std::map<std::string,thinkercad::OledDisplay> displays_;
    Updater updater_;
    SoundOutput sound_;
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
    ProjectRecovery recovery_;
    QMap<QString,QByteArray> projectBundle_;
    std::unique_ptr<QTemporaryDir> bundledDirectory_;
    QStringList projectLibraries_;
    void loadBundle(const SavedProject& project);
    SavedProject snapshot();
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

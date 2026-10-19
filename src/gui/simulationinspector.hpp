#pragma once
#include "thinkercad/circuit.hpp"
#include "thinkercad/moduledevices.hpp"
#include "thinkercad/uartbus.hpp"
#include "thinkercad/bustraffic.hpp"
#include <QWidget>
#include <QTimer>
#include <QByteArray>
#include <functional>
#include <deque>
class QComboBox;class QTreeWidget;class QTableWidget;class QLabel;class QCheckBox;
class CircuitCanvas;class ModuleControls;
class SimulationInspector final:public QWidget {
    Q_OBJECT
public:
    using BoardLookup=std::function<const thinkercad::Esp8266Board*(const std::string&)>;
    SimulationInspector(CircuitCanvas* canvas,const thinkercad::CircuitState& state,const thinkercad::ModuleDevices& devices,const thinkercad::UartBus& uart,thinkercad::BusTraffic& traffic,BoardLookup board,QWidget* parent=nullptr);
    void sample(thinkercad::Microseconds time,const thinkercad::CircuitState& state);
    void refresh();
    void showPins(const QString &part);
    void reset();
    void shutdown();
    bool capturing()const;
protected:
    void showEvent(QShowEvent*)override;
    void hideEvent(QHideEvent*)override;
private:
    CircuitCanvas* canvas_;
    const thinkercad::CircuitState* state_;
    const thinkercad::ModuleDevices* devices_;
    const thinkercad::UartBus* uart_;
    thinkercad::BusTraffic* traffic_;
    BoardLookup board_;
    QTimer timer_;
    QComboBox *parts_,*modules_,*environment_,*busFilter_;
    QTreeWidget *pins_,*historyView_,*busView_,*uartView_;
    QTableWidget* registers_;
    QLabel *net_,*registerHint_,*trafficHint_;
    QLabel* actuatorTelemetry_;
    QCheckBox* capture_;
    QWidget* controls_;
    ModuleControls* environmentControls_=nullptr;
    thinkercad::Endpoint selected_;
    struct PinSample {thinkercad::Microseconds time;std::optional<double> voltage;thinkercad::Level level;};
    std::deque<PinSample> history_;
    bool historyDirty_=false;
    thinkercad::Microseconds now_=0;
    QByteArray configured_,netSignature_;
    QString language_;
    std::uint64_t trafficRevision_=~std::uint64_t(0);
    std::string registerPart_;
    std::optional<std::array<std::uint8_t,256>> lastRegisters_;
    void configure();
    void selectPin();
    void refreshPins();
    void refreshRegisters();
    void refreshTraffic();
    void refreshEnvironment();
};

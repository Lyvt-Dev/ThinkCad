#pragma once
#include "thinkercad/circuit.hpp"
#include <QWidget>
#include <deque>
class QComboBox;class QDoubleSpinBox;class QLabel;class QCheckBox;
struct InstrumentSample {thinkercad::Microseconds time;std::vector<std::optional<double>> channels;};
// Bounded acquisition history. Missing voltages remain missing, never fabricated zeroes.
class InstrumentPanel final:public QWidget {
    Q_OBJECT
public:
    explicit InstrumentPanel(QWidget* parent=nullptr);
    void configure(const thinkercad::CircuitDocument& document);
    void reset();
    void selectInstrument(const std::string& id);
    void sample(thinkercad::Microseconds time,const thinkercad::CircuitState& state);
    void refresh();
    const std::deque<InstrumentSample>& samples(const std::string& id)const;
    QByteArray csv()const;
protected:void paintEvent(QPaintEvent*)override;
    void mousePressEvent(QMouseEvent*)override;
private:
    struct Trace {thinkercad::CircuitPart part;std::deque<InstrumentSample> data;};
    std::map<std::string,Trace> traces_;
    QComboBox *selection_,*triggerMode_,*edge_,*decoder_;QDoubleSpinBox *triggerLevel_,*baud_;QCheckBox* math_;
    std::optional<thinkercad::Microseconds> triggerAt_;
    std::deque<InstrumentSample> captured_;
    std::optional<double> cursorA_,cursorB_;
    bool nextCursorB_=false;
    QDoubleSpinBox *timebase_,*scale_;QLabel* reading_;QCheckBox* freeze_;
    std::deque<InstrumentSample> frozen_;
    void selected();
    const Trace* trace()const;
    const std::deque<InstrumentSample>& visible()const;
};

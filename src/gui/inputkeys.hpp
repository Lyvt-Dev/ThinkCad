#pragma once
#include "thinkercad/circuit.hpp"
#include <QDialog>
#include <set>
class InputKeys final:public QDialog {
    Q_OBJECT
public:
    explicit InputKeys(const thinkercad::CircuitPart&,QWidget* parent=nullptr);
signals:void valueChanged(double value);void keyChanged(int key,bool pressed);void midiPacket(const QByteArray& bytes);void encoderTurn(int direction);void encoderPhase(int phase);void encoderSwitch(bool on);
protected:void closeEvent(QCloseEvent*)override;void changeEvent(QEvent*)override;
private:
    bool encoder_=false;
    std::set<int> held_;std::map<int,int> midiHeld_;void releaseAll();void setKey(int,bool);
};

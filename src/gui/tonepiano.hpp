#pragma once
#include "soundoutput.hpp"
#include <QDialog>
class TonePiano final:public QDialog {
    Q_OBJECT
public:
    explicit TonePiano(QWidget* parent=nullptr);
    ~TonePiano()override;
private:SoundOutput sound_;
};

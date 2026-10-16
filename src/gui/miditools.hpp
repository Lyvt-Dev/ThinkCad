#pragma once
#include "../../sdk/MidiParser.h"
#include <QWidget>
#include <QMap>
class QPlainTextEdit;class QSpinBox;class QComboBox;
class MidiTools final:public QWidget {
    Q_OBJECT
public:
    explicit MidiTools(QWidget* parent=nullptr);
    void receive(const QString& source,const QByteArray& data);
    static QByteArray translate(const thinkermidi::Message&,int channel,int transpose,bool noteToCc,int controller);
signals:void sendPacket(const QByteArray& bytes);
private:
    QMap<QString,thinkermidi::Parser> parsers_;thinkermidi::Message last_;
    QPlainTextEdit* monitor_;QSpinBox *channel_,*transpose_,*controller_;QComboBox* mode_;
};

#pragma once
#include <QWidget>
#include <QRectF>
#include <set>
class PianoKeyboard final:public QWidget {
    Q_OBJECT
public:
    explicit PianoKeyboard(int first=21,int last=108,QWidget* parent=nullptr);
    static double frequency(int midiNote);
    static QString noteName(int midiNote);
    QRectF keyRect(int note)const;
    int noteAt(QPointF point)const;
    int firstNote()const{return first_;}int lastNote()const{return last_;}
signals:
    void notePressed(int note);void noteReleased(int note);
protected:
    void paintEvent(QPaintEvent*)override;
    void mousePressEvent(QMouseEvent*)override;
    void mouseMoveEvent(QMouseEvent*)override;
    void mouseReleaseEvent(QMouseEvent*)override;
    void focusOutEvent(QFocusEvent*)override;
private:
    int first_,last_,held_=-1;
    void hold(int note);
};

#pragma once
#include <QWidget>
#include <QSerialPort>
#include <QElapsedTimer>
#include <deque>
class QPainter;class QComboBox;class QPlainTextEdit;class QLineEdit;class QPushButton;class QTabWidget;
class SerialTools final:public QWidget {
    Q_OBJECT
public:
    explicit SerialTools(QWidget* parent=nullptr);
    void boards(const QStringList& ids);
    void receive(const QString& source,const QByteArray& data,double seconds);
    int available(const QString& board)const;
    int read(const QString& board,bool peek=false);
    void showMidiView(const QString& source);
    void sendBytes(const QString& source,const QByteArray& bytes);
    void closePort();
    void refreshPorts();
    std::size_t plotSamples()const{return points_.size();}
    QString monitorText()const;
signals:void notice(const QString& text);
private:
    struct Point{double time;QMap<QString,double> values;};
    QSerialPort port_;QElapsedTimer clock_;QComboBox *source_,*ports_,*baud_,*ending_,*format_;QPlainTextEdit* monitor_;QLineEdit* input_;QPushButton* connect_;QWidget* plot_;QTabWidget* tabs_;
    QMap<QString,QByteArray> incoming_,pending_;std::deque<Point> points_;
    void send();
    void paintPlot(QPainter& painter,QRect rect);
    bool eventFilter(QObject* object,QEvent* event)override;
};

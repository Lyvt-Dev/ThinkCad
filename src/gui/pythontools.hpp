#pragma once
#include <QDialog>
#include <QProcess>
#include <QTemporaryDir>
#include <QStringConverter>
#include <memory>
class QPlainTextEdit;
class QLabel;
class QPushButton;
class PythonTools final:public QDialog {
    Q_OBJECT
public:
    explicit PythonTools(QWidget* parent=nullptr);
    ~PythonTools() override;
    bool openFile(const QString& path);
    void reject() override;
    bool running()const{return process_.state()!=QProcess::NotRunning;}
private:
    QProcess process_;
    QPlainTextEdit* source_;
    QPlainTextEdit* output_;
    QLabel* path_;
    QPushButton* run_;
    QPushButton* stop_;
    QString file_;
    qint64 outputBytes_=0;
    QStringDecoder decoder_{QStringDecoder::Utf8};
    std::unique_ptr<QTemporaryDir> temporary_;
    bool saveFile();
    bool confirmReplace();
    void run();
    void updateButtons();
    void append(const QByteArray& bytes);
};

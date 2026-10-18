#pragma once
#include <QDialog>
class Updater;
class QLabel;
class QProgressBar;
class QPushButton;
class QTextBrowser;
class UpdateDialog final : public QDialog {
    Q_OBJECT
  public:
    explicit UpdateDialog(Updater &, QWidget *parent = nullptr);

  private:
    Updater &updater_;
    qint64 received_ = 0, total_ = 0;
    QLabel *release_, *status_, *security_, *metrics_;
    QProgressBar *progress_;
    QTextBrowser *notes_;
    QPushButton *download_, *pause_, *cancel_, *install_, *skip_;
    void refresh();
};

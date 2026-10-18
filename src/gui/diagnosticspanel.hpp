#pragma once
#include "thinkercad/diagnostics.hpp"
#include <QWidget>
#include <QElapsedTimer>
class QComboBox;
class QListWidget;
class QLabel;
class QPushButton;
class QToolButton;
class DiagnosticsPanel final : public QWidget {
    Q_OBJECT
  public:
    explicit DiagnosticsPanel(QWidget *parent = nullptr);
    void showAssistant();
    void refresh(const thinkercad::CircuitDocument &, const thinkercad::NetTopology &,
                 const thinkercad::CircuitState &,
                 const std::map<std::string, const thinkercad::Esp8266Board *> &, const thinkercad::UartBus *,
                 const thinkercad::BusTraffic *);
  signals:
    void targetActivated(const QString &part, const QString &pin, const QString &wire);

  private:
    QListWidget *list_;
    QComboBox *filter_;
    QToolButton *assistantToggle_;
    QWidget *assistant_;
    QLabel *assistantText_;
    QPushButton *showTarget_, *nextHint_;
    QElapsedTimer elapsed_;
    std::string signature_;
    QString language_;
    std::vector<thinkercad::DiagnosticIssue> issues_;
    void render();
    void explainSelection();
};

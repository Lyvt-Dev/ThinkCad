#pragma once
#include "thinkercad/diagnostics.hpp"
#include <QWidget>
#include <QElapsedTimer>
#include <QSet>
#include <QMap>
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
    enum class Mode { Off, ErrorsOnly, Normal, Verbose };
    enum class Context { ProjectOpened, BoardAdded, PartAdded, Wired, SimulationStarted };
    static Mode savedMode();
    void reloadPreferences();
    void beginProject(const thinkercad::CircuitDocument &);
    void documentEdited(const thinkercad::CircuitDocument &);
    void offerHint(Context, const QString &part = {});
    void clearCompilationHints();
    void compilationHint(const QString &board, bool error);
    void refresh(const thinkercad::CircuitDocument &, const thinkercad::NetTopology &,
                 const thinkercad::CircuitState &,
                 const std::map<std::string, const thinkercad::Esp8266Board *> &, const thinkercad::UartBus *,
                 const thinkercad::BusTraffic *);
  signals:
    void targetActivated(const QString &part, const QString &pin, const QString &wire);
    void outputRequested();
    void inspectorRequested(const QString &part);
    void hintAvailable(bool available);

  private:
    QListWidget *list_;
    QComboBox *filter_;
    QComboBox *modeChoice_;
    QToolButton *assistantToggle_;
    QWidget *assistant_;
    QLabel *assistantText_;
    QPushButton *showTarget_, *nextHint_;
    QPushButton *inspect_;
    Mode mode_ = Mode::Normal;
    QSet<QString> seen_, dismissed_;
    QMap<QString, thinkercad::DiagnosticIssue> compilationIssues_;
    QSet<QString> parts_, wires_;
    QString currentKey_, contextKey_, contextPart_, inspectionPart_;
    Context context_ = Context::ProjectOpened;
    bool manual_ = false;
    QElapsedTimer elapsed_;
    std::string signature_;
    QString language_;
    std::vector<thinkercad::DiagnosticIssue> issues_, displayIssues_;
    void render();
    void explainSelection();
    QString contextText() const;
    QString issueKey(const thinkercad::DiagnosticIssue &) const;
    void updateVisibility();
};

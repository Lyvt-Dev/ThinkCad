#include "diagnosticspanel.hpp"
#include "language.hpp"
#include <QListWidget>
#include <QScrollBar>
#include <QComboBox>
#include <QVBoxLayout>
using namespace thinkercad;
DiagnosticsPanel::DiagnosticsPanel(QWidget *parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    filter_ = new QComboBox;
    filter_->setAccessibleName(Language::text("Diagnosefilter"));
    for (const auto &s : {"Alle Meldungen", "Warnungen und Fehler", "Nur Fehler"})
        filter_->addItem(Language::text(s));
    layout->addWidget(filter_);
    list_ = new QListWidget;
    list_->setObjectName("circuitDiagnostics");
    list_->setWordWrap(true);
    list_->setAccessibleName(Language::text("Diagnose"));
    layout->addWidget(list_);
    connect(filter_, qOverload<int>(&QComboBox::currentIndexChanged), this, [this] { render(); });
    connect(list_, &QListWidget::itemActivated, this, [this](QListWidgetItem *item) {
        emit targetActivated(item->data(Qt::UserRole).toString(), item->data(Qt::UserRole + 1).toString(),
                             item->data(Qt::UserRole + 2).toString());
    });
}
void DiagnosticsPanel::refresh(const CircuitDocument &doc, const NetTopology &net, const CircuitState &state,
                               const std::map<std::string, const Esp8266Board *> &boards, const UartBus *uart,
                               const BusTraffic *traffic) {
    const auto signature = NetTopology::signature(doc);
    if (elapsed_.isValid() && elapsed_.elapsed() < 250 && signature == signature_)
        return;
    signature_ = signature;
    elapsed_.restart();
    auto previous = std::move(issues_);
    issues_ = DiagnosticEngine::analyze(doc, net, state, boards, uart, traffic);
    for (const auto &message : state.errors) {
        DiagnosticIssue issue{"solver",        message, "Verdrahtung und Modellgrenzen prüfen.",
                              Severity::Error, {},      {}};
        for (const auto &p : doc.parts())
            if (message.find(p.id) != std::string::npos) {
                issue.target.part = p.id;
                break;
            }
        issues_.push_back(std::move(issue));
    }
    for (const auto &warning : state.warnings)
        issues_.push_back({"solver-warning",
                           warning.message,
                           "Verdrahtung und Modellgrenzen prüfen.",
                           Severity::Warning,
                           {warning.part, {}},
                           {}});
    if (issues_ != previous || language_ != Language::current()) {
        language_ = Language::current();
        render();
    }
}
void DiagnosticsPanel::render() {
    const int scroll = list_->verticalScrollBar()->value();
    list_->clear();
    for (const auto &issue : issues_) {
        if (filter_->currentIndex() == 2 && issue.severity != Severity::Error)
            continue;
        if (filter_->currentIndex() == 1 && issue.severity == Severity::Info)
            continue;
        auto level = Language::text(issue.severity == Severity::Error     ? "Fehler"
                                    : issue.severity == Severity::Warning ? "Warnung"
                                                                          : "Hinweis");
        auto text = level + " · " + QString::fromStdString(issue.target.part) +
                    (issue.target.pin.empty() ? QString() : "." + QString::fromStdString(issue.target.pin)) +
                    "\n" + Language::text(QString::fromStdString(issue.explanation)) + "\n" +
                    Language::text("Lösung") + ": " + Language::text(QString::fromStdString(issue.solution));
        auto *item = new QListWidgetItem(text, list_);
        item->setData(Qt::UserRole, QString::fromStdString(issue.target.part));
        item->setData(Qt::UserRole + 1, QString::fromStdString(issue.target.pin));
        item->setData(Qt::UserRole + 2, QString::fromStdString(issue.wire));
        item->setToolTip(text);
    }
    if (list_->count() == 0)
        list_->addItem(Language::text("Keine elektrischen Fehler erkannt. Modellgrenzen beachten."));
    list_->verticalScrollBar()->setValue(scroll);
}

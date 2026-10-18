#include "diagnosticspanel.hpp"
#include "language.hpp"
#include <QListWidget>
#include <QScrollBar>
#include <QComboBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QToolButton>
#include <QSettings>
#include <QPixmap>
static void initializeAssistantResources() {
    Q_INIT_RESOURCE(thinkcad_assistant);
}
using namespace thinkercad;
DiagnosticsPanel::DiagnosticsPanel(QWidget *parent) : QWidget(parent) {
    initializeAssistantResources();
    auto *layout = new QVBoxLayout(this);
    assistantToggle_ = new QToolButton;
    assistantToggle_->setObjectName("duckAssistantToggle");
    assistantToggle_->setCheckable(true);
    assistantToggle_->setChecked(QSettings().value("duckAssistantVisible", true).toBool());
    layout->addWidget(assistantToggle_);
    assistant_ = new QWidget;
    assistant_->setObjectName("duckAssistant");
    auto *assistantLayout = new QHBoxLayout(assistant_);
    assistantLayout->setContentsMargins(0, 4, 0, 8);
    auto *duck = new QLabel;
    duck->setObjectName("duckAssistantMascot");
    duck->setPixmap(
        QPixmap(":/assistant/duck.png").scaled(80, 80, Qt::KeepAspectRatio, Qt::FastTransformation));
    duck->setFixedSize(88, 88);
    duck->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
    assistantLayout->addWidget(duck, 0, Qt::AlignTop);
    auto *words = new QVBoxLayout;
    assistantText_ = new QLabel;
    assistantText_->setObjectName("duckAssistantExplanation");
    assistantText_->setTextFormat(Qt::PlainText);
    assistantText_->setWordWrap(true);
    assistantText_->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    words->addWidget(assistantText_);
    auto *buttons = new QHBoxLayout;
    showTarget_ = new QPushButton;
    showTarget_->setObjectName("duckShowTarget");
    nextHint_ = new QPushButton;
    nextHint_->setObjectName("duckNextHint");
    buttons->addWidget(showTarget_);
    buttons->addWidget(nextHint_);
    buttons->addStretch();
    words->addLayout(buttons);
    assistantLayout->addLayout(words, 1);
    layout->addWidget(assistant_);
    assistant_->setVisible(assistantToggle_->isChecked());
    connect(assistantToggle_, &QToolButton::toggled, this, [this](bool visible) {
        assistant_->setVisible(visible);
        QSettings().setValue("duckAssistantVisible", visible);
    });
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
    connect(list_, &QListWidget::currentRowChanged, this, [this] { explainSelection(); });
    connect(showTarget_, &QPushButton::clicked, this, [this] {
        if (const auto *item = list_->currentItem(); item && showTarget_->isEnabled())
            emit targetActivated(item->data(Qt::UserRole).toString(), item->data(Qt::UserRole + 1).toString(),
                                 item->data(Qt::UserRole + 2).toString());
    });
    connect(nextHint_, &QPushButton::clicked, this, [this] {
        if (list_->count() > 1)
            list_->setCurrentRow((list_->currentRow() + 1) % list_->count());
    });
    connect(filter_, qOverload<int>(&QComboBox::currentIndexChanged), this, [this] { render(); });
    connect(list_, &QListWidget::itemActivated, this, [this](QListWidgetItem *item) {
        emit targetActivated(item->data(Qt::UserRole).toString(), item->data(Qt::UserRole + 1).toString(),
                             item->data(Qt::UserRole + 2).toString());
    });
    render();
}
void DiagnosticsPanel::showAssistant() {
    assistantToggle_->setChecked(true);
    assistantToggle_->setFocus();
}
void DiagnosticsPanel::explainSelection() {
    showTarget_->setEnabled(false);
    nextHint_->setEnabled(list_->count() > 1);
    const auto *item = list_->currentItem();
    bool valid = false;
    const int index = item ? item->data(Qt::UserRole + 3).toInt(&valid) : -1;
    if (!valid || index < 0 || index >= static_cast<int>(issues_.size())) {
        assistantText_->setText(Language::text("Quak! In dieser Ansicht gibt es keine Diagnosemeldungen. Das "
                                               "ist keine Garantie für echte Hardware."));
        return;
    }
    const auto &issue = issues_[index];
    const auto target =
        QString::fromStdString(issue.target.part) +
        (issue.target.pin.empty() ? QString() : "." + QString::fromStdString(issue.target.pin));
    assistantText_->setText(
        Language::text("Quak! Schauen wir uns das an:") + (target.isEmpty() ? QString() : " " + target) +
        "\n" + Language::text(QString::fromStdString(issue.explanation)) + "\n" + Language::text("Lösung") +
        ": " + Language::text(QString::fromStdString(issue.solution)));
    showTarget_->setEnabled(!issue.target.part.empty() || !issue.wire.empty());
}
void DiagnosticsPanel::refresh(const CircuitDocument &doc, const NetTopology &net, const CircuitState &state,
                               const std::map<std::string, const Esp8266Board *> &boards, const UartBus *uart,
                               const BusTraffic *traffic) {
    const auto signature = NetTopology::signature(doc);
    if (elapsed_.isValid() && elapsed_.elapsed() < 250 && signature == signature_ &&
        language_ == Language::current())
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
    assistantToggle_->setText(Language::text("Quak-Assistent"));
    findChild<QLabel *>("duckAssistantMascot")->setAccessibleName(Language::text("Diagnose-Ente"));
    showTarget_->setText(Language::text("Stelle zeigen"));
    nextHint_->setText(Language::text("Nächster Hinweis"));
    for (int i = 0; i < 3; ++i)
        filter_->setItemText(i, Language::text(i == 0   ? "Alle Meldungen"
                                               : i == 1 ? "Warnungen und Fehler"
                                                        : "Nur Fehler"));
    const int scroll = list_->verticalScrollBar()->value();
    list_->clear();
    for (std::size_t index = 0; index < issues_.size(); ++index) {
        const auto &issue = issues_[index];
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
        item->setData(Qt::UserRole + 3, static_cast<int>(index));
        item->setToolTip(text);
    }
    if (list_->count() == 0)
        list_->addItem(Language::text("Keine elektrischen Fehler erkannt. Modellgrenzen beachten."));
    list_->setCurrentRow(0);
    explainSelection();
    list_->verticalScrollBar()->setValue(scroll);
}

#include "diagnosticspanel.hpp"
#include "language.hpp"
#include "assistantskin.hpp"
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
#include <QSignalBlocker>
#include <algorithm>
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
    assistantToggle_->setChecked(true);
    auto *controls = new QHBoxLayout;
    controls->addWidget(assistantToggle_);
    modeChoice_ = new QComboBox;
    modeChoice_->setObjectName("duckAssistantMode");
    for (const auto *label : {"Aus", "Nur Fehler", "Normal", "Ausführlich"})
        modeChoice_->addItem(Language::text(label));
    controls->addWidget(modeChoice_);
    controls->addStretch();
    layout->addLayout(controls);
    assistant_ = new QWidget;
    assistant_->setObjectName("duckAssistant");
    auto *assistantLayout = new QHBoxLayout(assistant_);
    assistantLayout->setContentsMargins(0, 4, 0, 8);
    auto *duck = new QLabel;
    duck->setObjectName("duckAssistantMascot");
    duck->setPixmap(
        QPixmap(":/assistant/duck.png").scaled(32, 32, Qt::KeepAspectRatio, Qt::FastTransformation));
    duck->setFixedSize(36, 36);
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
    inspect_ = new QPushButton;
    inspect_->setObjectName("duckInspectPins");
    buttons->addWidget(inspect_);
    buttons->addStretch();
    words->addLayout(buttons);
    assistantLayout->addLayout(words, 1);
    layout->addWidget(assistant_);
    connect(assistantToggle_, &QToolButton::toggled, this, [this](bool visible) {
        if (!visible && !currentKey_.isEmpty()) dismissed_.insert(currentKey_);
        if (visible) { dismissed_.remove(currentKey_); manual_ = true; }
        updateVisibility();
    });
    connect(modeChoice_, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int index) {
        QSettings().setValue("duckAssistantMode", index);
        reloadPreferences();
    });
    connect(inspect_, &QPushButton::clicked, this, [this] { emit inspectorRequested(inspectionPart_); });
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
        if (!showTarget_->isEnabled()) return;
        if (currentKey_.startsWith("compiler/")) { emit outputRequested(); return; }
        if (const auto *item = list_->currentItem())
            emit targetActivated(item->data(Qt::UserRole).toString(), item->data(Qt::UserRole + 1).toString(),
                                 item->data(Qt::UserRole + 2).toString());
    });
    connect(nextHint_, &QPushButton::clicked, this, [this] {
        if (list_->count() > 1) list_->setCurrentRow((list_->currentRow() + 1) % list_->count());
    });
    connect(filter_, qOverload<int>(&QComboBox::currentIndexChanged), this, [this] { render(); });
    connect(list_, &QListWidget::itemActivated, this, [this](QListWidgetItem *item) {
        emit targetActivated(item->data(Qt::UserRole).toString(), item->data(Qt::UserRole + 1).toString(),
                             item->data(Qt::UserRole + 2).toString());
    });
    reloadPreferences();
}
DiagnosticsPanel::Mode DiagnosticsPanel::savedMode() {
    QSettings settings;
    const int legacy = settings.value("duckAssistantVisible", true).toBool() ? 2 : 0;
    return static_cast<Mode>(std::clamp(settings.value("duckAssistantMode", legacy).toInt(), 0, 3));
}
void DiagnosticsPanel::reloadPreferences() {
    mode_ = savedMode();
    const auto skinId = AssistantSkins::savedId();
    auto *mascot = findChild<QLabel *>("duckAssistantMascot");
    const auto ratio = mascot->devicePixelRatioF();
    auto image = AssistantSkins::pixmap(skinId).scaled(QSize(32, 32) * ratio,
                                                    Qt::KeepAspectRatio, Qt::FastTransformation);
    image.setDevicePixelRatio(ratio);
    mascot->setPixmap(image);
    mascot->setAccessibleName(QString::fromLatin1(AssistantSkins::find(skinId).name));
    mascot->setProperty("assistantSkin", skinId);
    { QSignalBlocker block(modeChoice_); modeChoice_->setCurrentIndex(static_cast<int>(mode_)); }
    render();
}
void DiagnosticsPanel::showAssistant() {
    manual_ = true;
    dismissed_.remove(currentKey_);
    explainSelection(); // An explicit request never changes the saved mode or grabs focus.
}
QString DiagnosticsPanel::issueKey(const DiagnosticIssue &issue) const {
    return QString::fromStdString(issue.rule + "/" + issue.target.part + "/" + issue.target.pin + "/" +
                                issue.wire + "/" + std::to_string(int(issue.severity)) + "/" + issue.explanation);
}
void DiagnosticsPanel::updateVisibility() {
    const bool visible = mode_ != Mode::Off && !currentKey_.isEmpty() && !dismissed_.contains(currentKey_);
    { QSignalBlocker block(assistantToggle_); assistantToggle_->setChecked(visible); }
    assistantToggle_->setText(Language::text(visible ? "Hinweis ausblenden" : "Hinweis anzeigen"));
    assistant_->setVisible(visible);
    emit hintAvailable(visible);
}
void DiagnosticsPanel::beginProject(const CircuitDocument &doc) {
    seen_.clear(); dismissed_.clear(); compilationIssues_.clear(); issues_.clear();
    contextKey_.clear(); currentKey_.clear(); manual_ = false;
    parts_.clear(); wires_.clear();
    for (const auto &p : doc.parts()) parts_.insert(QString::fromStdString(p.id));
    for (const auto &w : doc.wires()) wires_.insert(QString::fromStdString(w.id));
    elapsed_.invalidate(); signature_.clear();
    offerHint(Context::ProjectOpened);
    render();
}
void DiagnosticsPanel::documentEdited(const CircuitDocument &doc) {
    QSet<QString> parts, wires;
    for (const auto &p : doc.parts()) {
        const auto id = QString::fromStdString(p.id); parts.insert(id);
        if (!parts_.contains(id) && p.kind != PartKind::Note && p.kind != PartKind::Junction && p.kind != PartKind::NetLabel)
            offerHint(p.kind == PartKind::Esp8266 ? Context::BoardAdded : Context::PartAdded, id,
                      p.kind==PartKind::Module?QString::fromStdString(p.model):QString());
    }
    for (const auto &w : doc.wires()) {
        const auto id = QString::fromStdString(w.id); wires.insert(id);
        if (!wires_.contains(id)) {
            const auto& a=doc.part(w.a.part);const auto& b=doc.part(w.b.part);
            const auto& target=b.kind==PartKind::Esp8266?a:b;
            offerHint(Context::Wired,QString::fromStdString(target.id),target.kind==PartKind::Module?QString::fromStdString(target.model):QString());
        }
    }
    parts_ = std::move(parts); wires_ = std::move(wires);
    if (!contextPart_.isEmpty() && !parts_.contains(contextPart_)) { contextKey_.clear(); explainSelection(); }
}
void DiagnosticsPanel::offerHint(Context context, const QString &part,const QString& model) {
    const auto key = QString("context/%1/%2/%3").arg(int(context)).arg(part,model);
    if (seen_.contains(key)) return;
    seen_.insert(key);
    context_ = context; contextPart_ = part; contextKey_ = key;
    contextModel_=model;
    manual_ = false;
    explainSelection();
}
QString DiagnosticsPanel::contextText() const {
    if(context_==Context::PartAdded||context_==Context::Wired){
        if(contextModel_=="relay-5v")return Language::text("Dieses Relais benötigt eine Versorgung und einen Steuerpin. Prüfe außerdem COM, NO und NC für deinen Lastkreis.");
        if(contextModel_=="a4988")return Language::text("Prüfe GND, VMOT und VDD des A4988. STEP und DIR steuern den Motor; RESET und SLEEP müssen freigegeben sein.");
        if(contextModel_=="uln2003"||(contextModel_=="stepper-4"||contextModel_=="stepper-5"||contextModel_=="stepper-6"))return Language::text("Verbinde die Wicklungen mit dem passenden Treiber. Die Anfangsausrichtung ist der Bezugspunkt; Last und Resonanzen werden nicht simuliert.");
        if(contextModel_=="ili9341"||contextModel_=="st7735")return Language::text("Dieses TFT benötigt Versorgung, CS, DC, RESET und SPI-Leitungen. Versorge auch die LED-Hintergrundbeleuchtung und rufe die Initialisierung im Sketch auf.");
        if(contextModel_=="bme680")return Language::text("Die BME680-Messwerte sind simuliert. Prüfe Versorgung, SDA/SCL und Adresse; eine chemische Gasanalyse wird nicht modelliert.");
        if(contextModel_=="mh-z19b")return Language::text("Verbinde TX mit RX und RX mit TX. Der MH-Z19B benötigt 5-V-Versorgung und 3,3-V-UART-Signale; CO₂-Werte und Fehler stellst du im Inspektor ein.");
    }
    switch (context_) {
    case Context::ProjectOpened: return Language::text("Wähle ein Board, um seinen Sketch zu bearbeiten. Im Pin-Inspektor kannst du die Verbindungen prüfen.");
    case Context::BoardAdded: return Language::text("Das Board ist eingefügt. Wähle es aus und prüfe Sketch und Pinbelegung.");
    case Context::PartAdded: return Language::text("Das Bauteil ist eingefügt. Prüfe seine Anschlüsse und die benötigte Versorgung im Pin-Inspektor.");
    case Context::Wired: return Language::text("Die Verbindung ist angelegt. Prüfe im Pin-Inspektor, welche Anschlüsse zum selben Netz gehören.");
    case Context::SimulationStarted:
        return mode_ == Mode::Verbose
            ? Language::text("Die Simulation wird gestartet. Die Modelle bilden nicht alle Eigenschaften echter Hardware ab.")
            : Language::text("Die Simulation wird vorbereitet. In der Ausgabe siehst du Meldungen des Sketches.");
    case Context::ShareUnencrypted:return Language::text("Der Share-Code enthält deine Schaltung und deinen Code. Ohne Passwort ist er nicht vertraulich.");
    }
    return {};
}
void DiagnosticsPanel::clearCompilationHints() { compilationIssues_.clear(); render(); }
void DiagnosticsPanel::compilationHint(const QString &board, bool error) {
    auto found = compilationIssues_.find(board);
    if (found != compilationIssues_.end() && found->severity == Severity::Error && !error) return;
    DiagnosticIssue issue{"compiler", error ? "Der Sketch konnte nicht kompiliert werden." : "Der Compiler hat eine Warnung gemeldet.",
        "Öffne die Ausgabe und prüfe die erste Meldung mit Dateiname und Zeile.",
        error ? Severity::Error : Severity::Warning, {board.toStdString(), {}}, {}};
    if (found != compilationIssues_.end() && found.value() == issue) return;
    compilationIssues_[board] = std::move(issue);
    render();
}
void DiagnosticsPanel::explainSelection() {
    showTarget_->setEnabled(false);
    showTarget_->setText(Language::text("Stelle zeigen"));
    inspect_->setVisible(false);
    nextHint_->setEnabled(list_->count() > 1);
    const auto *item = list_->currentItem();
    bool valid = false;
    const int index = item ? item->data(Qt::UserRole + 3).toInt(&valid) : -1;
    if (valid && index >= 0 && index < static_cast<int>(displayIssues_.size()) &&
        (mode_ != Mode::ErrorsOnly || displayIssues_[index].severity != Severity::Info)) {
        const auto &issue = displayIssues_[index];
        currentKey_ = issueKey(issue);
        inspectionPart_ = QString::fromStdString(issue.target.part);
        const auto target = QString::fromStdString(issue.target.part) +
            (issue.target.pin.empty() ? QString() : "." + QString::fromStdString(issue.target.pin));
        auto text = Language::text(QString::fromStdString(issue.explanation)) + "\n" +
            Language::text("Nächster Schritt") + ": " + Language::text(QString::fromStdString(issue.solution));
        if (!target.isEmpty()) text.prepend(target + "\n");
        assistantText_->setText(text);
        if (issue.rule == "compiler") showTarget_->setText(Language::text("Ausgabe öffnen"));
        showTarget_->setEnabled(issue.rule == "compiler" || !issue.target.part.empty() || !issue.wire.empty());
        inspect_->setVisible(issue.rule != "compiler");
    } else {
        const bool contextual = mode_ >= Mode::Normal && !contextKey_.isEmpty();
        currentKey_ = contextual ? contextKey_ : (manual_ && mode_ >= Mode::Normal ? "manual" : QString());
        inspectionPart_ = contextPart_;
        assistantText_->setText(contextual ? contextText() : Language::text(
            "In dieser Ansicht gibt es keine Diagnosemeldungen. Das ist keine Garantie für echte Hardware."));
        inspect_->setVisible(contextual);
    }
    updateVisibility();
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
        DiagnosticIssue issue{"solver",        message, "Öffne den Pin-Inspektor und prüfe Versorgung, Masse und Netzstatus.",
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
                           "Öffne den Pin-Inspektor und prüfe Versorgung, Masse und Netzstatus.",
                           Severity::Warning,
                           {warning.part, {}},
                           {}});
    if (issues_ != previous || language_ != Language::current()) {
        language_ = Language::current();
        render();
    }
}
void DiagnosticsPanel::render() {
    assistantToggle_->setText(Language::text("Hinweis ausblenden"));
    modeChoice_->setAccessibleName(Language::text("Quack-Assistent"));
    const char *modes[] = {"Aus", "Nur Fehler", "Normal", "Ausführlich"};
    for (int i = 0; i < 4; ++i) modeChoice_->setItemText(i, Language::text(modes[i]));
    inspect_->setText(Language::text("Pin-Inspektor öffnen"));
    findChild<QLabel *>("duckAssistantMascot")->setAccessibleName(
        QString::fromLatin1(AssistantSkins::find(AssistantSkins::savedId()).name));
    showTarget_->setText(Language::text("Stelle zeigen"));
    nextHint_->setText(Language::text("Nächster Hinweis"));
    for (int i = 0; i < 3; ++i)
        filter_->setItemText(i, Language::text(i == 0   ? "Alle Meldungen"
                                               : i == 1 ? "Warnungen und Fehler"
                                                        : "Nur Fehler"));
    const int scroll = list_->verticalScrollBar()->value();
    const auto selectedKey = currentKey_;
    QSignalBlocker block(list_);
    list_->clear();
    displayIssues_ = issues_;
    for (const auto &issue : compilationIssues_) displayIssues_.push_back(issue);
    std::stable_sort(displayIssues_.begin(), displayIssues_.end(), [](const auto &a, const auto &b) {
        return a.severity > b.severity;
    });
    QSet<QString> active;
    for (const auto &issue : displayIssues_) active.insert(issueKey(issue));
    for (auto it = dismissed_.begin(); it != dismissed_.end();) {
        if (!it->startsWith("context/") && *it != "manual" && !active.contains(*it)) it = dismissed_.erase(it);
        else ++it;
    }
    int selection = 0;
    for (std::size_t index = 0; index < displayIssues_.size(); ++index) {
        const auto &issue = displayIssues_[index];
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
        if (issueKey(issue) == selectedKey && issue.severity == displayIssues_.front().severity)
            selection = list_->row(item);
    }
    if (list_->count() == 0)
        list_->addItem(Language::text("Keine elektrischen Fehler erkannt. Modellgrenzen beachten."));
    list_->setCurrentRow(selection);
    explainSelection();
    list_->verticalScrollBar()->setValue(scroll);
}

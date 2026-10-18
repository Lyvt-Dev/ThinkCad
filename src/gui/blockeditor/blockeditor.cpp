#include "blockeditor.hpp"
#include "blockscene.hpp"
#include <QGraphicsView>
#include <QTabWidget>
#include <QJsonArray>
#include <QTimer>
#include <QDialog>
#include <QDialogButtonBox>
#include <QTableWidget>
#include <QScrollArea>
#include <QHeaderView>
#include <QDropEvent>
#include "thinkercad/visual/expressionparser.hpp"
#include "language.hpp"
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QPainter>
#include <QComboBox>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QSignalBlocker>
#include <QJsonArray>
#include <QJsonDocument>
#include <QUuid>
#include <functional>
#include <algorithm>
using namespace thinkercad::visual;
namespace {
const char *kinds[] = {"Variable",     "Zuweisung", "Wenn / Sonst", "Solange",
                       "Zählschleife", "Aufruf",    "Rückgabe"};
const char *types[] = {"Ganzzahl", "Zahl", "Wahrheitswert", "Text", "Ohne Rückgabewert"};
QString q(const std::string &value) {
    return QString::fromStdString(value);
}
QString expressionText(const Expression &e) {
    switch (e.kind) {
    case Expression::Kind::Text: {
        auto v = q(e.text);
        v.replace('\\', "\\\\")
            .replace('"', "\\\"")
            .replace('\n', "\\n")
            .replace('\r', "\\r")
            .replace('\t', "\\t");
        return '"' + v + '"';
    }
    case Expression::Kind::Binary:
        return "(" + expressionText(e.arguments.at(0)) + " " + q(e.text) + " " +
               expressionText(e.arguments.at(1)) + ")";
    case Expression::Kind::Call: {
        QStringList args;
        for (const auto &a : e.arguments)
            args << expressionText(a);
        return q(e.text) + "(" + args.join(", ") + ")";
    }
    default:
        return q(e.text);
    }
}
QString uid() {
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}
std::optional<std::vector<Variable>> editVariables(QWidget *parent, const QString &title,
                                                   const std::vector<Variable> &variables, bool parameters) {
    QDialog dialog(parent);
    dialog.setWindowTitle(title);
    dialog.resize(620, 360);
    QVBoxLayout layout(&dialog);
    QTableWidget table(0, parameters ? 2 : 3);
    QStringList headers{Language::text("Name"), Language::text("Datentyp")};
    if (!parameters)
        headers << Language::text("Anfangswert");
    table.setHorizontalHeaderLabels(headers);
    table.horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table.setAccessibleName(title);
    layout.addWidget(&table);
    auto append = [&](const Variable &variable) {
        int row = table.rowCount();
        table.insertRow(row);
        table.setItem(row, 0, new QTableWidgetItem(q(variable.name)));
        auto *type = new QComboBox;
        for (int i = 0; i < 4; ++i)
            type->addItem(Language::text(types[i]));
        type->setCurrentIndex(int(variable.type));
        table.setCellWidget(row, 1, type);
        if (!parameters)
            table.setItem(row, 2, new QTableWidgetItem(expressionText(variable.initial)));
    };
    for (const auto &variable : variables)
        append(variable);
    QHBoxLayout actions;
    QPushButton add(Language::text("Hinzufügen")), remove(Language::text("Entfernen"));
    actions.addWidget(&add);
    actions.addWidget(&remove);
    layout.addLayout(&actions);
    QObject::connect(&add, &QPushButton::clicked, &dialog, [&] {
        append({"value" + std::to_string(table.rowCount() + 1), ValueType::Integer, {}});
    });
    QObject::connect(&remove, &QPushButton::clicked, &dialog, [&] {
        if (table.currentRow() >= 0)
            table.removeRow(table.currentRow());
    });
    QDialogButtonBox buttons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout.addWidget(&buttons);
    QObject::connect(&buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    while (dialog.exec() == QDialog::Accepted) {
        try {
            std::vector<Variable> result;
            for (int row = 0; row < table.rowCount(); ++row) {
                auto type = ValueType(qobject_cast<QComboBox *>(table.cellWidget(row, 1))->currentIndex());
                auto value = parameters ? Expression{}
                                        : ExpressionParser::parse(table.item(row, 2)->text().toStdString());
                result.push_back({table.item(row, 0)->text().trimmed().toStdString(), type, value});
            }
            return result;
        } catch (const std::exception &e) {
            QMessageBox::warning(&dialog, title, Language::text(QString::fromUtf8(e.what())));
        }
    }
    return std::nullopt;
}
QString auxiliaryFingerprint(const SketchProject &sketch) {
    QJsonObject sources;
    for (auto it = sketch.files.begin(); it != sketch.files.end(); ++it)
        if (it.key() != sketch.entryPoint() && !it.key().endsWith(".py", Qt::CaseInsensitive))
            sources[it.key()] = it.value();
    return BlockPersistence::fingerprint(
        QString::fromUtf8(QJsonDocument(sources).toJson(QJsonDocument::Compact)));
}
class BlockTree : public QTreeWidget {
  public:
    using QTreeWidget::QTreeWidget;
    std::function<void()> dropped;

  protected:
    void dropEvent(QDropEvent *e) override {
        QTreeWidget::dropEvent(e);
        if (e->isAccepted() && dropped)
            QTimer::singleShot(0, this, dropped);
    }
    bool dropMimeData(QTreeWidgetItem *parent, int index, const QMimeData *data,
                      Qt::DropAction action) override {
        if (!parent || parent->data(0, Qt::UserRole).toString() != "container")
            return false;
        for (auto *item : selectedItems())
            if (item->data(0, Qt::UserRole).toString() != "block")
                return false;
        return QTreeWidget::dropMimeData(parent, index, data, action);
    }
};
} // namespace
BlockEditor::BlockEditor(QWidget *parent) : QWidget(parent) {
    setObjectName("blockEditor");
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    mode_ = new QComboBox;
    mode_->setObjectName("codingMode");
    for (auto s : {"Blöcke", "Blöcke + C++", "C++"})
        mode_->addItem(Language::text(s));
    mode_->setAccessibleName(Language::text("Programmieransicht"));
    layout->addWidget(mode_);
    status_ = new QLabel;
    status_->setWordWrap(true);
    status_->setTextFormat(Qt::PlainText);
    status_->setObjectName("blockSyncStatus");
    layout->addWidget(status_);
    body_ = new QWidget;
    auto *body = new QVBoxLayout(body_);
    body->setContentsMargins(0, 0, 0, 0);
    tree_ = new BlockTree;
    tree_->setObjectName("blockTree");
    tree_->setHeaderHidden(true);
    tree_->setDragDropMode(QAbstractItemView::InternalMove);
    tree_->setDefaultDropAction(Qt::MoveAction);
    tree_->setAccessibleName(Language::text("Programmblöcke"));
    auto *pages = new QTabWidget;
    scene_ = new BlockScene(this);
    view_ = new QGraphicsView(scene_);
    view_->setObjectName("blockWorkspace");
    view_->setAccessibleName(Language::text("Visuelles Blockprogramm"));
    view_->setRenderHint(QPainter::Antialiasing);
    pages->addTab(view_, Language::text("Blöcke"));
    pages->addTab(tree_, Language::text("Struktur"));
    body->addWidget(pages, 1);
    auto *hint = new QLabel(Language::text("Reihenfolge und Verschachtelung in Struktur ändern. "
                                           "Kartenpositionen ändern nicht den Programmablauf."));
    hint->setWordWrap(true);
    body->addWidget(hint);
    connect(scene_, &BlockScene::blockSelected, this, [this](const QString &id) {
        if (id == "setup" || id == "loop") {
            tree_->setCurrentItem(tree_->topLevelItem(id == "setup" ? 0 : 1));
            return;
        }
        QTreeWidgetItemIterator it(tree_);
        while (*it) {
            if ((*it)->data(0, Qt::UserRole + 1).toJsonObject()["id"].toString() == id) {
                tree_->setCurrentItem(*it);
                return;
            }
            ++it;
        }
    });
    connect(scene_, &BlockScene::blockMoved, this, [this](const QString &id, QPointF position) {
        auto layout = metadata_["layout"].toObject();
        layout[id] = QJsonArray{position.x(), position.y()};
        metadata_["layout"] = layout;
        emit metadataChanged();
    });
    static_cast<BlockTree *>(tree_)->dropped = [this] {
        if (loading_)
            return;
        try {
            program_ = collect();
            metadata_["program"] = BlockPersistence::encode(program_);
            scene_->setProgram(program_, metadata_["layout"].toObject());
            emit metadataChanged();
            setStatus();
        } catch (const std::exception &e) {
            status_->setText(Language::text(QString::fromUtf8(e.what())));
        }
    };
    auto *row = new QHBoxLayout;
    add_ = new QPushButton(Language::text("Block hinzufügen"));
    remove_ = new QPushButton(Language::text("Block löschen"));
    row->addWidget(add_);
    row->addWidget(remove_);
    body->addLayout(row);
    auto *form = new QFormLayout;
    properties_ = form;
    kind_ = new QComboBox;
    kind_->setObjectName("blockKind");
    for (auto s : kinds)
        kind_->addItem(Language::text(s));
    form->addRow(Language::text("Blocktyp"), kind_);
    name_ = new QLineEdit;
    name_->setObjectName("blockName");
    name_->setMaxLength(64);
    form->addRow(Language::text("Name"), name_);
    command_ = new QComboBox;
    command_->setObjectName("blockCommand");
    for (const auto &c : ArduinoCodeGenerator::commands())
        command_->addItem(q(c.name));
    command_->setEditable(true);
    form->addRow(Language::text("Funktion / Befehl"), command_);
    type_ = new QComboBox;
    type_->setObjectName("blockValueType");
    for (auto s : types)
        type_->addItem(Language::text(s));
    form->addRow(Language::text("Datentyp"), type_);
    parameters_ = new QLineEdit;
    parameters_->setReadOnly(true);
    parameters_->setPlaceholderText(Language::text("Funktionsparameter"));
    form->addRow(Language::text("Funktionsparameter"), parameters_);
    auto *parameterButton = new QPushButton(Language::text("Parameter bearbeiten…"));
    parametersButton_ = parameterButton;
    form->addRow(parameterButton);
    connect(parameterButton, &QPushButton::clicked, this, [this] {
        auto *item = tree_->currentItem();
        if (locked_ || !item || item->data(0, Qt::UserRole).toString() != "function")
            return;
        auto json = BlockPersistence::encode(BlockProgram{});
        json["functions"] = QJsonArray{item->data(0, Qt::UserRole + 1).toJsonObject()};
        auto function = BlockPersistence::decode(json).functions[0];
        if (auto edited =
                editVariables(this, Language::text("Funktionsparameter"), function.parameters, true)) {
            function.parameters = *edited;
            BlockProgram p;
            p.functions = {function};
            item->setData(0, Qt::UserRole + 1,
                          BlockPersistence::encode(p)["functions"].toArray()[0].toObject());
            program_ = collect();
            metadata_["program"] = BlockPersistence::encode(program_);
            emit metadataChanged();
            select();
            setStatus();
        }
    });
    arguments_ = new QPlainTextEdit;
    arguments_->setMaximumHeight(100);
    arguments_->setPlaceholderText(Language::text("Ein Ausdruck pro Zeile, z. B. digitalRead(5) == HIGH"));
    arguments_->setObjectName("blockArguments");
    form->addRow(Language::text("Argumente / Ausdrücke"), arguments_);
    body->addLayout(form);
    auto *globals = new QPushButton(Language::text("Globale Variablen…"));
    body->addWidget(globals);
    connect(globals, &QPushButton::clicked, this, [this] {
        if (locked_ || !valid_)
            return;
        if (auto edited =
                editVariables(this, Language::text("Globale Variablen…"), program_.globals, false)) {
            program_ = collect();
            program_.globals = *edited;
            metadata_["program"] = BlockPersistence::encode(program_);
            emit metadataChanged();
            rebuild();
            setStatus();
        }
    });
    auto *functions = new QPushButton(Language::text("Funktion hinzufügen"));
    body->addWidget(functions);
    auto *actions = new QHBoxLayout;
    apply_ = new QPushButton(Language::text("Block übernehmen"));
    apply_->setObjectName("applyBlock");
    reset_ = new QPushButton(Language::text("C++ aus Blöcken neu erzeugen…"));
    reset_->setObjectName("regenerateBlocks");
    actions->addWidget(apply_);
    actions->addWidget(reset_);
    auto *restore = new QPushButton(Language::text("Manuelle Sicherung wiederherstellen"));
    restore->setObjectName("restoreManualSketch");
    body->addWidget(restore);
    connect(restore, &QPushButton::clicked, this, [this] {
        if (locked_ || !metadata_["backup"].isString())
            return;
        auto backup = metadata_["backup"].toString();
        metadata_["backup"] = source_;
        source_ = backup;
        emit programApplied(metadata_, backup);
        setStatus();
    });
    body->addLayout(actions);
    auto *scroll = new QScrollArea;
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidgetResizable(true);
    scroll->setMinimumHeight(160);
    scroll->setWidget(body_);
    body_ = scroll;
    layout->addWidget(body_, 1);
    connect(kind_, qOverload<int>(&QComboBox::currentIndexChanged), this, [this] { updateFields(); });
    connect(tree_, &QTreeWidget::currentItemChanged, this, [this] { select(); });
    connect(add_, &QPushButton::clicked, this, &BlockEditor::add);
    connect(remove_, &QPushButton::clicked, this, &BlockEditor::remove);
    connect(apply_, &QPushButton::clicked, this, &BlockEditor::apply);
    connect(reset_, &QPushButton::clicked, this, &BlockEditor::reset);
    connect(functions, &QPushButton::clicked, this, [this] {
        if (locked_ || !valid_)
            return;
        Function f;
        f.id = uid().toStdString();
        f.name = "function" + std::to_string(program_.functions.size() + 1);
        program_.functions.push_back(f);
        metadata_["program"] = BlockPersistence::encode(program_);
        emit metadataChanged();
        rebuild();
        setStatus();
    });
    connect(mode_, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int mode) {
        body_->setVisible(mode != 2);
        if (!loading_) {
            metadata_["mode"] = mode;
            emit metadataChanged();
        }
        emit modeChanged(mode);
    });
    connect(tree_->model(), &QAbstractItemModel::rowsMoved, this, [this] {
        if (!loading_) {
            program_ = collect();
            metadata_["program"] = BlockPersistence::encode(program_);
            emit metadataChanged();
            setStatus();
        }
    });
}
void BlockEditor::setSketch(const SketchProject &sketch, const thinkercad::BoardProfile &board) {
    loading_ = true;
    arduino_ = sketch.language == "arduino";
    board_ = board;
    metadata_ = sketch.visual;
    source_ = sketch.files.value(sketch.entryPoint());
    auxiliaryFingerprint_ = auxiliaryFingerprint(sketch);
    valid_ = true;
    try {
        program_ = metadata_.contains("program") ? BlockPersistence::decode(metadata_["program"].toObject())
                                                 : BlockProgram{};
    } catch (...) {
        valid_ = false;
        program_ = {};
    }
    const int mode = sketch.language == "arduino" ? std::clamp(metadata_["mode"].toInt(2), 0, 2) : 2;
    mode_->setCurrentIndex(mode);
    mode_->setEnabled(sketch.language == "arduino" && !locked_);
    body_->setVisible(mode != 2);
    rebuild();
    loading_ = false;
    setStatus();
    emit modeChanged(mode);
}
void BlockEditor::sourceEdited(const QString &source) {
    source_ = source;
    setStatus();
}
void BlockEditor::sourcesEdited(const SketchProject &sketch) {
    auxiliaryFingerprint_ = auxiliaryFingerprint(sketch);
    sourceEdited(sketch.files.value(sketch.entryPoint()));
}
void BlockEditor::setExecutionLocked(bool value) {
    locked_ = value;
    mode_->setEnabled(!value && arduino_);
    tree_->setEnabled(!value && arduino_);
    view_->setInteractive(!value);
    setStatus();
}
void BlockEditor::setStatus() {
    const auto generated = ArduinoCodeGenerator::generate(program_, board_);
    const bool sync = metadata_.contains("program") && generated.valid() &&
                      QString::fromStdString(generated.source) == source_ &&
                      metadata_["fingerprint"].toString() == BlockPersistence::fingerprint(source_) &&
                      metadata_["auxiliaryFingerprint"].toString(auxiliaryFingerprint(SketchProject{})) ==
                          auxiliaryFingerprint_;
    status_->setVisible(metadata_.contains("program") || mode_->currentIndex() != 2);
    status_->setText(Language::text(
        !metadata_.contains("program")
            ? "C++ bleibt erhalten. Zum Start ein Blockprogramm anlegen und ausdrücklich erzeugen."
        : !valid_ ? "Blockdaten sind unbekannt oder beschädigt. C++ bleibt erhalten."
        : sync
            ? "Blöcke und C++ synchron"
            : "Nicht synchron: Blöcke oder C++ wurden geändert. C++ bleibt bis zum Erzeugen unverändert."));
    apply_->setEnabled(valid_ && !locked_ && arduino_);
    reset_->setEnabled(!locked_ && arduino_);
    add_->setEnabled(valid_ && !locked_ && arduino_);
    remove_->setEnabled(valid_ && !locked_ && arduino_);
    if (!arduino_)
        status_->hide();
}
void BlockEditor::rebuild() {
    QSignalBlocker block(tree_);
    tree_->clear();
    auto container = [&](QTreeWidgetItem *parent, const QString &label) {
        auto *item = parent ? new QTreeWidgetItem(parent) : new QTreeWidgetItem(tree_);
        item->setText(0, label);
        item->setData(0, Qt::UserRole, "container");
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDropEnabled);
        return item;
    };
    std::function<void(QTreeWidgetItem *, const std::vector<Block> &)> fill =
        [&](QTreeWidgetItem *parent, const std::vector<Block> &list) {
            for (const auto &b : list) {
                auto *item = new QTreeWidgetItem(parent);
                item->setData(0, Qt::UserRole, "block");
                BlockProgram one;
                one.setup = {b};
                auto json = BlockPersistence::encode(one)["setup"].toArray()[0].toObject();
                item->setData(0, Qt::UserRole + 1, json);
                QString title = Language::text(kinds[int(b.kind)]);
                if (!b.name.empty())
                    title += " · " + q(b.name);
                if (!b.arguments.empty())
                    title += " · " + expressionText(b.arguments[0]);
                item->setText(0, title);
                item->setToolTip(0, title);
                item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled);
                if (b.kind == Block::Kind::If || b.kind == Block::Kind::While || b.kind == Block::Kind::For) {
                    fill(container(item, Language::text("Anweisungen")), b.body);
                    if (b.kind == Block::Kind::If)
                        fill(container(item, Language::text("Sonst")), b.otherwise);
                }
            }
        };
    fill(container(nullptr, "setup()"), program_.setup);
    fill(container(nullptr, "loop()"), program_.loop);
    for (const auto &f : program_.functions) {
        auto *item = container(nullptr, Language::text("Funktion") + " · " + q(f.name));
        item->setData(0, Qt::UserRole, "function");
        BlockProgram one;
        one.functions = {f};
        item->setData(0, Qt::UserRole + 1,
                      BlockPersistence::encode(one)["functions"].toArray()[0].toObject());
        fill(container(item, Language::text("Anweisungen")), f.body);
    }
    tree_->expandAll();
    tree_->setCurrentItem(tree_->topLevelItem(1));
    scene_->setProgram(program_, metadata_["layout"].toObject());
    updateFields();
}
BlockProgram BlockEditor::collect() const {
    auto encoded = BlockPersistence::encode(program_);
    std::function<QJsonArray(QTreeWidgetItem *)> read = [&](QTreeWidgetItem *parent) {
        QJsonArray values;
        for (int i = 0; i < parent->childCount(); ++i) {
            auto *item = parent->child(i);
            if (item->data(0, Qt::UserRole).toString() != "block")
                continue;
            auto o = item->data(0, Qt::UserRole + 1).toJsonObject();
            if (item->childCount() > 0)
                o["body"] = read(item->child(0));
            if (item->childCount() > 1)
                o["otherwise"] = read(item->child(1));
            values.append(o);
        }
        return values;
    };
    encoded["setup"] = read(tree_->topLevelItem(0));
    encoded["loop"] = read(tree_->topLevelItem(1));
    QJsonArray functions;
    for (int i = 2; i < tree_->topLevelItemCount(); ++i) {
        auto *item = tree_->topLevelItem(i);
        auto f = item->data(0, Qt::UserRole + 1).toJsonObject();
        f["body"] = read(item->child(0));
        functions.append(f);
    }
    encoded["functions"] = functions;
    return BlockPersistence::decode(encoded);
}
void BlockEditor::select() {
    auto *item = tree_->currentItem();
    if (!item) {
        updateFields();
        return;
    }
    auto role = item->data(0, Qt::UserRole).toString();
    auto o = item->data(0, Qt::UserRole + 1).toJsonObject();
    if (role == "function") {
        name_->setText(o["name"].toString());
        type_->setCurrentIndex(o["result"].toInt(4));
        QStringList parameters;
        for (auto a : o["parameters"].toArray()) {
            auto p = a.toObject();
            parameters << p["name"].toString() + ": " + Language::text(types[p["type"].toInt()]);
        }
        parameters_->setText(parameters.join(", "));
        arguments_->clear();
        updateFields();
        return;
    }
    if (role != "block") {
        updateFields();
        return;
    }
    BlockProgram one;
    auto obj = BlockPersistence::encode(one);
    obj["setup"] = QJsonArray{o};
    auto b = BlockPersistence::decode(obj).setup[0];
    kind_->setCurrentIndex(int(b.kind));
    name_->setText(q(b.name));
    command_->setCurrentText(q(b.name));
    type_->setCurrentIndex(int(b.type));
    QStringList args;
    for (const auto &a : b.arguments)
        args << expressionText(a);
    arguments_->setPlainText(args.join('\n'));
    updateFields();
}
void BlockEditor::add() {
    if (locked_ || !valid_)
        return;
    auto *target = tree_->currentItem();
    if (!target)
        target = tree_->topLevelItem(1);
    if (target->data(0, Qt::UserRole).toString() == "function")
        target = target->child(0);
    if (target->data(0, Qt::UserRole).toString() == "block")
        target = target->childCount() ? target->child(0) : target->parent();
    Block b;
    b.id = uid().toStdString();
    b.name = "delay";
    b.arguments = {Expression{Expression::Kind::Number, "100", {}}};
    BlockProgram one;
    one.setup = {b};
    auto *item = new QTreeWidgetItem(target);
    item->setData(0, Qt::UserRole, "block");
    item->setData(0, Qt::UserRole + 1, BlockPersistence::encode(one)["setup"].toArray()[0].toObject());
    item->setText(0, "delay(100)");
    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled);
    target->setExpanded(true);
    tree_->setCurrentItem(item);
    program_ = collect();
    metadata_["program"] = BlockPersistence::encode(program_);
    emit metadataChanged();
    setStatus();
}
void BlockEditor::remove() {
    auto *item = tree_->currentItem();
    if (!item || locked_)
        return;
    const auto role = item->data(0, Qt::UserRole).toString();
    if (role != "block" && role != "function")
        return;
    delete item;
    program_ = collect();
    metadata_["program"] = BlockPersistence::encode(program_);
    emit metadataChanged();
    setStatus();
}
void BlockEditor::apply() {
    if (locked_ || !arduino_ || !valid_)
        return;
    try {
        auto *item = tree_->currentItem();
        if (item) {
            const auto role = item->data(0, Qt::UserRole).toString();
            if (role == "block") {
                BlockProgram one;
                auto data = BlockPersistence::encode(one);
                data["setup"] = QJsonArray{item->data(0, Qt::UserRole + 1).toJsonObject()};
                auto b = BlockPersistence::decode(data).setup[0];
                auto current = collect();
                std::function<void(const std::vector<Block> &)> find = [&](const std::vector<Block> &nodes) {
                    for (const auto &node : nodes) {
                        if (node.id == b.id)
                            b = node;
                        find(node.body);
                        find(node.otherwise);
                    }
                };
                find(current.setup);
                find(current.loop);
                for (const auto &f : current.functions)
                    find(f.body);
                b.kind = Block::Kind(kind_->currentIndex());
                b.type = ValueType(type_->currentIndex());
                b.name =
                    (b.kind == Block::Kind::Call ? command_->currentText() : name_->text()).toStdString();
                b.arguments.clear();
                for (const auto &line : arguments_->toPlainText().split('\n'))
                    if (!line.trimmed().isEmpty())
                        b.arguments.push_back(ExpressionParser::parse(line.toStdString()));
                if (b.kind != Block::Kind::If)
                    b.otherwise.clear();
                if (b.kind != Block::Kind::If && b.kind != Block::Kind::While && b.kind != Block::Kind::For)
                    b.body.clear();
                one.setup = {b};
                item->setData(0, Qt::UserRole + 1,
                              BlockPersistence::encode(one)["setup"].toArray()[0].toObject());
                while (item->childCount())
                    delete item->takeChild(0);
            } else if (role == "function") {
                auto f = item->data(0, Qt::UserRole + 1).toJsonObject();
                f["name"] = name_->text();
                f["result"] = type_->currentIndex();
                item->setData(0, Qt::UserRole + 1, f);
            }
        }
        program_ = collect();
        metadata_["program"] = BlockPersistence::encode(program_);
        emit metadataChanged();
        rebuild();
        reset();
    } catch (const std::exception &e) {
        status_->setText(Language::text(QString::fromUtf8(e.what())));
    }
}
void BlockEditor::reset() {
    if (locked_ || !arduino_)
        return;
    try {
        auto candidate = program_;
        if (!valid_) {
            if (QMessageBox::question(
                    this, Language::text("Blockdaten zurücksetzen"),
                    Language::text(
                        "Unbekannte Blockdaten entfernen? Der vorhandene C++-Sketch wird gesichert.")) !=
                QMessageBox::Yes)
                return;
            candidate = {};
        }
        auto generated = ArduinoCodeGenerator::generate(candidate, board_);
        if (!generated.valid()) {
            status_->setText(Language::text(QString::fromStdString(generated.issues[0].code)) + " · " +
                             QString::fromStdString(generated.issues[0].block));
            return;
        }
        const bool replaces =
            !source_.isEmpty() &&
            (metadata_["fingerprint"].toString() != BlockPersistence::fingerprint(source_) ||
             metadata_["auxiliaryFingerprint"].toString(auxiliaryFingerprint(SketchProject{})) !=
                 auxiliaryFingerprint_);
        if (replaces && QMessageBox::question(
                            this, Language::text("C++ neu erzeugen"),
                            Language::text("Manuellen C++-Sketch sichern und durch die Blöcke ersetzen?"),
                            QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
            return;
        program_ = std::move(candidate);
        valid_ = true;
        metadata_["program"] = BlockPersistence::encode(program_);
        metadata_["generator"] = int(ArduinoCodeGenerator::version);
        metadata_["auxiliaryFingerprint"] = auxiliaryFingerprint_;
        metadata_["fingerprint"] = BlockPersistence::fingerprint(QString::fromStdString(generated.source));
        if (replaces ||
            (!metadata_["backup"].isString() && source_ != QString::fromStdString(generated.source)))
            metadata_["backup"] = source_;
        metadata_["mode"] = mode_->currentIndex();
        source_ = QString::fromStdString(generated.source);
        emit programApplied(metadata_, source_);
        setStatus();
    } catch (const std::exception &e) {
        status_->setText(Language::text(QString::fromUtf8(e.what())));
    }
}

void BlockEditor::retranslate() {
    Language::translateTree(this);
    rebuild();
    setStatus();
}

void BlockEditor::updateFields() {
    const auto *item = tree_->currentItem();
    const auto role = item ? item->data(0, Qt::UserRole).toString() : QString();
    const bool block = role == "block", function = role == "function";
    const auto kind = Block::Kind(kind_->currentIndex());
    properties_->setRowVisible(kind_, block);
    properties_->setRowVisible(
        name_, function || (block && (kind == Block::Kind::Declare || kind == Block::Kind::Assign ||
                                      kind == Block::Kind::For)));
    properties_->setRowVisible(command_, block && kind == Block::Kind::Call);
    properties_->setRowVisible(type_, function || (block && kind == Block::Kind::Declare));
    properties_->setRowVisible(parameters_, function);
    properties_->setRowVisible(parametersButton_, function);
    properties_->setRowVisible(arguments_, block);
}

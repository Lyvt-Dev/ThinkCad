#pragma once
#include "language.hpp"
#include <QDialog>
#include <QDialogButtonBox>
#include <QEvent>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QSettings>
#include <QPushButton>
#include <QVBoxLayout>
#include <algorithm>
#include <optional>

struct ComponentSearchEntry {
    QString label, category, model, description;
    int kind = 0;
    QString id() const {
        return QString::number(kind) + "/" + model;
    }
};
class ComponentFinder final : public QDialog {
    std::vector<ComponentSearchEntry> entries_;
    QLineEdit *search_;
    QListWidget *results_;
    std::optional<ComponentSearchEntry> chosen_;
    void filter() {
        results_->clear();
        const auto words = search_->text().simplified().split(' ', Qt::SkipEmptyParts);
        for (std::size_t i = 0; i < entries_.size(); ++i) {
            const auto &e = entries_[i];
            const auto haystack = e.label + " " + e.category + " " + e.model + " " + e.description;
            if (!std::all_of(words.begin(), words.end(),
                             [&](const QString &w) { return haystack.contains(w, Qt::CaseInsensitive); }))
                continue;
            auto *item = new QListWidgetItem(e.label + "\n" + e.category + " · " + e.description, results_);
            item->setData(Qt::UserRole, int(i));
            item->setToolTip(item->text());
        }
        if (results_->count())
            results_->setCurrentRow(0);
        buttons_->button(QDialogButtonBox::Ok)->setEnabled(results_->count() > 0);
    }
    QDialogButtonBox *buttons_;
    void choose() {
        if (auto *item = results_->currentItem()) {
            chosen_ = entries_.at(item->data(Qt::UserRole).toInt());
            accept();
        }
    }

  protected:
    bool eventFilter(QObject *watched, QEvent *event) override {
        if (watched == search_ && event->type() == QEvent::KeyPress) {
            auto *key = static_cast<QKeyEvent *>(event);
            if (key->key() == Qt::Key_Down || key->key() == Qt::Key_Up) {
                if (results_->count())
                    results_->setCurrentRow(
                        std::clamp(results_->currentRow() + (key->key() == Qt::Key_Down ? 1 : -1), 0,
                                   results_->count() - 1));
                return true;
            }
        }
        return QDialog::eventFilter(watched, event);
    }

  public:
    explicit ComponentFinder(std::vector<ComponentSearchEntry> entries, QWidget *parent = nullptr)
        : QDialog(parent), entries_(std::move(entries)) {
        setObjectName("componentFinder");
        setWindowTitle(Language::text("Bauteil suchen"));
        resize(610, 480);
        QSettings settings;
        const auto recent = settings.value("componentFinder/recent").toStringList();
        const auto counts = settings.value("componentFinder/counts").toMap();
        std::stable_sort(entries_.begin(), entries_.end(), [&](const auto &a, const auto &b) {
            const int ai = recent.indexOf(a.id()), bi = recent.indexOf(b.id());
            if (ai >= 0 || bi >= 0)
                return ai >= 0 && (bi < 0 || ai < bi);
            return counts.value(a.id()).toInt() > counts.value(b.id()).toInt();
        });
        auto *layout = new QVBoxLayout(this);
        search_ = new QLineEdit;
        search_->setObjectName("componentFinderQuery");
        search_->setPlaceholderText(Language::text("Komponenten suchen…"));
        search_->setAccessibleName(Language::text("Bauteil suchen"));
        search_->setClearButtonEnabled(true);
        layout->addWidget(search_);
        auto *hint = new QLabel(Language::text(
            "Zuletzt verwendete Bauteile zuerst. Pfeiltasten: wählen · Enter: einfügen · Esc: schließen"));
        hint->setWordWrap(true);
        layout->addWidget(hint);
        results_ = new QListWidget;
        results_->setObjectName("componentFinderResults");
        results_->setWordWrap(true);
        results_->setAccessibleName(Language::text("Suchergebnisse"));
        layout->addWidget(results_);
        buttons_ = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        layout->addWidget(buttons_);
        buttons_->button(QDialogButtonBox::Ok)->setText(Language::text("Bauteil einfügen"));
        buttons_->button(QDialogButtonBox::Cancel)->setText(Language::text("Abbrechen"));
        connect(buttons_, &QDialogButtonBox::accepted, this, [this] { choose(); });
        connect(buttons_, &QDialogButtonBox::rejected, this, &QDialog::reject);
        connect(results_, &QListWidget::itemActivated, this, [this] { choose(); });
        connect(search_, &QLineEdit::returnPressed, this, [this] { choose(); });
        connect(search_, &QLineEdit::textChanged, this, [this] { filter(); });
        search_->installEventFilter(this);
        filter();
        search_->setFocus();
    }
    const std::optional<ComponentSearchEntry> &chosen() const {
        return chosen_;
    }
    static void remember(const ComponentSearchEntry &entry) {
        QSettings settings;
        auto recent = settings.value("componentFinder/recent").toStringList();
        recent.removeAll(entry.id());
        recent.prepend(entry.id());
        while (recent.size() > 12)
            recent.removeLast();
        settings.setValue("componentFinder/recent", recent);
        auto counts = settings.value("componentFinder/counts").toMap();
        if (counts.size() < 512 || counts.contains(entry.id()))
            counts[entry.id()] = std::min(1000000, counts.value(entry.id()).toInt() + 1);
        settings.setValue("componentFinder/counts", counts);
    }
};

#include "blockscene.hpp"
#include "language.hpp"
#include <QGraphicsObject>
#include <QGraphicsLineItem>
#include <QPainter>
#include <QApplication>
#include <QPalette>
#include <QKeyEvent>
#include <QJsonArray>
#include <cmath>
using namespace thinkercad::visual;
namespace {
class Card final : public QGraphicsObject {
    QString text_;
    std::function<void(QPointF)> moved_;

  public:
    Card(QString id, QString text, std::function<void(QPointF)> moved)
        : text_(std::move(text)), moved_(std::move(moved)) {
        setData(0, id);
        setFlags(ItemIsSelectable | ItemIsMovable | ItemIsFocusable | ItemSendsGeometryChanges);
        setToolTip(text_);
    }
    QRectF boundingRect() const override {
        return {0, 0, 260, 48};
    }
    void paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *) override {
        auto palette = QApplication::palette();
        p->setPen(
            QPen(palette.color(isSelected() ? QPalette::Highlight : QPalette::Mid), isSelected() ? 2 : 1));
        p->setBrush(palette.color(QPalette::Button));
        p->drawRoundedRect(boundingRect().adjusted(1, 1, -1, -1), 4, 4);
        p->setPen(palette.color(QPalette::ButtonText));
        p->drawText(boundingRect().adjusted(10, 5, -10, -5), Qt::AlignVCenter | Qt::TextWordWrap, text_);
    }
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override {
        auto result = QGraphicsObject::itemChange(change, value);
        if (change == ItemPositionHasChanged && moved_)
            moved_(pos());
        return result;
    }
    void keyPressEvent(QKeyEvent *e) override {
        QPointF delta;
        switch (e->key()) {
        case Qt::Key_Left:
            delta = {-10, 0};
            break;
        case Qt::Key_Right:
            delta = {10, 0};
            break;
        case Qt::Key_Up:
            delta = {0, -10};
            break;
        case Qt::Key_Down:
            delta = {0, 10};
            break;
        default:
            QGraphicsObject::keyPressEvent(e);
            return;
        }
        moveBy(delta.x(), delta.y());
        e->accept();
    }
};
} // namespace
BlockScene::BlockScene(QObject *parent) : QGraphicsScene(parent) {
    connect(this, &QGraphicsScene::selectionChanged, this, [this] {
        if (!loading_ && !selectedItems().isEmpty())
            emit blockSelected(selectedItems().front()->data(0).toString());
    });
}
void BlockScene::setProgram(const BlockProgram &program, const QJsonObject &positions) {
    loading_ = true;
    updateLinks_ = {};
    clear();
    struct Link {
        QGraphicsLineItem *line;
        Card *a;
        Card *b;
    };
    auto links = std::make_shared<std::vector<Link>>();
    const char *kinds[] = {"Variable",     "Zuweisung", "Wenn / Sonst", "Solange",
                           "Zählschleife", "Aufruf",    "Rückgabe"};
    auto card = [&](const QString &id, const QString &label, QPointF position) {
        auto *item = new Card(id, label, [this, id](QPointF pos) {
            if (loading_)
                return;
            if (updateLinks_)
                updateLinks_();
            emit blockMoved(id, pos);
        });
        addItem(item);
        auto stored = positions[id].toArray();
        if (stored.size() == 2 && stored[0].isDouble() && stored[1].isDouble() &&
            std::isfinite(stored[0].toDouble()) && std::isfinite(stored[1].toDouble()) &&
            std::abs(stored[0].toDouble()) < 100000 && std::abs(stored[1].toDouble()) < 100000)
            position = {stored[0].toDouble(), stored[1].toDouble()};
        item->setPos(position);
        return item;
    };
    std::function<void(const std::vector<Block> &, double &, int, Card *)> fill =
        [&](const std::vector<Block> &blocks, double &y, int depth, Card *previous) {
            for (const auto &b : blocks) {
                auto label = Language::text(kinds[int(b.kind)]);
                if (!b.name.empty())
                    label += " · " + QString::fromStdString(b.name);
                auto *item = card(QString::fromStdString(b.id), label, {20. + depth * 35., y});
                if (previous) {
                    auto *line = addLine({}, QPen(QApplication::palette().color(QPalette::Mid), 2));
                    line->setZValue(-1);
                    links->push_back({line, previous, item});
                }
                previous = item;
                y += 66;
                if (!b.body.empty())
                    fill(b.body, y, depth + 1, item);
                if (!b.otherwise.empty()) {
                    auto *label = addText(Language::text("Sonst"));
                    label->setDefaultTextColor(QApplication::palette().color(QPalette::Text));
                    label->setPos(55. + depth * 35., y);
                    y += 28;
                    fill(b.otherwise, y, depth + 1, item);
                }
            }
        };
    double y = 20;
    for (bool setup : {true, false}) {
        const auto id = setup ? "setup" : "loop";
        auto *header = card(id, setup ? "setup()" : "loop()", {20, y});
        y += 66;
        fill(setup ? program.setup : program.loop, y, 0, header);
        y += 30;
    }
    for (const auto &f : program.functions) {
        auto *header = card(QString::fromStdString(f.id),
                            Language::text("Funktion") + " · " + QString::fromStdString(f.name), {20, y});
        y += 66;
        fill(f.body, y, 1, header);
        y += 30;
    }
    updateLinks_ = [links] {
        for (const auto &link : *links)
            link.line->setLine(QLineF(link.a->pos() + QPointF(130, 48), link.b->pos() + QPointF(130, 0)));
    };
    updateLinks_();
    setSceneRect(itemsBoundingRect().adjusted(-20, -20, 100, 100));
    loading_ = false;
}

#pragma once
#include "thinkercad/visual/blockprogram.hpp"
#include <QGraphicsScene>
#include <QJsonObject>
class BlockScene final : public QGraphicsScene {
    Q_OBJECT
  public:
    explicit BlockScene(QObject *parent = nullptr);
    void setProgram(const thinkercad::visual::BlockProgram &, const QJsonObject &positions);
  signals:
    void blockSelected(const QString &id);
    void blockMoved(const QString &id, const QPointF &position);

  private:
    bool loading_ = false;
    std::function<void()> updateLinks_;
};

#pragma once
#include "thinkercad/circuit.hpp"
#include <QGraphicsView>
#include <QUndoStack>
#include <functional>
class PartItem;
class QGraphicsPathItem;
class CircuitCanvas final : public QGraphicsView {
    Q_OBJECT
public:
    CircuitCanvas(thinkercad::CircuitDocument& document,QWidget* parent=nullptr);
    ~CircuitCanvas() override;
    QUndoStack* history(){return &history_;}
    void rebuild();
    void resetHistory();
    void mutate(const QString& label,std::function<void(thinkercad::CircuitDocument&)> action);
    void applyDocument(const thinkercad::CircuitDocument& document);
    void setEditingEnabled(bool enabled);
    void setWireMode(bool enabled);
    void setWireColor(const QColor& color){wireColor_=color;}
    void addPart(thinkercad::PartKind kind);
    void removeSelected();
    void fitCircuit();
    void zoom(double factor);
    void cancelWire();
    void connectPin(const thinkercad::Endpoint& pin);
    void toggleButton(const std::string& id);
    void changeValue(const std::string& id,double value);
    void recordMove(const thinkercad::CircuitDocument& before);
    void moveBend(const std::string& id,std::size_t index,QPointF position);
    void addBend(const std::string& id,QPointF position);
    void updateSimulation(const thinkercad::CircuitState& state,const std::map<std::string,thinkercad::OledDisplay>& displays);
    QString selectedPart() const;
    void selectPart(const std::string& id);
    const thinkercad::CircuitDocument& document() const{return document_;}
    bool editingEnabled() const{return editingEnabled_;}
signals:
    void documentChanged();
    void selectionChanged();
    void runtimeInputChanged();
    void inputValueChanged();
    void notice(const QString& text);
protected:
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void drawBackground(QPainter* painter,const QRectF& rect) override;
private:
    thinkercad::CircuitDocument& document_;
    QUndoStack history_;
    std::map<std::string,PartItem*> parts_;
    std::optional<thinkercad::Endpoint> wireStart_;
    QGraphicsPathItem* preview_=nullptr;
    QColor wireColor_{"#0d9488"};
    bool editingEnabled_=true,wireMode_=false,panning_=false;
    QPoint panOrigin_;
    std::vector<thinkercad::WirePoint> pendingBends_;
    QPointF lastPointer_;
    void refreshBendHandles();
    QPainterPath previewPath(QPointF end)const;
    QPointF pinPosition(const thinkercad::Endpoint& pin) const;
    QPainterPath routedWire(const thinkercad::CircuitWire& wire) const;
    void redrawWires();
};

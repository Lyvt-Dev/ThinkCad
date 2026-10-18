#pragma once
#include "thinkercad/circuit.hpp"
#include "projectio.hpp"
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
    void rotateSelected();
    std::function<QMap<QString,SketchProject>()> clipboardSketches;
    std::function<void(const QMap<QString,SketchProject>&)> importClipboardSketches;
    void copySelected();
    void paste();
    void duplicateSelected();
    void alignSelected(bool horizontal);
    void distributeSelected(bool horizontal);
    void highlightNet(const thinkercad::Endpoint& endpoint);
    void setGrid(double size,bool snap){gridSize_=size;snap_=snap;viewport()->update();}
    QPointF snapped(QPointF position)const;
    void branchWire(const std::string& id,QPointF position);

    void fitCircuit();
    void zoom(double factor);
    void cancelWire();
    void connectPin(const thinkercad::Endpoint& pin);
    void editNote(const std::string& id);
    void toggleButton(const std::string& id);
    void setButtonPressed(const std::string& id,bool pressed);
    void setEncoderInput(const std::string& id,int phase,int switchState=-1);
    void setKeyPressed(const std::string& id,int key,bool pressed);
    void changeValue(const std::string& id,double value);
    void recordMove(const thinkercad::CircuitDocument& before);
    QPointF constrainBend(const std::string& id,std::size_t index,QPointF position,Qt::KeyboardModifiers modifiers)const;
    void moveBend(const std::string& id,std::size_t index,QPointF position,Qt::KeyboardModifiers modifiers=Qt::NoModifier);
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
    void componentInputRequested(const QString& id);
    void inputValueChanged();
    void notice(const QString& text);
protected:
    void focusOutEvent(QFocusEvent* event) override;
    void dragEnterEvent(QDragEnterEvent*) override;
    void dragMoveEvent(QDragMoveEvent*) override;
    void dragLeaveEvent(QDragLeaveEvent*) override;
    void dropEvent(QDropEvent*) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void drawBackground(QPainter* painter,const QRectF& rect) override;
private:
    thinkercad::CircuitDocument& document_;
    QUndoStack history_;
    std::map<std::string,PartItem*> parts_;
    std::optional<thinkercad::Endpoint> wireStart_;
    QGraphicsPathItem* preview_=nullptr;
    QGraphicsItem* placementPreview_=nullptr;
    void clearPlacementPreview();
    QColor wireColor_{"#0d9488"};
    bool editingEnabled_=true,wireMode_=false,panning_=false;
    QPoint panOrigin_;
    std::string heldButton_;
    double gridSize_=10;bool snap_=true;
    std::vector<thinkercad::WirePoint> pendingBends_;
    QPointF lastPointer_;
    void refreshBendHandles();
    QPainterPath previewPath(QPointF end)const;
    QPointF wirePoint(QPointF point,Qt::KeyboardModifiers modifiers)const;
    QPointF pinPosition(const thinkercad::Endpoint& pin) const;
    QPainterPath routedWire(const thinkercad::CircuitWire& wire) const;
    void redrawWires();
};

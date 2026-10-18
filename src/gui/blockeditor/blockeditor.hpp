#pragma once
#include "projectio.hpp"
#include "blockpersistence.hpp"
#include <QWidget>
class BlockScene;
class QGraphicsView;
class QTreeWidget;
class QTreeWidgetItem;
class QComboBox;
class QLineEdit;
class QPlainTextEdit;
class QLabel;
class QPushButton;
class QFormLayout;
class BlockEditor final : public QWidget {
    Q_OBJECT
  public:
    explicit BlockEditor(QWidget *parent = nullptr);
    void setSketch(const SketchProject &, const thinkercad::BoardProfile &);
    void sourceEdited(const QString &source);
    void sourcesEdited(const SketchProject &sketch);
    QJsonObject metadata() const {
        return metadata_;
    }
    void setExecutionLocked(bool);
    void retranslate();
  signals:
    void programApplied(const QJsonObject &metadata, const QString &source);
    void metadataChanged();
    void modeChanged(int mode);

  private:
    BlockScene *scene_;
    QGraphicsView *view_;
    QTreeWidget *tree_;
    QComboBox *mode_, *kind_, *type_, *command_;
    QLineEdit *name_, *parameters_;
    QPlainTextEdit *arguments_;
    QLabel *status_;
    QPushButton *apply_, *reset_, *add_, *remove_;
    QWidget *body_;
    QFormLayout *properties_;
    QPushButton *parametersButton_;
    QJsonObject metadata_;
    thinkercad::visual::BlockProgram program_;
    thinkercad::BoardProfile board_;
    QString source_, auxiliaryFingerprint_;
    bool loading_ = false, valid_ = true, locked_ = false, arduino_ = true;
    void rebuild();
    void select();
    void updateFields();
    void add();
    void remove();
    void apply();
    void reset();
    void setStatus();
    thinkercad::visual::BlockProgram collect() const;
};

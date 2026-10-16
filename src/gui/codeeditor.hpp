#pragma once
#include <QPlainTextEdit>
#include <QSyntaxHighlighter>
class CodeEditor final:public QPlainTextEdit {
public:
    explicit CodeEditor(QWidget* parent=nullptr);
    void applyPreferences();
    int lineAreaWidth() const;
    void paintLineArea(QPaintEvent* event);
protected:
    void resizeEvent(QResizeEvent* event) override;
private:
    QWidget* lineArea_;
};

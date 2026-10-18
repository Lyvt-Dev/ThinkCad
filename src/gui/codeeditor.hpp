#pragma once
#include <QPlainTextEdit>
#include <QSyntaxHighlighter>
#include <QSet>
#include <QTextCursor>
class CodeEditor final:public QPlainTextEdit {
    Q_OBJECT
public:
    explicit CodeEditor(QWidget* parent=nullptr);
    void applyPreferences();
    int lineAreaWidth() const;
    void paintLineArea(QPaintEvent* event);
    void clickLineArea(int y);
    QSet<int> breakpoints()const;
    void setBreakpoints(const QSet<int>& lines);
    void setExecutionLine(int line);
signals:
    void breakpointsChanged();
protected:
    void resizeEvent(QResizeEvent* event) override;
private:
    QWidget* lineArea_=nullptr;
    QList<QTextCursor> breakpointCursors_;
    int executionLine_=0;
};

#include "codeeditor.hpp"
#include <QPainter>
#include <QApplication>
#include <QRegularExpression>
#include <QTextBlock>
#include <QResizeEvent>
#include <QSettings>
#include <QFontDatabase>
#include <QMouseEvent>
class LineArea final:public QWidget{
    CodeEditor* editor_;
public:explicit LineArea(CodeEditor* e):QWidget(e),editor_(e){}QSize sizeHint()const override{return {editor_->lineAreaWidth(),0};}
    void paintEvent(QPaintEvent* e)override{editor_->paintLineArea(e);}
    void mousePressEvent(QMouseEvent* e)override{if(e->button()==Qt::LeftButton)editor_->clickLineArea(qRound(e->position().y()));}
};
class CppColors final:public QSyntaxHighlighter{
public:explicit CppColors(QTextDocument* d):QSyntaxHighlighter(d){}
protected:void highlightBlock(const QString& text)override{
    auto apply=[&](const QString& pattern,const QColor& color){QTextCharFormat f;f.setForeground(qApp->property("darkMode").toBool()?color.lighter(150):color);auto m=QRegularExpression(pattern).globalMatch(text);while(m.hasNext()){auto found=m.next();setFormat(found.capturedStart(),found.capturedLength(),f);}};
    if(document()->property("language").toString()=="python"){
        apply("\\b(?:def|class|import|from|as|return|if|elif|else|for|while|in|is|not|and|or|try|except|finally|raise|with|pass|break|continue|lambda|global|nonlocal|yield|async|await|True|False|None)\\b",QColor("#7854b9"));
        apply("\\b(?:Pin|PWM|ADC|I2C|SPI|UART|print|range|len|bytes|bytearray|sleep|sleep_ms|sleep_us)\\b",QColor("#0d9488"));
        apply("\\b(?:0[xX][0-9a-fA-F]+|0[bB][01]+|[0-9]+(?:\\.[0-9]+)?)\\b",QColor("#b46b16"));
        const QRegularExpression tokens("('(?:\\\\.|[^'\\\\])*'|\"(?:\\\\.|[^\"\\\\])*\")|(#.*)");auto matches=tokens.globalMatch(text);
        while(matches.hasNext()){const auto token=matches.next();QTextCharFormat format;const QColor color=token.captured(2).isEmpty()?QColor("#39749b"):QColor("#8a97a9");format.setForeground(qApp->property("darkMode").toBool()?color.lighter(150):color);setFormat(token.capturedStart(),token.capturedLength(),format);}
        setCurrentBlockState(0);return;
    }
    apply("\\b(?:void|bool|char|int|long|short|unsigned|const|static|struct|class|return|if|else|for|while|switch|case|break|continue|auto|template|typename|true|false|uint8_t|uint16_t|uint32_t)\\b",QColor("#7854b9"));
    apply("\\b(?:setup|loop|pinMode|digitalWrite|digitalRead|analogRead|delay|millis|micros|HIGH|LOW|INPUT|OUTPUT|INPUT_PULLUP|PROGMEM|Wire|Serial)\\b",QColor("#0d9488"));
    apply("\\b(?:0x[0-9a-fA-F]+|[0-9]+(?:\\.[0-9]+)?)\\b",QColor("#b46b16"));apply("\"(?:\\\\.|[^\"\\\\])*\"",QColor("#39749b"));apply("^\\s*#.*",QColor("#526a87"));apply("//.*",QColor("#8a97a9"));
    setCurrentBlockState(0);int start=previousBlockState()==1?0:text.indexOf("/*");QTextCharFormat comment;comment.setForeground(QColor("#8a97a9"));while(start>=0){int end=text.indexOf("*/",start);if(end<0){setCurrentBlockState(1);setFormat(start,text.size()-start,comment);break;}setFormat(start,end-start+2,comment);start=text.indexOf("/*",end+2);}
}};
CodeEditor::CodeEditor(QWidget* parent):QPlainTextEdit(parent),lineArea_(new LineArea(this)){
    applyPreferences();setMinimumWidth(240);setObjectName("codeEditor");new CppColors(document());
    connect(document(),&QTextDocument::contentsChanged,this,[this]{lineArea_->update();if(!breakpointCursors_.isEmpty())emit breakpointsChanged();});
    connect(this,&QPlainTextEdit::blockCountChanged,this,[this]{setViewportMargins(lineAreaWidth(),0,0,0);});
    connect(this,&QPlainTextEdit::updateRequest,this,[this](const QRect& r,int dy){if(dy)lineArea_->scroll(0,dy);else lineArea_->update(0,r.y(),lineArea_->width(),r.height());});setViewportMargins(lineAreaWidth(),0,0,0);
}
void CodeEditor::applyPreferences(){
    QSettings settings;
    auto selected=QFontDatabase::systemFont(QFontDatabase::FixedFont);
    selected.setPointSize(std::clamp(settings.value("editorFontSize",10).toInt(),8,28));
    setFont(selected);
    setTabStopDistance(fontMetrics().horizontalAdvance(' ')*std::clamp(settings.value("editorTabWidth",4).toInt(),2,8));
    setLineWrapMode(settings.value("editorWordWrap",false).toBool()?QPlainTextEdit::WidgetWidth:QPlainTextEdit::NoWrap);
    if(lineArea_)setViewportMargins(lineAreaWidth(),0,0,0);
}
int CodeEditor::lineAreaWidth()const{int digits=1,n=std::max(1,blockCount());while(n>=10){n/=10;++digits;}return 30+fontMetrics().horizontalAdvance('9')*digits;}
void CodeEditor::resizeEvent(QResizeEvent* e){QPlainTextEdit::resizeEvent(e);lineArea_->setGeometry(contentsRect().left(),contentsRect().top(),lineAreaWidth(),contentsRect().height());}
void CodeEditor::paintLineArea(QPaintEvent* event){
    QPainter p(lineArea_);p.fillRect(event->rect(),palette().color(QPalette::AlternateBase));
    auto block=firstVisibleBlock();int number=block.blockNumber();int top=qRound(blockBoundingGeometry(block).translated(contentOffset()).top());int bottom=top+qRound(blockBoundingRect(block).height());const auto points=breakpoints();
    while(block.isValid()&&top<=event->rect().bottom()){
        if(block.isVisible()&&bottom>=event->rect().top()){
            p.setPen(palette().color(QPalette::PlaceholderText));p.drawText(16,top,lineArea_->width()-24,fontMetrics().height(),Qt::AlignRight,QString::number(number+1));
            if(points.contains(number+1)){p.setPen(Qt::NoPen);p.setBrush(QColor("#c83232"));p.drawEllipse(QRect(3,top+3,10,10));}
            if(executionLine_==number+1){p.setPen(QColor("#9a6700"));p.setBrush(QColor("#edb82b"));p.drawPolygon(QPolygon({QPoint(2,top+2),QPoint(14,top+8),QPoint(2,top+14)}));}
        }
        block=block.next();top=bottom;bottom=top+qRound(blockBoundingRect(block).height());++number;
    }
}
QSet<int> CodeEditor::breakpoints()const{QSet<int> lines;for(const auto& cursor:breakpointCursors_)if(!cursor.isNull())lines.insert(cursor.blockNumber()+1);return lines;}
void CodeEditor::setBreakpoints(const QSet<int>& lines){breakpointCursors_.clear();for(int line:lines){if(line<1||line>blockCount()||breakpointCursors_.size()>=2048)continue;breakpointCursors_.append(QTextCursor(document()->findBlockByNumber(line-1)));}lineArea_->update();}
void CodeEditor::clickLineArea(int y){if(isReadOnly())return;auto block=firstVisibleBlock();while(block.isValid()){const auto rect=blockBoundingGeometry(block).translated(contentOffset());if(y>=rect.top()&&y<rect.bottom()){auto lines=breakpoints();const int line=block.blockNumber()+1;if(lines.contains(line))lines.remove(line);else if(lines.size()<2048)lines.insert(line);setBreakpoints(lines);emit breakpointsChanged();return;}block=block.next();}}
void CodeEditor::setExecutionLine(int line){executionLine_=line;QList<QTextEdit::ExtraSelection> selections;if(line>=1&&line<=blockCount()){QTextEdit::ExtraSelection selection;selection.cursor=QTextCursor(document()->findBlockByNumber(line-1));selection.format.setBackground(palette().color(QPalette::Highlight).lighter(150));selection.format.setProperty(QTextFormat::FullWidthSelection,true);selections.append(selection);setTextCursor(selection.cursor);ensureCursorVisible();}setExtraSelections(selections);lineArea_->update();}

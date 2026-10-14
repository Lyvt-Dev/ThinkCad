#include "codeeditor.hpp"
#include <QPainter>
#include <QRegularExpression>
#include <QTextBlock>
#include <QResizeEvent>
class LineArea final:public QWidget{
    CodeEditor* editor_;
public:explicit LineArea(CodeEditor* e):QWidget(e),editor_(e){}QSize sizeHint()const override{return {editor_->lineAreaWidth(),0};}
    void paintEvent(QPaintEvent* e)override{editor_->paintLineArea(e);}
};
class CppColors final:public QSyntaxHighlighter{
public:explicit CppColors(QTextDocument* d):QSyntaxHighlighter(d){}
protected:void highlightBlock(const QString& text)override{
    auto apply=[&](const QString& pattern,const QColor& color){QTextCharFormat f;f.setForeground(color);auto m=QRegularExpression(pattern).globalMatch(text);while(m.hasNext()){auto found=m.next();setFormat(found.capturedStart(),found.capturedLength(),f);}};
    apply("\\b(?:void|bool|char|int|long|short|unsigned|const|static|struct|class|return|if|else|for|while|switch|case|break|continue|auto|template|typename|true|false|uint8_t|uint16_t|uint32_t)\\b",QColor("#7854b9"));
    apply("\\b(?:setup|loop|pinMode|digitalWrite|digitalRead|analogRead|delay|millis|micros|HIGH|LOW|INPUT|OUTPUT|INPUT_PULLUP|PROGMEM|Wire|Serial)\\b",QColor("#0d9488"));
    apply("\\b(?:0x[0-9a-fA-F]+|[0-9]+(?:\\.[0-9]+)?)\\b",QColor("#b46b16"));apply("\"(?:\\\\.|[^\"\\\\])*\"",QColor("#39749b"));apply("^\\s*#.*",QColor("#526a87"));apply("//.*",QColor("#8a97a9"));
    setCurrentBlockState(0);int start=previousBlockState()==1?0:text.indexOf("/*");QTextCharFormat comment;comment.setForeground(QColor("#8a97a9"));while(start>=0){int end=text.indexOf("*/",start);if(end<0){setCurrentBlockState(1);setFormat(start,text.size()-start,comment);break;}setFormat(start,end-start+2,comment);start=text.indexOf("/*",end+2);}
}};
CodeEditor::CodeEditor(QWidget* parent):QPlainTextEdit(parent),lineArea_(new LineArea(this)){
    setFont(QFont("Consolas",10));setTabStopDistance(28);setLineWrapMode(QPlainTextEdit::NoWrap);setMinimumWidth(240);setObjectName("codeEditor");new CppColors(document());
    connect(this,&QPlainTextEdit::blockCountChanged,this,[this]{setViewportMargins(lineAreaWidth(),0,0,0);});
    connect(this,&QPlainTextEdit::updateRequest,this,[this](const QRect& r,int dy){if(dy)lineArea_->scroll(0,dy);else lineArea_->update(0,r.y(),lineArea_->width(),r.height());});setViewportMargins(lineAreaWidth(),0,0,0);
}
int CodeEditor::lineAreaWidth()const{int digits=1,n=std::max(1,blockCount());while(n>=10){n/=10;++digits;}return 18+fontMetrics().horizontalAdvance('9')*digits;}
void CodeEditor::resizeEvent(QResizeEvent* e){QPlainTextEdit::resizeEvent(e);lineArea_->setGeometry(contentsRect().left(),contentsRect().top(),lineAreaWidth(),contentsRect().height());}
void CodeEditor::paintLineArea(QPaintEvent* event){QPainter p(lineArea_);p.fillRect(event->rect(),QColor("#f8fafc"));p.setPen(QColor("#9caabd"));auto block=firstVisibleBlock();int number=block.blockNumber();int top=qRound(blockBoundingGeometry(block).translated(contentOffset()).top());int bottom=top+qRound(blockBoundingRect(block).height());while(block.isValid()&&top<=event->rect().bottom()){if(block.isVisible()&&bottom>=event->rect().top())p.drawText(0,top,lineArea_->width()-8,fontMetrics().height(),Qt::AlignRight,QString::number(number+1));block=block.next();top=bottom;bottom=top+qRound(blockBoundingRect(block).height());++number;}}

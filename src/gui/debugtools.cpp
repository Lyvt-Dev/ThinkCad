#include "debugtools.hpp"
#include "language.hpp"
#include <QComboBox>
#include <QLabel>
#include <QTreeWidget>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
namespace {QString text(const char* de,const char* en){return Language::text(QString::fromUtf8(Language::current()=="de"?de:en));}}
DebugTools::DebugTools(QWidget* parent):QWidget(parent){
    setObjectName("sourceDebugger");auto* layout=new QVBoxLayout(this);auto* actions=new QHBoxLayout;layout->addLayout(actions);
    auto button=[&](const QString& label,const char* name,auto function){auto* b=new QPushButton(label);b->setObjectName(name);actions->addWidget(b);connect(b,&QPushButton::clicked,this,function);};
    button(text("Debuggen","Debug"),"startSourceDebugging",[this]{emit startRequested();});
    button(text("Fortsetzen","Continue"),"debugContinue",[this]{emit continueRequested(boards_->currentText());});
    button(text("Anhalten","Interrupt"),"debugInterrupt",[this]{if(auto* s=selected())s->interrupt();});
    button(text("Nächste Zeile","Next line"),"debugNext",[this]{if(auto* s=selected())s->next();});
    button(text("Hinein","Step into"),"debugStep",[this]{if(auto* s=selected())s->step();});
    button(text("Heraus","Step out"),"debugFinish",[this]{if(auto* s=selected())s->finish();});
    button(text("Stopp","Stop"),"debugStop",[this]{emit stopRequested();});actions->addStretch();
    boards_=new QComboBox;boards_->setObjectName("debugBoardSelector");layout->addWidget(boards_);status_=new QLabel(text("Lokales GDB · Haltepunkte am Zeilenrand setzen.","Local GDB · Click the line gutter to set breakpoints."));status_->setWordWrap(true);layout->addWidget(status_);
    auto* tabs=new QTabWidget;layout->addWidget(tabs);locals_=new QTreeWidget;locals_->setObjectName("debugVariables");locals_->setHeaderLabels({text("Variable","Variable"),text("Wert","Value")});tabs->addTab(locals_,text("Lokale Variablen","Local variables"));registers_=new QTreeWidget;registers_->setObjectName("debugHostRegisters");registers_->setHeaderLabels({text("PC-CPU-Register","Host CPU register"),text("Wert","Value")});tabs->addTab(registers_,text("PC-CPU (kein MCU)","Host CPU (not MCU)"));
    connect(boards_,qOverload<int>(&QComboBox::currentIndexChanged),this,[this]{locals_->clear();registers_->clear();if(auto* s=selected()){if(s->stopped()){s->inspectVariables();s->inspectRegisters();status_->setText(text("Angehalten","Stopped"));}else status_->setText(text("Läuft / wird vorbereitet","Running / preparing"));}});
}
GdbSession* DebugTools::selected()const{return sessions_.value(boards_->currentText());}
void DebugTools::clear(){for(const auto& s:sessions_)if(s)disconnect(s,nullptr,this,nullptr);sessions_.clear();boards_->clear();locals_->clear();registers_->clear();status_->setText(text("Debugger beendet","Debugger stopped"));}
void DebugTools::selectBoard(const QString& board){const auto at=boards_->findText(board);if(at>=0)boards_->setCurrentIndex(at);}
void DebugTools::addBoard(const QString& board,GdbSession* session){
    sessions_[board]=session;boards_->addItem(board);
    connect(session,&GdbSession::halted,this,[this,board,session](const auto& file,int line,const auto& reason){selectBoard(board);status_->setText(board+" · "+file+":"+QString::number(line)+" · "+reason);emit sourceStopped(board,file,line);session->inspectVariables();session->inspectRegisters();});
    connect(session,&GdbSession::running,this,[this,session]{if(selected()==session){status_->setText(text("Läuft","Running"));locals_->clear();registers_->clear();}});
    connect(session,&GdbSession::variables,this,[this,session](const auto& values){if(selected()!=session)return;locals_->clear();for(const auto& value:values){const auto item=value.toMap();new QTreeWidgetItem(locals_,{item.value("name").toString(),item.value("value").toString()});}locals_->resizeColumnToContents(0);});
    connect(session,&GdbSession::registers,this,[this,session](const auto& values){if(selected()!=session)return;registers_->clear();for(const auto& value:values){const auto item=value.toMap();new QTreeWidgetItem(registers_,{item.value("name",item.value("number")).toString(),item.value("value").toString()});}registers_->resizeColumnToContents(0);});
}

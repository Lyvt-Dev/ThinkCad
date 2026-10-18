#include "mainwindow.hpp"
#include "collaborationtools.hpp"
#include "collaborationproject.hpp"
#include "projectmerge.hpp"
#include "codeeditor.hpp"
#include "language.hpp"
#include <QDockWidget>
#include <QMenu>
#include <QSettings>
#include <QScopedValueRollback>
#include <QTabWidget>
#include <QDialog>
#include <QDialogButtonBox>
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include <QLabel>
#include <QJsonDocument>
#include <algorithm>
#include <QDir>
#include <QFile>
#include <QDateTime>
#include <QUuid>
namespace {QString text(const char* de,const char* en){return Language::text(QString::fromUtf8(Language::current()=="de"?de:en));}}
void MainWindow::setupCollaboration(QMenu* view){
    collaboration_.setValidator(CollaborationProject::validate);
    collaborationTools_=new CollaborationTools(&collaboration_);auto* dock=new QDockWidget(text("Lokale Zusammenarbeit","Local collaboration"),this);dock->setObjectName("collaborationDock");dock->setWidget(collaborationTools_);addDockWidget(Qt::BottomDockWidgetArea,dock);dock->hide();view->addAction(dock->toggleViewAction());
    collaborationDebounce_.setSingleShot(true);connect(&collaborationDebounce_,&QTimer::timeout,this,&MainWindow::publishCollaboration);connect(canvas_,&CircuitCanvas::documentChanged,this,&MainWindow::scheduleCollaboration);connect(canvas_,&CircuitCanvas::inputValueChanged,this,&MainWindow::scheduleCollaboration);
    connect(collaborationTools_,&CollaborationTools::hostRequested,this,[this]{hostCollaboration(QHostAddress::AnyIPv4,quint16(std::clamp(QSettings().value("collaborationPort",0).toInt(),0,65535)));});
    connect(collaborationTools_,&CollaborationTools::joinRequested,this,&MainWindow::joinCollaboration);connect(collaborationTools_,&CollaborationTools::acceptRemoteRequested,this,[this]{resolveCollaboration(false);});connect(collaborationTools_,&CollaborationTools::keepLocalRequested,this,[this]{resolveCollaboration(true);});connect(collaborationTools_,&CollaborationTools::compareRequested,this,&MainWindow::compareCollaboration);
    connect(&collaboration_,&LocalCollaboration::stateReceived,this,&MainWindow::receiveCollaboration);
    connect(&collaboration_,&LocalCollaboration::conflict,this,[this](const auto&,quint64,const auto& paths){collaborationSubmitted_=false;flagCollaborationConflict(paths);});
    connect(&collaboration_,&LocalCollaboration::error,this,[this](const auto& failure){appendLog(text("LAN: ","LAN: ")+failure);});
    connect(&collaboration_,&LocalCollaboration::connectionChanged,this,[this]{if(!collaboration_.connected()){collaborationDebounce_.stop();collaborationBase_={};submittedLocal_={};collaborationSubmitted_=false;collaborationJoining_=false;collaborationQueued_=false;collaborationConflict_=false;collaborationDraftPath_.clear();if(collaborationTools_)collaborationTools_->clearConflict();}else if(!collaboration_.identityNamespace().isEmpty())circuit_.setIdentityNamespace(collaboration_.identityNamespace().toStdString());});
}
bool MainWindow::hostCollaboration(const QHostAddress& address,quint16 port){
    stopSimulation();try{const auto doc=CollaborationProject::encode(snapshot());if(!collaboration_.host(doc,address,port,QSettings().value("collaborationMaxPeers",4).toInt()))return false;collaborationBase_=doc;circuit_.setIdentityNamespace(collaboration_.identityNamespace().toStdString());if(auto* dock=findChild<QDockWidget*>("collaborationDock"))dock->show();return true;}catch(const std::exception& error){appendLog(QString::fromUtf8(error.what()));return false;}
}
void MainWindow::joinCollaboration(const QString& address,quint16 port,const QString& ticket){
    if(!maybeSave())return;stopSimulation();collaboration_.join(address,port,ticket);collaborationJoining_=true;if(auto* dock=findChild<QDockWidget*>("collaborationDock"))dock->show();
}
void MainWindow::scheduleCollaboration(){if(collaboration_.connected())circuit_.setIdentityNamespace(collaboration_.identityNamespace().toStdString());if(collaboration_.connected()&&!collaborationApplying_&&!collaborationConflict_)collaborationDebounce_.start(std::clamp(QSettings().value("collaborationIntervalMs",200).toInt(),100,1000));}
void MainWindow::publishCollaboration(){
    if(!collaboration_.connected()||collaborationApplying_||collaborationSubmitted_||collaborationConflict_||collaborationBase_.isEmpty())return;
    try{const auto local=CollaborationProject::encode(snapshot());const auto authority=collaboration_.document();const auto merged=ProjectMerge::merge(collaborationBase_,authority,local);if(!merged.conflicts.isEmpty()){flagCollaborationConflict(merged.conflicts);return;}if(merged.document==authority)return;
        submittedLocal_=local;collaborationSubmitted_=true;collaboration_.submit(collaboration_.revision(),merged.document);
    }catch(const std::exception& failure){appendLog(QString::fromUtf8(failure.what()));}
}
void MainWindow::receiveCollaboration(const QJsonObject& document,quint64,const QString& origin){
    if(!collaboration_.connected())return;
    const bool acknowledged=collaborationSubmitted_&&origin==collaboration_.identityNamespace();
    if(acknowledged){collaborationSubmitted_=false;collaborationBase_=submittedLocal_;submittedLocal_={};}
    if(collaborationSubmitted_)return; // The ordered acknowledgment includes earlier remote changes.
    if(simulationActive()){
        if(!collaborationQueued_)appendLog(text("LAN-Änderungen werden nach dem Stoppen der Simulation übernommen.","LAN changes will be applied after stopping the simulation."));collaborationQueued_=true;return;
    }
    if(collaborationConflict_)return;
    try{
        const bool initial=collaborationJoining_||collaborationBase_.isEmpty();
        if(initial){if(collaborationJoining_){projectFile_.clear();recovery_.clear();}collaborationJoining_=false;applyCollaboration(document);collaborationBase_=document;collaborationQueued_=false;return;}
        const auto local=CollaborationProject::encode(snapshot());const auto merged=ProjectMerge::merge(collaborationBase_,document,local);
        if(!merged.conflicts.isEmpty()){flagCollaborationConflict(merged.conflicts);return;}
        collaborationBase_=document;collaborationQueued_=false;if(merged.document!=local)applyCollaboration(merged.document);if(merged.document!=document)scheduleCollaboration();
    }catch(const std::exception& failure){appendLog(QString::fromUtf8(failure.what()));}
}
void MainWindow::applyCollaboration(const QJsonObject& document){
    if(simulationActive())return;QScopedValueRollback<bool> applying(collaborationApplying_,true);
    const auto previous=CollaborationProject::encode(snapshot());auto incoming=CollaborationProject::decode(document);const auto active=circuit_.boardId();
    const bool structural=previous.value("parts")!=document.value("parts")||previous.value("wires")!=document.value("wires")||previous.value("componentModels")!=document.value("componentModels");
    circuit_=std::move(incoming.circuit);if(!active.empty()&&std::any_of(circuit_.parts().begin(),circuit_.parts().end(),[&](const auto& part){return part.id==active&&part.kind==thinkercad::PartKind::Esp8266;}))circuit_.setActiveBoard(active);
    circuit_.setIdentityNamespace(collaboration_.identityNamespace().toStdString());boardSketches_=incoming.boardSketches;sketchBoard_=circuit_.boardId();const auto sketch=boardSketches_.value(QString::fromStdString(sketchBoard_),incoming.sketch);
    if(sketch_.order==sketch.order){
        for(int i=0;i<codeTabs_->count();++i){auto* editor=static_cast<CodeEditor*>(codeTabs_->widget(i));const auto file=editor->property("fileName").toString();const auto before=editor->toPlainText(),after=sketch.files.value(file);if(before!=after){int prefix=0;while(prefix<before.size()&&prefix<after.size()&&before[prefix]==after[prefix])++prefix;int end=before.size(),tail=after.size();while(end>prefix&&tail>prefix&&before[end-1]==after[tail-1]){--end;--tail;}QTextCursor cursor(editor->document());cursor.beginEditBlock();cursor.setPosition(prefix);cursor.setPosition(end,QTextCursor::KeepAnchor);cursor.insertText(after.mid(prefix,tail-prefix));cursor.endEditBlock();}editor->setBreakpoints(sketch.breakpoints.value(file));}sketch_=sketch;
    }else setSketch(sketch);
    executionMode_=incoming.mode;updateExecutionMode();if(structural){runtimes_.clear();canvas_->rebuild();canvas_->resetHistory();}inputDirty_=true;updateCircuit();refreshTitle();
}
void MainWindow::flagCollaborationConflict(const QStringList& paths){
    collaborationConflict_=true;collaborationDebounce_.stop();
    // Retain the actual local project, including local library references, before
    // offering either resolution. Recovery never runs code or overwrites a project.
    saveCollaborationDraft();
    collaborationTools_->showConflict(paths);if(auto* dock=findChild<QDockWidget*>("collaborationDock"))dock->show();
}
void MainWindow::resolveCollaboration(bool preferLocal){
    if(!collaboration_.connected()||!collaborationConflict_||simulationActive())return;
    try{const auto authority=collaboration_.document(),local=CollaborationProject::encode(snapshot());const auto resolved=preferLocal?ProjectMerge::merge(collaborationBase_,authority,local,true).document:authority;
        if(!saveCollaborationDraft())return;const auto failure=CollaborationProject::validate(resolved);if(!failure.isEmpty()){appendLog(failure);return;}collaborationConflict_=false;collaborationDraftPath_.clear();collaborationTools_->clearConflict();collaborationBase_=authority;applyCollaboration(resolved);if(preferLocal)scheduleCollaboration();
    }catch(const std::exception& failure){appendLog(QString::fromUtf8(failure.what()));}
}
void MainWindow::compareCollaboration(){
    if(!collaborationConflict_)return;QDialog dialog(this);dialog.setWindowTitle(text("Lokaler Entwurf / Host-Stand","Local draft / host version"));dialog.resize(900,600);auto* layout=new QVBoxLayout(&dialog);auto* tabs=new QTabWidget;layout->addWidget(tabs);
    auto add=[&](const QString& title,const QJsonObject& doc){auto* edit=new QPlainTextEdit;edit->setReadOnly(true);edit->setPlainText(QString::fromUtf8(QJsonDocument(doc).toJson()));tabs->addTab(edit,title);};
    try{add(text("Lokaler Entwurf","Local draft"),CollaborationProject::encode(snapshot()));add(text("Aktueller Host-Stand","Current host version"),collaboration_.document());}catch(const std::exception& failure){appendLog(QString::fromUtf8(failure.what()));}
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Close);layout->addWidget(buttons);connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);dialog.exec();
}

bool MainWindow::saveCollaborationDraft(){
    try{const auto directory=ProjectRecovery::directory()+"/collaboration-drafts";if(!QDir().mkpath(directory))throw std::runtime_error("Cannot create local conflict draft folder");
        if(collaborationDraftPath_.isEmpty())collaborationDraftPath_=QDir(directory).filePath(QDateTime::currentDateTimeUtc().toString("yyyyMMdd-HHmmsszzz")+'-'+QUuid::createUuid().toString(QUuid::Id128)+".thinkcad");
        ProjectIO::save(collaborationDraftPath_,snapshot());const auto drafts=QDir(directory).entryInfoList({"*.thinkcad"},QDir::Files,QDir::Time);for(int i=20;i<drafts.size();++i)QFile::remove(drafts[i].absoluteFilePath());appendLog(text("Lokaler Konfliktentwurf: ","Local conflict draft: ")+collaborationDraftPath_);return true;
    }catch(const std::exception& failure){appendLog(QString::fromUtf8(failure.what()));return false;}
}

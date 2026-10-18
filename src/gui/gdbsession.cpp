#include "gdbsession.hpp"
#include <QElapsedTimer>

GdbSession::GdbSession(QObject* parent):QObject(parent){
    connect(&process_,&QProcess::readyReadStandardOutput,this,&GdbSession::read);
    connect(&process_,&QProcess::readyReadStandardError,this,[this]{emit log(QString::fromUtf8(process_.readAllStandardError()));});
    connect(&process_,&QProcess::errorOccurred,this,[this](QProcess::ProcessError){if(!closing_)emit error(process_.errorString());});
    connect(&process_,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),this,[this]{pending_.clear();replies_.clear();inferiorPid_=0;stopped_=false;emit exited();});
}
GdbSession::~GdbSession(){stop();}
int GdbSession::command(const QByteArray& text,std::function<void(const GdbMi::Record&)> reply){
    if(!active()||text.contains('\n')||text.contains('\r')||text.size()>65536)return -1;
    if(replies_.size()>=256){emit error(QStringLiteral("Too many pending debugger commands"));return -1;}
    if(token_==2147483647)token_=0;const int id=++token_;
    if(reply)replies_.insert(id,std::move(reply));
    process_.write(QByteArray::number(id)+text+'\n');return id;
}
void GdbSession::start(const QString& debugger,const QString& executable,const QProcessEnvironment& environment,const QList<Breakpoint>& points){
    stop();closing_=false;token_=0;pending_.clear();replies_.clear();inferiorPid_=0;stopped_=false;
    auto env=environment;env.insert("DEBUGINFOD_URLS","");process_.setProcessEnvironment(env);
    process_.start(debugger,{"--nx","--nh","--interpreter=mi2"});
    if(!process_.waitForStarted(3000))return;
    command("-gdb-set pagination off");command("-gdb-set confirm off");command("-gdb-set mi-async on");
    // Never load scripts from the project or consult a symbol server.
    command("-gdb-set auto-load off");
    command("-file-exec-and-symbols "+GdbMi::quote(executable),[this,points](const auto& record){
        if(record.kind!="done")return;
        for(const auto& point:points)insertBreakpoint(point.file,point.line);
        // MI commands are processed in order; this barrier includes breakpoint setup.
        command("-list-features",[this](const auto& result){if(result.kind=="done")emit ready();});
    });
}
void GdbSession::stop(){
    if(!active())return;closing_=true;
    // GDB owns the inferior: kill it before exiting, including a busy sketch.
    command("-exec-interrupt --all");
    command("-interpreter-exec console \"kill\"");command("-gdb-exit");
    QElapsedTimer timer;timer.start();
    while(active()&&timer.elapsed()<1500){process_.waitForReadyRead(30);read();}
    if(active()){process_.kill();process_.waitForFinished(1000);}
    replies_.clear();pending_.clear();inferiorPid_=0;stopped_=false;
}
void GdbSession::run(){command("-exec-run");}
void GdbSession::resume(){command("-exec-continue");}
void GdbSession::interrupt(){command("-exec-interrupt --all");}
void GdbSession::next(){if(stopped_)command("-exec-next");}
void GdbSession::step(){if(stopped_)command("-exec-step");}
void GdbSession::finish(){if(stopped_)command("-exec-finish");}
void GdbSession::evaluate(const QString& expression){if(stopped_)command("-data-evaluate-expression "+GdbMi::quote(expression),[this,expression](const auto& r){if(r.kind=="done")emit evaluated(expression,r.values.value("value").toString());});}
void GdbSession::inspectVariables(){if(stopped_)command("-stack-list-variables --all-values",[this](const auto& r){if(r.kind=="done")emit variables(r.values.value("variables").toList());});}
void GdbSession::inspectRegisters(){
    if(!stopped_)return;
    command("-data-list-register-names",[this](const auto& names){if(names.kind!="done")return;const auto list=names.values.value("register-names").toList();
        command("-data-list-register-values x",[this,list](const auto& r){if(r.kind!="done")return;auto values=r.values.value("register-values").toList();for(auto& value:values){auto item=value.toMap();const int index=item.value("number").toString().toInt();if(index>=0&&index<list.size())item["name"]=list[index];value=item;}emit registers(values);});
    });
}
void GdbSession::insertBreakpoint(const QString& file,int line){
    if(line<1||line>1000000||file.isEmpty())return;
    command("-break-insert "+GdbMi::quote(file+":"+QString::number(line)),[this,file,line](const auto& r){if(r.kind=="done")emit breakpointInserted(r.values.value("bkpt").toMap().value("number").toString(),file,line);});
}
void GdbSession::removeBreakpoint(const QString& number){bool ok=false;const int id=number.toInt(&ok);if(ok&&id>0)command("-break-delete "+QByteArray::number(id));}
void GdbSession::read(){
    pending_+=process_.readAllStandardOutput();if(pending_.size()>1048576){pending_.clear();emit error(QStringLiteral("Debugger output exceeded its buffer limit"));process_.kill();return;}
    qsizetype end;while((end=pending_.indexOf('\n'))>=0){const auto line=pending_.left(end);pending_.remove(0,end+1);QString failure;auto record=GdbMi::parse(line,&failure);if(record)handle(*record);else if(!failure.isEmpty()){
        qsizetype at=0;while(at<line.size()&&line[at]>='0'&&line[at]<='9')++at;
        // Native C stdio may share GDB's pipe. Keep plain inferior diagnostics
        // separate from MI records; Arduino RPC uses its authenticated socket.
        if(line.size()<=262144&&at<line.size()&&!QByteArray("^*+=~@&").contains(line[at]))emit log(QString::fromUtf8(line));else emit error(failure);
    }}
}
void GdbSession::handle(const GdbMi::Record& r){
    if(r.type=='^'){
        const auto reply=replies_.take(r.token);if(reply)reply(r);
        if(r.kind=="error"&&!closing_)emit error(r.values.value("msg").toString());
    }else if(r.type=='~'||r.type=='@'||r.type=='&')emit log(r.text);
    else if(r.type=='='&&r.kind=="thread-group-started")inferiorPid_=r.values.value("pid").toString().toLongLong();
    else if(r.type=='*'&&r.kind=="running"){stopped_=false;emit running();}
    else if(r.type=='*'&&r.kind=="stopped"){
        const auto reason=r.values.value("reason").toString();
        if(reason.startsWith("exited")){stopped_=false;inferiorPid_=0;emit exited();return;}
        stopped_=true;const auto frame=r.values.value("frame").toMap();
        emit halted(frame.value("fullname",frame.value("file")).toString(),frame.value("line").toString().toInt(),reason);
    }
}

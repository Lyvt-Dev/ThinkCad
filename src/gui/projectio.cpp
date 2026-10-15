#include "projectio.hpp"
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSaveFile>
#include <stdexcept>
#include <algorithm>
namespace {
QString read(const QString& path){QFile f(path);if(!f.open(QIODevice::ReadOnly)||f.size()>2*1024*1024)throw std::runtime_error("Datei nicht lesbar oder zu groß.");return QString::fromUtf8(f.readAll());}
QString maskCpp(const QString& source){
    auto mask=source;auto blank=[&](int start,int end){for(int k=start;k<end;++k)if(mask[k]!='\n')mask[k]=' ';};
    for(int i=0;i<source.size();){
        if(source.mid(i,2)=="//"){int e=source.indexOf('\n',i);if(e<0)e=source.size();blank(i,e);i=e;}
        else if(source.mid(i,2)=="/*"){int e=source.indexOf("*/",i+2);e=e<0?source.size():e+2;blank(i,e);i=e;}
        else if(source.mid(i,2)=="R\""){const int paren=source.indexOf('(',i+2);if(paren<0||paren-i>18){++i;continue;}const auto endToken=")"+source.mid(i+2,paren-i-2)+"\"";int e=source.indexOf(endToken,paren+1);e=e<0?source.size():e+endToken.size();blank(i,e);i=e;}
        else if(source[i]=='\"'||source[i]=='\''){const auto quote=source[i];int e=i+1;while(e<source.size()){if(source[e]=='\\'){e+=2;continue;}if(source[e++]==quote)break;}e=std::min<int>(e,source.size());blank(i,e);i=e;}
        else ++i;
    }
    int line=0;while(line<mask.size()){int end=mask.indexOf('\n',line);if(end<0)end=mask.size();if(mask.mid(line,end-line).trimmed().startsWith('#'))blank(line,end);line=end+1;}
    return mask;
}
QString prototypes(QString source){
    const auto mask=maskCpp(source);int depth=0,start=0,first=-1;QStringList declarations;
    const QRegularExpression function("^\\s*([\\w:\\s*&<>]+?)\\b([A-Za-z_]\\w*)\\s*\\(([^{}]*)\\)\\s*$");
    for(int i=0;i<mask.size();++i){
        if(mask[i]=='{'){
            if(depth==0){const auto prefix=mask.mid(start,i-start);auto match=function.match(prefix);if(match.hasMatch()&&!prefix.contains('=')&&!prefix.contains("template")&&!prefix.contains("namespace")){
                if(first<0){first=start;while(first<i&&mask[first].isSpace())++first;}
                const auto signature=match.captured(1).trimmed()+" "+match.captured(2)+"("+match.captured(3)+");";
                if(!declarations.contains(signature))declarations<<signature;
            }}++depth;
        }else if(mask[i]=='}'){depth=std::max(0,depth-1);if(depth==0)start=i+1;}
        else if(mask[i]==';'&&depth==0)start=i+1;
    }
    if(first>=0){int line=source.left(first).count('\n')+1;source.insert(first,"\n// Arduino-style declarations for plain global functions.\n"+declarations.join('\n')+"\n#line "+QString::number(line)+" \"combined-sketch.ino\"\n");}
    return source;
}
QJsonObject endpoint(const thinkercad::Endpoint& ep){return {{"part",QString::fromStdString(ep.part)},{"pin",QString::fromStdString(ep.pin)}};}
thinkercad::Endpoint endpoint(const QJsonObject& o){return {o["part"].toString().toStdString(),o["pin"].toString().toStdString()};}
}
bool ProjectIO::validFileName(const QString& name){return !name.isEmpty()&&name.size()<240&&!name.startsWith('/')&&!name.contains('\\')&&!name.contains(':')&&QDir::cleanPath(name)==name&&!name.split('/').contains("..")&&QStringList{"ino","h","hpp","cpp","cc","cxx"}.contains(QFileInfo(name).suffix().toLower());}
SketchProject SketchProject::openIno(const QString& path){
    QFileInfo file(path);if(file.suffix().toLower()!="ino"||!file.exists())throw std::runtime_error("Eine bestehende .ino-Datei auswählen.");
    SketchProject p;p.sourceFolder=file.absolutePath();p.name=file.completeBaseName();QDir dir(p.sourceFolder);
    auto tabs=dir.entryList({"*.ino"},QDir::Files,QDir::Name);const auto primary=dir.dirName()+".ino";const auto first=tabs.contains(primary)?primary:file.fileName();tabs.removeAll(first);tabs.prepend(first);
    qsizetype total=0;
    auto add=[&](const QString& name){if(!ProjectIO::validFileName(name)||p.files.size()>=100)throw std::runtime_error("Ungültiges oder zu großes Arduino-Projekt.");auto contents=read(dir.filePath(name));total+=contents.toUtf8().size();if(total>2*1024*1024)throw std::runtime_error("Arduino-Projekt ist größer als 2 MiB.");p.files[name]=contents;p.order<<name;};
    for(const auto& tab:tabs)add(tab);
    for(const auto& name:dir.entryList({"*.h","*.hpp","*.cpp","*.cc","*.cxx"},QDir::Files,QDir::Name))add(name);
    for(const auto& sub:QStringList{"src","include"}){QDirIterator files(dir.filePath(sub),{"*.h","*.hpp","*.cpp","*.cc","*.cxx"},QDir::Files,QDirIterator::Subdirectories);while(files.hasNext()){auto full=files.next();if(!QFileInfo(full).isSymLink())add(dir.relativeFilePath(full));}}
    return p;
}
QString SketchProject::combinedSource()const{QString combined="#include <Arduino.h>\n";for(const auto& name:order)if(name.endsWith(".ino",Qt::CaseInsensitive)){auto escaped=name;escaped.replace('\\',"\\\\").replace('\"',"\\\"");combined+="#line 1 \""+escaped+"\"\n"+files.value(name)+"\n";}return prototypes(combined);}
QMap<QString,QString> SketchProject::auxiliaryFiles()const{auto aux=files;for(auto it=aux.begin();it!=aux.end();)if(it.key().endsWith(".ino",Qt::CaseInsensitive))it=aux.erase(it);else++it;return aux;}
void ProjectIO::save(const QString& path,const SavedProject& p){
    p.circuit.validate();QJsonArray parts,wires,files;
    for(const auto& part:p.circuit.parts())parts.append(QJsonObject{{"id",QString::fromStdString(part.id)},{"kind",static_cast<int>(part.kind)},{"x",part.x},{"y",part.y},{"value",part.value},{"color",QString::fromStdString(part.color)},{"model",QString::fromStdString(part.model)},{"state",part.kind==thinkercad::PartKind::Button?false:part.pressed}});
    for(const auto& wire:p.circuit.wires()){QJsonArray bends;for(const auto& point:wire.points)bends.append(QJsonArray{point.x,point.y});wires.append(QJsonObject{{"a",endpoint(wire.a)},{"b",endpoint(wire.b)},{"color",QString::fromStdString(wire.color)},{"points",bends}});}
    for(const auto& name:p.sketch.order){if(!validFileName(name))throw std::runtime_error("Invalid project file name");files.append(QJsonObject{{"name",name},{"text",p.sketch.files.value(name)}});}
    const auto data=QJsonDocument(QJsonObject{{"format","ThinkCad"},{"version",1},{"name",p.sketch.name},{"parts",parts},{"wires",wires},{"files",files}}).toJson();
    if(data.size()>3*1024*1024)throw std::runtime_error("Projekt ist größer als 3 MiB.");QSaveFile file(path);if(!file.open(QIODevice::WriteOnly)||file.write(data)!=data.size()||!file.commit())throw std::runtime_error("Projekt konnte nicht gespeichert werden.");
}
SavedProject ProjectIO::load(const QString& path){
    QFile file(path);if(!file.open(QIODevice::ReadOnly)||file.size()>3*1024*1024)throw std::runtime_error("Projektdatei ist nicht lesbar oder zu groß.");QJsonParseError error;const auto json=QJsonDocument::fromJson(file.readAll(),&error);const auto o=json.object();if(error.error!=QJsonParseError::NoError||(o["format"].toString()!="ThinkCad"&&o["format"].toString()!="ThinkerCad")||o["version"].toInt()!=1||!o["parts"].isArray()||!o["wires"].isArray()||!o["files"].isArray())throw std::runtime_error("Ungültiges ThinkCad-Projektformat.");
    SavedProject p;p.sketch.name=o["name"].toString("sketch");bool board=false;
    for(const auto& entry:o["parts"].toArray()){auto v=entry.toObject();if(!v["x"].isDouble()||!v["y"].isDouble()||!v["value"].isDouble()||!v["kind"].isDouble())throw std::runtime_error("Invalid component properties");thinkercad::CircuitPart part{v["id"].toString().toStdString(),static_cast<thinkercad::PartKind>(v["kind"].toInt(-1)),v["x"].toDouble(),v["y"].toDouble(),v["value"].toDouble(),v["state"].toBool(),v["color"].toString("#fb586b").toStdString(),v["model"].toString().toStdString()};if(part.id=="board"){if(board||part.kind!=thinkercad::PartKind::Esp8266)throw std::runtime_error("Invalid primary board");p.circuit.part("board")=part;board=true;}else p.circuit.insertPart(part);}
    if(!board)throw std::runtime_error("ESP8266 fehlt im Projekt.");
    for(const auto& entry:o["wires"].toArray()){auto v=entry.toObject();p.circuit.connect(endpoint(v["a"].toObject()),endpoint(v["b"].toObject()),v["color"].toString("#0d9488").toStdString());if(v.contains("points")){if(!v["points"].isArray()||v["points"].toArray().size()>64)throw std::runtime_error("Invalid wire bends");auto& wire=const_cast<thinkercad::CircuitWire&>(p.circuit.wires().back());for(const auto& value:v["points"].toArray()){auto xy=value.toArray();if(xy.size()!=2||!xy[0].isDouble()||!xy[1].isDouble())throw std::runtime_error("Invalid wire bend");wire.points.push_back({xy[0].toDouble(),xy[1].toDouble()});}}}
    for(const auto& entry:o["files"].toArray()){auto v=entry.toObject();const auto name=v["name"].toString();if(!validFileName(name)||p.sketch.files.contains(name)||p.sketch.files.size()>=100||!v["text"].isString())throw std::runtime_error("Invalid sketch files");p.sketch.order<<name;p.sketch.files[name]=v["text"].toString();}
    if(p.sketch.files.isEmpty()||std::none_of(p.sketch.order.begin(),p.sketch.order.end(),[](const auto& n){return n.endsWith(".ino",Qt::CaseInsensitive);}))throw std::runtime_error("Projekt enthält keinen Arduino-Sketch.");p.circuit.validate();return p;
}

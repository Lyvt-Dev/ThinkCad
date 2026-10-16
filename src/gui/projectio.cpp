#include "projectio.hpp"
#include "modulepackages.hpp"
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
#include <QCryptographicHash>
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
QByteArray ProjectIO::serialize(const SavedProject& p){
    p.circuit.validate();if(p.mode!=thinkercad::ExecutionMode::Simulation&&p.mode!=thinkercad::ExecutionMode::Emulation)throw std::runtime_error("Invalid project execution mode");QJsonArray parts,wires,files;
    for(const auto& part:p.circuit.parts()){QJsonObject params;for(const auto& [key,value]:part.parameters)params[QString::fromStdString(key)]=value;parts.append(QJsonObject{{"netName",QString::fromStdString(part.netName)},{"parameters",params},{"id",QString::fromStdString(part.id)},{"kind",static_cast<int>(part.kind)},{"x",part.x},{"y",part.y},{"value",part.value},{"color",QString::fromStdString(part.color)},{"model",QString::fromStdString(part.model)},{"rotation",part.rotation},{"state",part.kind==thinkercad::PartKind::Button?false:part.pressed}});}
    for(const auto& wire:p.circuit.wires()){QJsonArray bends;for(const auto& point:wire.points)bends.append(QJsonArray{point.x,point.y});wires.append(QJsonObject{{"a",endpoint(wire.a)},{"b",endpoint(wire.b)},{"color",QString::fromStdString(wire.color)},{"points",bends}});}
    for(const auto& name:p.sketch.order){if(!validFileName(name))throw std::runtime_error("Invalid project file name");files.append(QJsonObject{{"name",name},{"text",p.sketch.files.value(name)}});}
    QJsonObject sketches;
    for(auto it=p.boardSketches.cbegin();it!=p.boardSketches.cend();++it){
        if(p.circuit.part(it.key().toStdString()).kind!=thinkercad::PartKind::Esp8266)throw std::runtime_error("Sketch is not assigned to a board");
        QJsonArray boardFiles;for(const auto& name:it->order){if(!validFileName(name)||!it->files.contains(name))throw std::runtime_error("Invalid board sketch file");boardFiles.append(QJsonObject{{"name",name},{"text",it->files.value(name)}});}
        sketches[it.key()]=QJsonObject{{"name",it->name},{"files",boardFiles}};
    }
    QJsonObject bundled;for(auto it=p.bundledLibraries.cbegin();it!=p.bundledLibraries.cend();++it)bundled[it.key()]=QJsonObject{{"data",QString::fromLatin1(it.value().toBase64())},{"sha256",QString::fromLatin1(QCryptographicHash::hash(it.value(),QCryptographicHash::Sha256).toHex())}};
    const auto data=QJsonDocument(QJsonObject{{"libraries",bundled},{"boardSketches",sketches},{"format","ThinkCad"},{"version",4},{"executionMode",QString::fromLatin1(thinkercad::executionModeName(p.mode))},{"componentModels",ModulePackages::usedDefinitions(p.circuit)},{"name",p.sketch.name},{"activeBoard",QString::fromStdString(p.circuit.boardId())},{"parts",parts},{"wires",wires},{"files",files}}).toJson();
    if(data.size()>32*1024*1024)throw std::runtime_error("Projekt ist größer als 32 MiB.");return data;
}
void ProjectIO::save(const QString& path,const SavedProject& p){const auto data=serialize(p);QSaveFile file(path);if(!file.open(QIODevice::WriteOnly)||file.write(data)!=data.size()||!file.commit())throw std::runtime_error("Projekt konnte nicht gespeichert werden.");
}
SavedProject ProjectIO::load(const QString& path){
    QFile file(path);if(!file.open(QIODevice::ReadOnly)||file.size()>32*1024*1024)throw std::runtime_error("Projektdatei ist nicht lesbar oder zu groß.");return deserialize(file.readAll());
}
SavedProject ProjectIO::deserialize(const QByteArray& data){
    if(data.size()>32*1024*1024)throw std::runtime_error("Project exceeds 32 MiB");QJsonParseError error;const auto json=QJsonDocument::fromJson(data,&error);const auto o=json.object();if(error.error!=QJsonParseError::NoError||(o["format"].toString()!="ThinkCad"&&o["format"].toString()!="ThinkerCad")||(o["version"].toInt()!=1&&o["version"].toInt()!=2&&o["version"].toInt()!=3&&o["version"].toInt()!=4)||!o["parts"].isArray()||!o["wires"].isArray()||!o["files"].isArray())throw std::runtime_error("Ungültiges ThinkCad-Projektformat.");
    QStringList installed;bool accepted=false;struct Rollback{QStringList& ids;bool& accepted;~Rollback(){if(!accepted)for(const auto& id:ids)thinkercad::ModuleRegistry::removeLocal(id.toStdString());}} rollback{installed,accepted};
    SavedProject p;
    if(o["version"].toInt()>=4){const auto mode=o["executionMode"].toString("simulation");if((o.contains("executionMode")&&!o["executionMode"].isString())||(mode!="simulation"&&mode!="emulation"))throw std::runtime_error("Invalid project execution mode");p.mode=mode=="emulation"?thinkercad::ExecutionMode::Emulation:thinkercad::ExecutionMode::Simulation;if(o.contains("componentModels")){if(!o["componentModels"].isArray())throw std::runtime_error("Invalid embedded component models");installed=ModulePackages::installEmbedded(o["componentModels"].toArray());}}
    p.circuit.removePart("board");p.sketch.name=o["name"].toString("sketch");
    for(const auto& entry:o["parts"].toArray()){auto v=entry.toObject();if(!v["x"].isDouble()||!v["y"].isDouble()||!v["value"].isDouble()||!v["kind"].isDouble())throw std::runtime_error("Invalid component properties");thinkercad::CircuitPart part{v["id"].toString().toStdString(),static_cast<thinkercad::PartKind>(v["kind"].toInt(-1)),v["x"].toDouble(),v["y"].toDouble(),v["value"].toDouble(),v["state"].toBool(),v["color"].toString("#fb586b").toStdString(),v["model"].toString().toStdString()};if(v.contains("rotation")&&(!v["rotation"].isDouble()||v["rotation"].toDouble()!=v["rotation"].toInt(-1)))throw std::runtime_error("Invalid component rotation");part.rotation=v["rotation"].toInt();part.netName=v["netName"].toString().toStdString();if(v.contains("parameters")){if(!v["parameters"].isObject())throw std::runtime_error("Invalid component parameters");const auto params=v["parameters"].toObject();for(auto it=params.begin();it!=params.end();++it){if(!it.value().isDouble())throw std::runtime_error("Invalid parameter value");part.parameters[it.key().toStdString()]=it.value().toDouble();}}p.circuit.insertPart(part);}
    if(!o["activeBoard"].toString().isEmpty())p.circuit.setActiveBoard(o["activeBoard"].toString().toStdString());
    for(const auto& entry:o["wires"].toArray()){auto v=entry.toObject();p.circuit.connect(endpoint(v["a"].toObject()),endpoint(v["b"].toObject()),v["color"].toString("#0d9488").toStdString());if(v.contains("points")){if(!v["points"].isArray()||v["points"].toArray().size()>64)throw std::runtime_error("Invalid wire bends");auto& wire=const_cast<thinkercad::CircuitWire&>(p.circuit.wires().back());for(const auto& value:v["points"].toArray()){auto xy=value.toArray();if(xy.size()!=2||!xy[0].isDouble()||!xy[1].isDouble())throw std::runtime_error("Invalid wire bend");wire.points.push_back({xy[0].toDouble(),xy[1].toDouble()});}}}
    for(const auto& entry:o["files"].toArray()){auto v=entry.toObject();const auto name=v["name"].toString();if(!validFileName(name)||p.sketch.files.contains(name)||p.sketch.files.size()>=100||!v["text"].isString())throw std::runtime_error("Invalid sketch files");p.sketch.order<<name;p.sketch.files[name]=v["text"].toString();}
    if(p.sketch.files.isEmpty()||std::none_of(p.sketch.order.begin(),p.sketch.order.end(),[](const auto& n){return n.endsWith(".ino",Qt::CaseInsensitive);}))throw std::runtime_error("Projekt enthält keinen Arduino-Sketch.");if(o["version"].toInt()>=2){
        if(!o["boardSketches"].isObject())throw std::runtime_error("Invalid board sketches");
        const auto sketches=o["boardSketches"].toObject();
        for(auto it=sketches.constBegin();it!=sketches.constEnd();++it){
            if(p.circuit.part(it.key().toStdString()).kind!=thinkercad::PartKind::Esp8266||!it.value().isObject())throw std::runtime_error("Invalid sketch board");
            auto object=it.value().toObject();SketchProject sketch;sketch.name=object["name"].toString("sketch");
            if(!object["files"].isArray())throw std::runtime_error("Invalid board files");
            for(const auto& entry:object["files"].toArray()){const auto file=entry.toObject();const auto name=file["name"].toString();if(!validFileName(name)||sketch.files.contains(name)||sketch.files.size()>=100||!file["text"].isString())throw std::runtime_error("Invalid board sketch files");sketch.order<<name;sketch.files[name]=file["text"].toString();}
            if(std::none_of(sketch.order.cbegin(),sketch.order.cend(),[](const auto& name){return name.endsWith(".ino",Qt::CaseInsensitive);}))throw std::runtime_error("Board has no Arduino sketch");
            p.boardSketches[it.key()]=sketch;
        }
    }
    if(o.contains("libraries")){if(!o["libraries"].isObject())throw std::runtime_error("Invalid library bundle");auto bundle=o["libraries"].toObject();if(bundle.size()>2000)throw std::runtime_error("Too many bundled files");qint64 total=0;for(auto it=bundle.begin();it!=bundle.end();++it){const auto name=it.key();if(!QRegularExpression("^[A-Za-z0-9_. /+-]+$").match(name).hasMatch()||name.startsWith('/')||name.split('/').contains("..")||name.split('/').contains(".")||name.split('/').contains("")||!name.contains('/'))throw std::runtime_error("Unsafe bundled library path");auto item=it.value().toObject();auto decoded=QByteArray::fromBase64Encoding(item["data"].toString().toLatin1(),QByteArray::AbortOnBase64DecodingErrors);if(!decoded)throw std::runtime_error("Corrupt library data");auto bytes=decoded.decoded;total+=bytes.size();if(total>20*1024*1024||QCryptographicHash::hash(bytes,QCryptographicHash::Sha256).toHex()!=item["sha256"].toString().toLatin1())throw std::runtime_error("Library checksum/size mismatch");p.bundledLibraries[name]=bytes;}}
    p.circuit.validate();accepted=true;return p;
}

QMap<QString,QByteArray> ProjectIO::bundleLibraries(const QStringList& directories){QMap<QString,QByteArray> files;qint64 total=0;int library=0;for(const auto& directory:directories){QDir root(directory);if(!root.exists())throw std::runtime_error("Library directory missing");QString prefix="library"+QString::number(++library);QDirIterator it(directory,QDir::Files|QDir::NoSymLinks,QDirIterator::Subdirectories);while(it.hasNext()){auto path=it.next();auto relative=root.relativeFilePath(path);if(relative.startsWith(".git/")||relative.startsWith("build/"))continue;auto name=prefix+"/"+relative;if(!QRegularExpression("^[A-Za-z0-9_. /+-]+$").match(name).hasMatch())throw std::runtime_error("Unsupported library filename");QFile file(path);if(!file.open(QIODevice::ReadOnly)||file.size()>4*1024*1024)throw std::runtime_error("Library file unreadable/too large");auto data=file.readAll();total+=data.size();if(total>20*1024*1024||files.size()>=2000)throw std::runtime_error("Library bundle exceeds 20 MiB / 2000 files");files[name]=data;}}return files;}
QStringList ProjectIO::extractLibraries(const QMap<QString,QByteArray>& files,const QString& destination){QStringList roots;for(auto it=files.cbegin();it!=files.cend();++it){auto name=it.key();if(name.startsWith('/')||name.contains('\\')||name.contains(':')||name.split('/').contains("..")||name.split('/').contains(".")||name.split('/').contains("")||!name.contains('/'))throw std::runtime_error("Unsafe library path");auto path=QDir(destination).filePath(name);QDir().mkpath(QFileInfo(path).absolutePath());QSaveFile file(path);if(!file.open(QIODevice::WriteOnly)||file.write(it.value())!=it.value().size()||!file.commit())throw std::runtime_error("Cannot extract bundled library");auto root=QDir(destination).filePath(name.section('/',0,0));if(!roots.contains(root))roots<<root;}return roots;}

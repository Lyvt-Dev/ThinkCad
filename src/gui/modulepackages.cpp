#include "modulepackages.hpp"
#include "thinkercad/circuit.hpp"
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QSaveFile>
#include <QStandardPaths>
#include <QSettings>
#include <QCryptographicHash>
#include <set>
#include <cmath>
#include <stdexcept>
using namespace thinkercad;
namespace {
void check(bool condition,const char* message){if(!condition)throw std::invalid_argument(message);}
QByteArray canonical(const ModuleDefinition& d){return QJsonDocument(ModulePackages::encode(d)).toJson(QJsonDocument::Compact);}
QJsonArray package(const QByteArray& bytes){
    check(bytes.size()<=512*1024,"Component package exceeds 512 KiB");QJsonParseError error;const auto doc=QJsonDocument::fromJson(bytes,&error);const auto root=doc.object();
    check(error.error==QJsonParseError::NoError&&root["format"]=="ThinkCad.Components"&&root["version"]==1&&root["models"].isArray(),"Invalid component package format");
    for(auto it=root.begin();it!=root.end();++it)check(it.key()=="format"||it.key()=="version"||it.key()=="models","Unknown component package field");
    const auto models=root["models"].toArray();check(!models.isEmpty()&&models.size()<=32,"A component package must contain 1..32 models");return models;
}
}
QString ModulePackages::directory(){return QSettings().value("componentDirectory",QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)+"/components").toString();}
QJsonObject ModulePackages::encode(const ModuleDefinition& d){
    QJsonArray pins,params,ro;QJsonObject registers,positions;for(const auto& [pin,point]:d.pinPositions)positions[QString::fromStdString(pin)]=QJsonArray{point.first,point.second};for(const auto& pin:d.pins)pins.append(QString::fromStdString(pin));for(const auto& p:d.parameters)params.append(QJsonObject{{"name",QString::fromStdString(p.name)},{"label",QString::fromStdString(p.label)},{"unit",QString::fromStdString(p.unit)},{"default",p.initial},{"min",p.min},{"max",p.max},{"register",p.registerAddress},{"bytes",p.bytes},{"scale",p.scale},{"littleEndian",p.littleEndian},{"signed",p.signedValue}});for(const auto& [reg,value]:d.registers)registers[QString::number(reg)]=int(value);for(auto reg:d.readOnly)ro.append(int(reg));
    return {{"id",QString::fromStdString(d.id)},{"label",QString::fromStdString(d.label)},{"bus",QString::fromStdString(d.bus)},{"implementation",QString::fromStdString(d.implementation)},{"power",QString::fromStdString(d.power)},{"ground",QString::fromStdString(d.ground)},{"pins",pins},{"pinPositions",positions},{"parameters",params},{"widthMm",d.width},{"heightMm",d.height},{"minVoltage",d.minVoltage},{"maxVoltage",d.maxVoltage},{"address",d.address},{"spiMode",d.spiMode},{"maxClock",int(d.maxClock)},{"registers",registers},{"readOnly",ro}};
}
ModuleDefinition ModulePackages::decode(const QJsonObject& o){
    const std::set<QString> keys{"id","label","bus","implementation","power","ground","pins","pinPositions","parameters","widthMm","heightMm","minVoltage","maxVoltage","address","spiMode","maxClock","registers","readOnly"};
    for(auto it=o.begin();it!=o.end();++it)check(keys.contains(it.key()),"Unknown component descriptor field");
    auto str=[&](const char* key,const QString& fallback=QString()){check(!o.contains(key)||o[key].isString(),"Component text field is not a string");return o[key].toString(fallback).toStdString();};
    auto num=[&](const char* key,double fallback){check(!o.contains(key)||o[key].isDouble(),"Component numeric field is not a number");return o[key].toDouble(fallback);};
    auto integer=[&](const char* key,int fallback){const auto n=num(key,fallback);check(std::isfinite(n)&&n==std::floor(n)&&n>=-1&&n<=80000000,"Invalid component integer");return int(n);};
    ModuleDefinition d;d.id=str("id");d.label=str("label");d.bus=str("bus");d.implementation=str("implementation","registers");d.power=str("power","VCC");d.ground=str("ground","GND");check(d.implementation=="registers"&&(d.bus=="i2c"||d.bus=="spi"),"Local component packages support data-only I2C/SPI register models");
    check(o["pins"].isArray()&&(!o.contains("parameters")||o["parameters"].isArray())&&(!o.contains("registers")||o["registers"].isObject())&&(!o.contains("readOnly")||o["readOnly"].isArray()),"Invalid component collections");
    for(const auto& value:o["pins"].toArray()){check(value.isString(),"Invalid component pin");d.pins.push_back(value.toString().toStdString());}
    if(o.contains("pinPositions")){check(o["pinPositions"].isObject(),"Invalid pin positions");const auto positions=o["pinPositions"].toObject();for(auto it=positions.begin();it!=positions.end();++it){const auto point=it.value().toArray();check(it.value().isArray()&&point.size()==2&&point[0].isDouble()&&point[1].isDouble(),"Invalid module pin coordinates");d.pinPositions[it.key().toStdString()]={point[0].toDouble(),point[1].toDouble()};}}
    d.width=num("widthMm",18);d.height=num("heightMm",24);d.minVoltage=num("minVoltage",3);d.maxVoltage=num("maxVoltage",3.6);d.address=integer("address",0);d.spiMode=integer("spiMode",0);d.maxClock=integer("maxClock",400000);
    const auto regs=o["registers"].toObject();for(auto it=regs.begin();it!=regs.end();++it){bool ok=false;const auto address=it.key().toUInt(&ok);const auto v=it.value().toDouble(-1);check(ok&&address<=255&&it.value().isDouble()&&v>=0&&v<=255&&v==std::floor(v),"Invalid component register");d.registers[address]=unsigned(v);}
    for(const auto& reg:o["readOnly"].toArray()){const auto n=reg.toDouble(-1);check(reg.isDouble()&&n>=0&&n<=255&&n==std::floor(n),"Invalid read-only register");d.readOnly.push_back(unsigned(n));}
    for(const auto& value:o["parameters"].toArray()){
        check(value.isObject(),"Invalid component parameter");const auto p=value.toObject();const std::set<QString> allowed{"name","label","unit","default","min","max","register","bytes","scale","littleEndian","signed"};for(auto it=p.begin();it!=p.end();++it)check(allowed.contains(it.key()),"Unknown component parameter field");
        check(p["name"].isString()&&p["label"].isString()&&(!p.contains("unit")||p["unit"].isString())&&p["default"].isDouble()&&p["min"].isDouble()&&p["max"].isDouble(),"Invalid parameter fields");
        for(const auto* key:{"register","bytes","scale"})check(!p.contains(key)||p[key].isDouble(),"Invalid parameter encoding");for(const auto* key:{"littleEndian","signed"})check(!p.contains(key)||p[key].isBool(),"Invalid parameter encoding flag");
        ModuleParameter field{p["name"].toString().toStdString(),p["label"].toString().toStdString(),p["unit"].toString().toStdString(),p["default"].toDouble(),p["min"].toDouble(),p["max"].toDouble()};
        const auto reg=p["register"].toDouble(-1),bytes=p["bytes"].toDouble(1);check(std::isfinite(reg)&&reg>=-1&&reg<=255&&reg==std::floor(reg)&&std::isfinite(bytes)&&bytes>=1&&bytes<=4&&bytes==std::floor(bytes),"Invalid parameter register or width");field.registerAddress=int(reg);field.bytes=int(bytes);field.scale=p["scale"].toDouble(1);field.littleEndian=p["littleEndian"].toBool();field.signedValue=p["signed"].toBool();d.parameters.push_back(field);
    }
    ModuleRegistry::validate(d);return d;
}
QStringList ModulePackages::installEmbedded(const QJsonArray& entries){
    check(entries.size()<=64,"Maximum 64 embedded component descriptors");std::vector<ModuleDefinition> pending;std::set<std::string> ids;
    for(const auto& entry:entries){check(entry.isObject(),"Invalid embedded component descriptor");auto d=decode(entry.toObject());check(ids.insert(d.id).second,"Duplicate component descriptor");const auto found=ModuleRegistry::definitions().find(d.id);if(found!=ModuleRegistry::definitions().end()){check(!found->second.builtin&&canonical(found->second)==canonical(d),"Component model conflicts with an installed model");}else pending.push_back(std::move(d));}
    check(ModuleRegistry::definitions().size()+pending.size()<=256,"Maximum 256 component models");QStringList added;for(auto& d:pending){const auto id=QString::fromStdString(d.id);ModuleRegistry::install(std::move(d));added<<id;}return added;
}
QStringList ModulePackages::importFile(const QString& path){
    QFile file(path);check(!QFileInfo(path).isSymLink()&&file.open(QIODevice::ReadOnly)&&file.size()<=512*1024,"Component package is unreadable, linked or too large");const auto data=file.readAll();const auto models=package(data);const auto added=installEmbedded(models);
    try{check(QDir().mkpath(directory()),"Could not create local component directory");const auto name=QString::fromLatin1(QCryptographicHash::hash(data,QCryptographicHash::Sha256).toHex())+".thinkcad-components.json";QSaveFile target(directory()+"/"+name);check(target.open(QIODevice::WriteOnly)&&target.write(data)==data.size()&&target.commit(),"Could not install component package");}
    catch(...){for(const auto& id:added)ModuleRegistry::removeLocal(id.toStdString());throw;}return added;
}
QStringList ModulePackages::loadInstalled(){
    QStringList errors;const auto root=QDir(directory());for(const auto& name:root.entryList({"*.thinkcad-components.json"},QDir::Files,QDir::Name)){try{QFile file(root.filePath(name));check(!QFileInfo(file).isSymLink()&&file.open(QIODevice::ReadOnly)&&file.size()<=512*1024,"Invalid local component file");installEmbedded(package(file.readAll()));}catch(const std::exception& error){errors<<name+": "+QString::fromUtf8(error.what());}}return errors;
}
QStringList ModulePackages::importRepository(const QString& path){
    const auto root=QFileInfo(path);check(root.isDir()&&!root.isSymLink(),"Choose a local folder or a checked-out local Git repository");QStringList added;QDirIterator files(root.absoluteFilePath(),{"*.thinkcad-components.json"},QDir::Files,QDirIterator::Subdirectories);int count=0;while(files.hasNext()){const auto name=files.next();check(++count<=64,"Maximum 64 component packages per repository");if(!QFileInfo(name).isSymLink())added<<importFile(name);}check(count>0,"No .thinkcad-components.json packages found");return added;
}
QJsonArray ModulePackages::usedDefinitions(const CircuitDocument& doc){QJsonArray result;std::set<std::string> seen;for(const auto& part:doc.parts())if(part.kind==PartKind::Module&&seen.insert(part.model).second){const auto& model=ModuleRegistry::get(part.model);if(!model.builtin)result.append(encode(model));}return result;}

#include <QDialog>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include "language.hpp"
bool ModulePackages::manage(QWidget* parent){
    QDialog dialog(parent);dialog.setObjectName("componentPackagesDialog");dialog.setWindowTitle(Language::text("Lokale Bauteile und Plugins"));dialog.resize(760,520);QVBoxLayout root(&dialog);QLabel intro(Language::text("Datenpakete aus lokalen Dateien oder Git-Arbeitskopien. Alle Bauteile bleiben auf diesem Computer; es werden keine Plugins ausgeführt."));intro.setWordWrap(true);root.addWidget(&intro);QTableWidget table;table.setObjectName("localComponentModels");table.setColumnCount(4);table.setHorizontalHeaderLabels({Language::text("Bauteil"),"Bus",Language::text("Pins"),Language::text("Quelle")});table.horizontalHeader()->setSectionResizeMode(0,QHeaderView::Stretch);table.setEditTriggers(QAbstractItemView::NoEditTriggers);root.addWidget(&table,1);bool changed=false;
    auto refresh=[&]{table.setRowCount(int(ModuleRegistry::definitions().size()));int row=0;for(const auto& [id,d]:ModuleRegistry::definitions()){const QStringList values{QString::fromStdString(d.label),QString::fromStdString(d.bus),QString::number(d.pins.size()),Language::text(d.builtin?"Integriert":"Lokal")};for(int col=0;col<4;++col)table.setItem(row,col,new QTableWidgetItem(values[col]));table.item(row,0)->setToolTip(QString::fromStdString(id));++row;}};refresh();QHBoxLayout actions;root.addLayout(&actions);QPushButton file(Language::text("Paket importieren…")),repository(Language::text("Lokales Repository…")),folder(Language::text("Bauteilordner öffnen")),sample(Language::text("Beispielpaket speichern…"));for(auto* button:{&file,&repository,&folder,&sample})actions.addWidget(button);
    auto run=[&](const QString& path,bool repo){if(path.isEmpty())return;try{const auto ids=repo?importRepository(path):importFile(path);changed|=!ids.isEmpty();refresh();}catch(const std::exception& error){QMessageBox::warning(&dialog,dialog.windowTitle(),QString::fromUtf8(error.what()));}};
    QObject::connect(&file,&QPushButton::clicked,&dialog,[&]{run(QFileDialog::getOpenFileName(&dialog,Language::text("Bauteilpaket auswählen"),{},"ThinkCad Components (*.thinkcad-components.json)"),false);});QObject::connect(&repository,&QPushButton::clicked,&dialog,[&]{run(QFileDialog::getExistingDirectory(&dialog,Language::text("Lokalen Bauteilordner auswählen")),true);});QObject::connect(&folder,&QPushButton::clicked,&dialog,[&]{QDir().mkpath(directory());QDesktopServices::openUrl(QUrl::fromLocalFile(directory()));});
    QObject::connect(&sample,&QPushButton::clicked,&dialog,[&]{const auto path=QFileDialog::getSaveFileName(&dialog,Language::text("Beispielpaket speichern"),"local-sensor.thinkcad-components.json","ThinkCad Components (*.thinkcad-components.json)");if(path.isEmpty())return;ModuleDefinition d;d.id="local-register-sensor";d.label="Local temperature sensor";d.bus="i2c";d.implementation="registers";d.pins={"VCC","GND","SDA","SCL"};d.address=0x48;d.registers={{0,0x42}};d.readOnly={0};d.parameters={{"temperature","Temperature","C",25,-40,85,0x10,2,100,false,true}};const auto bytes=QJsonDocument(QJsonObject{{"format","ThinkCad.Components"},{"version",1},{"models",QJsonArray{encode(d)}}}).toJson();QSaveFile out(path);if(!out.open(QIODevice::WriteOnly)||out.write(bytes)!=bytes.size()||!out.commit())QMessageBox::warning(&dialog,dialog.windowTitle(),Language::text("Bauteilpaket konnte nicht gespeichert werden."));});
    QDialogButtonBox close(QDialogButtonBox::Close);root.addWidget(&close);QObject::connect(&close,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);dialog.exec();return changed;
}

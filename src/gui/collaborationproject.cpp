#include "collaborationproject.hpp"
#include <QJsonDocument>
#include <QJsonArray>
#include <QSet>
QJsonObject CollaborationProject::encode(const SavedProject& project){
    auto object=QJsonDocument::fromJson(ProjectIO::serialize(project)).object();
    // Local selection and library installation paths are never shared. The root
    // sketch duplicates the active board in disk projects; synchronize board
    // sketches once to avoid conflicting edits to that redundant copy.
    if(object["boardSketches"].toObject().isEmpty())object["unassignedSketch"]=QJsonObject{{"language",object["language"]},{"name",object["name"]},{"files",object["files"]},{"breakpoints",object["breakpoints"]}};
    for(const auto* key:{"activeBoard","name","files","breakpoints","libraries","language"})object.remove(key);
    return object;
}
SavedProject CollaborationProject::decode(const QJsonObject& document){
    static const QSet<QString> allowed={"format","version","physicalLayout","executionMode","componentModels","parts","wires","boardSketches","unassignedSketch"};
    for(auto it=document.begin();it!=document.end();++it)if(!allowed.contains(it.key()))throw std::runtime_error("Unexpected LAN document field");
    auto object=document;const auto sketches=object["boardSketches"].toObject();
    QJsonObject primary;
    if(sketches.isEmpty()){if(!object["unassignedSketch"].isObject())throw std::runtime_error("Missing LAN sketch");primary=object["unassignedSketch"].toObject();}
    else {if(object.contains("unassignedSketch"))throw std::runtime_error("Ambiguous LAN sketch");primary=sketches.begin().value().toObject();object["activeBoard"]=sketches.begin().key();}
    for(const auto* key:{"files","name","breakpoints","language"})if(primary.contains(key))object[key]=primary[key];object["libraries"]=QJsonObject{};
    for(const auto& value:object["parts"].toArray()){const auto part=value.toObject();if(part["kind"].toInt(-1)==static_cast<int>(thinkercad::PartKind::Esp8266)&&!sketches.contains(part["id"].toString()))throw std::runtime_error("Missing LAN board sketch");}
    auto project=ProjectIO::deserialize(QJsonDocument(object).toJson(QJsonDocument::Compact));
    for(const auto& part:project.circuit.parts())if(part.kind==thinkercad::PartKind::Esp8266&&!project.boardSketches.contains(QString::fromStdString(part.id)))throw std::runtime_error("Missing LAN board sketch");
    return project;
}
QString CollaborationProject::validate(const QJsonObject& document){try{decode(document);return {};}catch(const std::exception& error){return QString::fromUtf8(error.what());}}

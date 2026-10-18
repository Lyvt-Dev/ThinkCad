#include "projectmerge.hpp"
#include <QJsonArray>
#include <QSet>
#include <algorithm>
namespace {
struct Edit {int start=0,end=0;QString replacement;};
Edit difference(const QString& base,const QString& value){int start=0;while(start<base.size()&&start<value.size()&&base[start]==value[start])++start;int end=base.size(),tail=value.size();while(end>start&&tail>start&&base[end-1]==value[tail-1]){--end;--tail;}return {start,end,value.mid(start,tail-start)};}
bool splitSurrogate(const QString& value,int at){return at>0&&at<value.size()&&value[at-1].isHighSurrogate()&&value[at].isLowSurrogate();}
QStringList keys(const QJsonObject& a,const QJsonObject& b,const QJsonObject& c){auto names=a.keys();for(const auto& name:b.keys())if(!names.contains(name))names.append(name);for(const auto& name:c.keys())if(!names.contains(name))names.append(name);return names;}
QString identityKey(const QString& path){const auto field=path.section('/',-1);if(field=="files")return "name";if(field=="parts"||field=="wires"||field=="componentModels"||field=="models")return "id";return {};}
bool index(const QJsonArray& array,const QString& key,QJsonObject& result,QStringList& order){for(const auto& value:array){if(!value.isObject())return false;const auto object=value.toObject();if(!object[key].isString()||object[key].toString().isEmpty())return false;const auto id=object[key].toString();if(result.contains(id))return false;result[id]=object;if(!order.contains(id))order.append(id);}return true;}
class Merger {
    bool prefer_;int nodes_=0;
public:
    QStringList conflicts;
    explicit Merger(bool prefer):prefer_(prefer){}
    QJsonValue value(const QJsonValue& base,const QJsonValue& current,const QJsonValue& proposed,const QString& path,int depth=0){
        if(++nodes_>40000||depth>32){if(conflicts.size()<256)conflicts.append(path);return current;}
        if(current==proposed||proposed==base)return current;if(current==base)return proposed;
        if(base.isObject()&&current.isObject()&&proposed.isObject()){
            const auto b=base.toObject(),c=current.toObject(),p=proposed.toObject();QJsonObject merged;
            for(const auto& key:keys(b,c,p)){const auto child=value(b.value(key),c.value(key),p.value(key),path+'/'+key,depth+1);if(!child.isUndefined())merged[key]=child;}return merged;
        }
        const auto idKey=identityKey(path);
        if(!idKey.isEmpty()&&base.isArray()&&current.isArray()&&proposed.isArray()){
            QJsonObject b,c,p;QStringList order,unused;
            if(index(base.toArray(),idKey,b,unused)&&index(current.toArray(),idKey,c,order)&&index(proposed.toArray(),idKey,p,order)){
                QJsonArray merged;for(const auto& id:order){const auto child=value(b.value(id),c.value(id),p.value(id),path+'/'+id,depth+1);if(!child.isUndefined())merged.append(child);}return merged;
            }
        }
        if(path.endsWith("/text")&&base.isString()&&current.isString()&&proposed.isString()){
            const auto original=base.toString();const auto first=difference(original,current.toString()),second=difference(original,proposed.toString());
            // Adjacent edits merge. Two inserts at the same position conflict;
            // a boundary shared with a replacement is also kept explicit.
            const bool disjoint=first.end<second.start||second.end<first.start||(first.end==second.start&&first.start<first.end&&second.start<second.end)||(second.end==first.start&&first.start<first.end&&second.start<second.end);
            if(disjoint&&!splitSurrogate(original,first.start)&&!splitSurrogate(original,first.end)&&!splitSurrogate(original,second.start)&&!splitSurrogate(original,second.end)){
                auto text=original;auto edits=QList<Edit>{first,second};std::sort(edits.begin(),edits.end(),[](const auto& a,const auto& b){return a.start>b.start;});for(const auto& edit:edits)text.replace(edit.start,edit.end-edit.start,edit.replacement);return text;
            }
        }
        if(conflicts.size()<256)conflicts.append(path);return prefer_?proposed:current;
    }
};
}
ProjectMerge::Result ProjectMerge::merge(const QJsonObject& base,const QJsonObject& current,const QJsonObject& proposed,bool prefer){Merger merger(prefer);auto result=merger.value(base,current,proposed,QString()).toObject();return {result,merger.conflicts};}

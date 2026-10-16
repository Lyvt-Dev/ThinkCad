#include "gdbmi.hpp"
#include <QVariantList>
#include <stdexcept>
namespace {
class Reader {
    const QByteArray& text_;qsizetype at_=0;unsigned items_=0;
    [[noreturn]] void fail()const{throw std::runtime_error("Malformed or oversized GDB/MI record");}
    void space(){while(at_<text_.size()&&(text_[at_]==' '||text_[at_]=='\t'))++at_;}
    char peek(){space();return at_<text_.size()?text_[at_]:0;}
    char take(){if(at_>=text_.size())fail();return text_[at_++];}
    void expect(char c){space();if(take()!=c)fail();}
    static bool nameChar(char c){return (c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='-';}
    QString name(){space();const auto start=at_;while(at_<text_.size()&&nameChar(text_[at_]))++at_;if(start==at_)fail();return QString::fromLatin1(text_.mid(start,at_-start));}
    QString string(){
        expect('"');QByteArray result;
        while(at_<text_.size()){const char c=take();if(c=='"')return QString::fromUtf8(result);if(c!='\\'){result+=c;continue;}const char escaped=take();if(escaped>='0'&&escaped<='7'){int code=escaped-'0';for(int i=1;i<3&&at_<text_.size()&&text_[at_]>='0'&&text_[at_]<='7';++i)code=code*8+take()-'0';result+=char(code);}
            else switch(escaped){case 'n':result+='\n';break;case 'r':result+='\r';break;case 't':result+='\t';break;case 'b':result+='\b';break;case 'f':result+='\f';break;case 'a':result+='\a';break;case 'v':result+='\v';break;default:result+=escaped;break;}}
        fail();
    }
    QVariant value(unsigned depth){
        if(depth>20||++items_>4096)fail();const auto c=peek();if(c=='"')return string();
        if(c=='{'){++at_;auto result=results('}',depth+1);expect('}');return result;}
        if(c=='['){++at_;QVariantList list;if(peek()==']'){++at_;return list;}while(true){const auto start=at_;bool keyed=false;if(nameChar(peek())){name();keyed=peek()=='=';}at_=start;if(keyed){const auto key=name();expect('=');list.append(QVariantMap{{key,value(depth+1)}});}else list.append(value(depth+1));if(peek()==']'){++at_;return list;}expect(',');}}
        fail();
    }
    QVariantMap results(char terminator,unsigned depth){
        QVariantMap map;if(peek()==terminator||peek()==0)return map;while(true){const auto key=name();if(map.contains(key))fail();expect('=');map[key]=value(depth);if(peek()==terminator||peek()==0)return map;expect(',');}
    }
public:
    explicit Reader(const QByteArray& text):text_(text){}
    GdbMi::Record record(){
        GdbMi::Record result;const auto start=at_;while(at_<text_.size()&&text_[at_]>='0'&&text_[at_]<='9')++at_;if(at_>start){bool ok=false;result.token=text_.mid(start,at_-start).toInt(&ok);if(!ok)fail();}
        result.type=take();if(result.type=='~'||result.type=='@'||result.type=='&')result.text=string();else {if(result.type!='^'&&result.type!='*'&&result.type!='+'&&result.type!='=')fail();result.kind=name();if(peek()==','){++at_;if(peek()==0)fail();result.values=results(0,0);}}
        if(peek()!=0)fail();return result;
    }
};
}
std::optional<GdbMi::Record> GdbMi::parse(const QByteArray& input,QString* error){
    if(error)error->clear();const auto line=input.trimmed();if(line.isEmpty()||line=="(gdb)")return std::nullopt;
    try{if(line.contains('\0'))throw std::runtime_error("NUL in GDB/MI record");if(line.size()>262144)throw std::runtime_error("GDB/MI record exceeds 256 KiB");return Reader(line).record();}catch(const std::exception& e){if(error)*error=QString::fromUtf8(e.what());return std::nullopt;}
}
QByteArray GdbMi::quote(const QString& text){
    QByteArray out="\"";for(unsigned char c:text.toUtf8()){if(c=='\\'||c=='"'){out+='\\';out+=char(c);}else if(c<32||c==127){out+='\\';out+=char('0'+((c>>6)&7));out+=char('0'+((c>>3)&7));out+=char('0'+(c&7));}else out+=char(c);}return out+'"';
}

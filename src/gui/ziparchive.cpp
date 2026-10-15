#include "ziparchive.hpp"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>
#include <zlib.h>
#include <stdexcept>
#include <algorithm>
namespace {
struct Entry {QString name;QByteArray rawName;quint32 crc,compressed,size,offset;quint16 method,flags;bool directory;};
[[noreturn]]void bad(const char* text){throw std::runtime_error(text);}
}
void ZipArchive::extractArduinoLibrary(const QByteArray& bytes,const QString& destination){
    if(bytes.size()>32*1024*1024||bytes.size()<22)bad("Ungültige ZIP-Größe.");
    auto u16=[&](qint64 pos)->quint16{if(pos<0||pos+2>bytes.size())bad("ZIP-Datei abgeschnitten.");return static_cast<quint8>(bytes[pos])|(static_cast<quint16>(static_cast<quint8>(bytes[pos+1]))<<8);};
    auto u32=[&](qint64 pos)->quint32{return u16(pos)|(static_cast<quint32>(u16(pos+2))<<16);};
    qint64 end=-1;for(qint64 i=bytes.size()-22;i>=std::max<qint64>(0,bytes.size()-65557);--i)if(u32(i)==0x06054b50&&i+22+u16(i+20)==bytes.size()){end=i;break;}
    if(end<0||u16(end+4)||u16(end+6)||u16(end+8)!=u16(end+10))bad("ZIP-Endverzeichnis ungültig.");
    const auto count=u16(end+10);const auto central=u32(end+16),centralSize=u32(end+12);if(count>4000||static_cast<quint64>(central)+centralSize!=static_cast<quint64>(end))bad("ZIP-Verzeichnis zu groß oder ungültig.");
    QList<Entry> entries;QString root;QSet<QString> names;quint64 total=0;qint64 pos=central;
    for(unsigned i=0;i<count;++i){if(u32(pos)!=0x02014b50)bad("ZIP-Verzeichniseintrag fehlt.");const auto nameLength=u16(pos+28),extra=u16(pos+30),comment=u16(pos+32);if(pos+46+nameLength+extra+comment>end||u16(pos+34))bad("ZIP-Eintrag ungültig.");Entry e;e.rawName=bytes.mid(pos+46,nameLength);e.name=QString::fromUtf8(e.rawName);e.directory=e.name.endsWith('/');if(e.directory)e.name.chop(1);e.flags=u16(pos+8);e.method=u16(pos+10);e.crc=u32(pos+16);e.compressed=u32(pos+20);e.size=u32(pos+24);e.offset=u32(pos+42);
        if((e.flags&1)||(e.method!=0&&e.method!=8)||e.size>16*1024*1024||e.compressed>32*1024*1024||((u32(pos+38)>>16)&0170000)==0120000)bad("ZIP-Eintrag nicht unterstützt oder zu groß.");
        if(e.name.isEmpty()||e.name.startsWith('/')||e.name.contains('\\')||e.name.contains(':')||e.name.contains(QChar(0))||e.name.size()>240||QDir::cleanPath(e.name)!=e.name||e.name.split('/').contains(".."))bad("Unsicherer Archivpfad.");
        for(const auto& component:e.name.split('/'))if(component.endsWith(' ')||component.endsWith('.')||QRegularExpression("^(CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])(?:\\.|$)",QRegularExpression::CaseInsensitiveOption).match(component).hasMatch())bad("Ungültiger Windows-Dateiname.");
        if(names.contains(e.name.toCaseFolded()))bad("Doppelter Archivpfad.");names.insert(e.name.toCaseFolded());const auto first=e.name.section('/',0,0);if(root.isEmpty())root=first;else if(root!=first)bad("Archiv braucht genau einen Wurzelordner.");total+=e.size;if(total>64*1024*1024)bad("Entpackte Bibliothek zu groß.");entries<<e;pos+=46+nameLength+extra+comment;
    }
    if(pos!=end)bad("ZIP-Verzeichnisgröße stimmt nicht.");QDir out(destination);if(!out.exists()||!out.entryList(QDir::AllEntries|QDir::NoDotAndDotDot).isEmpty())bad("Zielordner muss leer sein.");
    for(const auto& e:entries){if(e.directory)continue;const auto relative=e.name.mid(root.size()+1);if(relative.isEmpty())bad("Ungültiges Bibliothekslayout.");const qint64 local=e.offset;if(u32(local)!=0x04034b50||u16(local+8)!=e.method||u16(local+6)!=e.flags)bad("ZIP-Lokalheader stimmt nicht.");const auto nameLength=u16(local+26),extra=u16(local+28);if(bytes.mid(local+30,nameLength)!=e.rawName)bad("ZIP-Dateinamen stimmen nicht.");const qint64 data=local+30+nameLength+extra;if(data<0||data+e.compressed>central)bad("ZIP-Daten außerhalb des Archivs.");QByteArray content(std::max<quint32>(1,e.size),Qt::Uninitialized);
        if(e.method==0){if(e.compressed!=e.size)bad("ZIP-Dateigröße stimmt nicht.");content=bytes.mid(data,e.size);}else{z_stream stream{};stream.next_in=reinterpret_cast<Bytef*>(const_cast<char*>(bytes.constData()+data));stream.avail_in=e.compressed;stream.next_out=reinterpret_cast<Bytef*>(content.data());stream.avail_out=content.size();if(inflateInit2(&stream,-MAX_WBITS)!=Z_OK)bad("Zlib konnte nicht starten.");const auto status=inflate(&stream,Z_FINISH);const bool ok=status==Z_STREAM_END&&stream.total_out==e.size&&stream.total_in==e.compressed;inflateEnd(&stream);if(!ok)bad("Defekte oder zu große ZIP-Daten.");content.resize(e.size);}
        if(crc32(0,reinterpret_cast<const Bytef*>(content.constData()),content.size())!=e.crc)bad("ZIP-CRC stimmt nicht.");const auto path=out.filePath(relative);if(!QDir().mkpath(QFileInfo(path).absolutePath()))bad("Bibliotheksordner nicht schreibbar.");QFile file(path);if(!file.open(QIODevice::WriteOnly|QIODevice::NewOnly)||file.write(content)!=content.size())bad("Bibliotheksdatei nicht schreibbar.");
    }
    if(!QFileInfo::exists(out.filePath("library.properties")))bad("Arduino library.properties fehlt.");
}

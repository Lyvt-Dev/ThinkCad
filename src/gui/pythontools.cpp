#include "pythontools.hpp"
#include "paths.hpp"
#include "language.hpp"
#include <QPlainTextEdit>
#include <QTextDocument>
#include <QTextCursor>
#include <QLabel>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QFontDatabase>
#include <QProcessEnvironment>

PythonTools::PythonTools(QWidget* parent):QDialog(parent){
    setObjectName("pythonToolsDialog");setWindowTitle(Language::text("Lokale Python-3-Skripte"));resize(840,650);
    auto* root=new QVBoxLayout(this);auto* description=new QLabel(Language::text("Allgemeine lokale Python-3-Skripte. Board-Sketches mit GPIO unter MicroPython im Editor ausführen."));description->setWordWrap(true);root->addWidget(description);
    auto* tools=new QHBoxLayout;auto button=[&](const QString& title,const char* name){auto* item=new QPushButton(Language::text(title));item->setObjectName(name);item->setMinimumWidth(100);tools->addWidget(item);return item;};
    auto* open=button("Öffnen…","pythonOpen");auto* save=button("Speichern…","pythonSave");tools->addStretch();run_=button("Ausführen","pythonRun");stop_=button("Stoppen","pythonStop");root->addLayout(tools);
    path_=new QLabel;path_->setObjectName("pythonScriptPath");path_->setTextFormat(Qt::PlainText);path_->setWordWrap(true);root->addWidget(path_);
    auto* split=new QSplitter(Qt::Vertical);source_=new QPlainTextEdit;source_->setObjectName("pythonSource");source_->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));source_->setPlainText("print('Hello from ThinkCad')\n");split->addWidget(source_);output_=new QPlainTextEdit;output_->setObjectName("pythonOutput");output_->setReadOnly(true);output_->setMaximumBlockCount(4000);output_->setFont(source_->font());split->addWidget(output_);split->setSizes({380,180});root->addWidget(split,1);
    connect(open,&QPushButton::clicked,this,[this]{const auto path=QFileDialog::getOpenFileName(this,Language::text("Python-Skript öffnen"),{},"Python (*.py)");if(!path.isEmpty())openFile(path);});
    connect(save,&QPushButton::clicked,this,[this]{saveFile();});
    connect(run_,&QPushButton::clicked,this,&PythonTools::run);connect(stop_,&QPushButton::clicked,&process_,&QProcess::kill);
    connect(&process_,&QProcess::readyReadStandardOutput,this,[this]{append(process_.readAllStandardOutput());});
    connect(&process_,&QProcess::started,this,&PythonTools::updateButtons);connect(&process_,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),this,[this](int code,QProcess::ExitStatus){append(process_.readAllStandardOutput());output_->appendPlainText(Language::text("Python beendet: %1").arg(code));updateButtons();});
    connect(&process_,&QProcess::errorOccurred,this,[this](QProcess::ProcessError){output_->appendPlainText(process_.errorString());updateButtons();});
    process_.setProcessChannelMode(QProcess::MergedChannels);updateButtons();
}
PythonTools::~PythonTools(){process_.disconnect(this);if(running()){process_.kill();process_.waitForFinished(1000);}}
bool PythonTools::openFile(const QString& path){
    if(running()||!confirmReplace())return false;QFile file(path);if(!file.open(QIODevice::ReadOnly)||file.size()>2*1024*1024){output_->appendPlainText(Language::text("Python-Datei nicht lesbar oder größer als 2 MiB."));return false;}
    file_=QFileInfo(path).absoluteFilePath();source_->setPlainText(QString::fromUtf8(file.readAll()));source_->document()->setModified(false);path_->setText(file_);return true;
}
bool PythonTools::saveFile(){
    auto path=QFileDialog::getSaveFileName(this,Language::text("Python-Skript speichern"),file_.isEmpty()?QString("script.py"):file_,"Python (*.py)");if(path.isEmpty())return false;if(!path.endsWith(".py",Qt::CaseInsensitive))path+=".py";QSaveFile file(path);const auto bytes=source_->toPlainText().toUtf8();if(!file.open(QIODevice::WriteOnly)||file.write(bytes)!=bytes.size()||!file.commit()){output_->appendPlainText(file.errorString());return false;}file_=path;path_->setText(file_);source_->document()->setModified(false);return true;
}
bool PythonTools::confirmReplace(){
    if(!source_->document()->isModified())return true;const auto choice=QMessageBox::question(this,Language::text("Ungespeicherte Änderungen"),Language::text("Skript vor dem Fortfahren speichern?"),QMessageBox::Save|QMessageBox::Discard|QMessageBox::Cancel);return choice==QMessageBox::Discard||(choice==QMessageBox::Save&&saveFile());
}
void PythonTools::reject(){if(confirmReplace())QDialog::reject();}
void PythonTools::append(const QByteArray& bytes){
    if(bytes.isEmpty()||outputBytes_>1024*1024)return;outputBytes_+=bytes.size();if(outputBytes_>1024*1024){process_.kill();output_->appendPlainText(Language::text("Python-Ausgabe überschreitet 1 MiB. Skript gestoppt."));return;}
    auto cursor=output_->textCursor();cursor.movePosition(QTextCursor::End);cursor.insertText(decoder_(bytes));output_->setTextCursor(cursor);
}
void PythonTools::updateButtons(){const bool busy=running();run_->setEnabled(!busy);stop_->setEnabled(busy);source_->setReadOnly(busy);findChild<QPushButton*>("pythonOpen")->setEnabled(!busy);findChild<QPushButton*>("pythonSave")->setEnabled(!busy);}
void PythonTools::run(){
    if(running())return;const auto python=AppPaths::python();if(python.isEmpty()){output_->appendPlainText(Language::text("Python 3 fehlt. Unter Einstellungen / Editor einen Python-Interpreter auswählen."));return;}
    temporary_=std::make_unique<QTemporaryDir>();if(!temporary_->isValid())return;const auto filename=temporary_->filePath("script.py");QFile file(filename);const auto bytes=source_->toPlainText().toUtf8();if(bytes.size()>2*1024*1024||!file.open(QIODevice::WriteOnly)||file.write(bytes)!=bytes.size()){output_->appendPlainText(Language::text("Python-Datei nicht lesbar oder größer als 2 MiB."));return;}file.close();
    const auto origin=file_.isEmpty()?QDir::homePath():QFileInfo(file_).absolutePath();const auto display=file_.isEmpty()?QString("<ThinkCad script>"):file_;
    auto environment=QProcessEnvironment::systemEnvironment();environment.insert("PYTHONIOENCODING","utf-8");environment.insert("PYTHONUNBUFFERED","1");
    // AppImage library paths belong to ThinkCad, not the user's Python runtime.
    if(environment.contains("APPIMAGE"))for(const auto* name:{"LD_LIBRARY_PATH","QT_PLUGIN_PATH","QT_QPA_PLATFORM_PLUGIN_PATH"}){const auto original="THINKCAD_ORIGINAL_"+QString::fromLatin1(name);if(environment.contains(original))environment.insert(name,environment.value(original));else environment.remove(name);}
    process_.setProcessEnvironment(environment);process_.setWorkingDirectory(origin);output_->clear();outputBytes_=0;decoder_=QStringDecoder(QStringDecoder::Utf8);
    const QString bootstrap="import sys\np,base,name=sys.argv[1:4]\nsys.path.insert(0,base)\nsys.argv=[name]\nwith open(p,encoding='utf-8') as f: source=f.read()\nexec(compile(source,name,'exec'),{'__name__':'__main__','__file__':name,'__package__':None})\n";
    process_.start(python,{"-u","-c",bootstrap,filename,origin,display});updateButtons();
}

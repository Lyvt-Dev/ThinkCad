#include "tutorialcourse.hpp"
#include "startpage.hpp"
#include "language.hpp"
#include "projectpreview.hpp"
#include <QCheckBox>
#include <QLineEdit>
#include <QComboBox>
#include <algorithm>
#include <QButtonGroup>
#include <QGroupBox>
#include <QDateTime>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QSettings>
#include <QSignalBlocker>
#include <QTimer>
#include <QStackedWidget>
#include <QVBoxLayout>
namespace {
QLabel* label(const QString& text,const char* name,QWidget* parent=nullptr){auto* l=new QLabel(Language::text(text),parent);l->setObjectName(name);l->setTextFormat(Qt::PlainText);l->setWordWrap(true);return l;}
QPushButton* button(const QString& text,const char* name,bool translate=true){auto* b=new QPushButton(translate?Language::text(text):text);b->setObjectName(name);b->setMinimumHeight(34);return b;}
}
WelcomePage::WelcomePage(QWidget* parent):QWidget(parent){
    setObjectName("startPage");
    // Scoped to the welcome page: editor widgets retain their existing styling.
    setStyleSheet("QWidget#startPage QPushButton {padding:6px 12px;} "
                  "QWidget#startPage QPushButton:checked {background:palette(highlight);color:palette(highlighted-text);} "
                  "QWidget#startPage QLabel#welcomeTitle {font-size:18px;font-weight:600;} "
                  "QWidget#startPage QFrame#startDivider {border:0;background:palette(mid);max-width:1px;} "
                  "QWidget#startPage QGroupBox {font-weight:600;margin-top:12px;} "
                  "QWidget#startPage QGroupBox::title {subcontrol-origin:margin;left:12px;padding:0 4px;}");
    auto* root=new QHBoxLayout(this);root->setContentsMargins(0,0,0,0);root->setSpacing(0);
    auto* sidebarScroll=new QScrollArea;sidebarScroll->setObjectName("startNavigationScroll");sidebarScroll->setWidgetResizable(true);sidebarScroll->setFrameShape(QFrame::NoFrame);sidebarScroll->setFixedWidth(230);
    auto* sidebar=new QFrame;sidebar->setObjectName("startSidebar");auto* nav=new QVBoxLayout(sidebar);nav->setContentsMargins(14,18,14,14);nav->setSpacing(4);
    nav->addWidget(label("ThinkCad","startBrand"));nav->addWidget(label("Circuit Studio","muted"));nav->addSpacing(10);nav->addWidget(label("Projekte","navigationSection"));
    auto* home=button("Meine Projekte","homeNavigation");home->setCheckable(true);home->setChecked(true);nav->addWidget(home);
    auto* navigationNew=button("Neues Projekt","navigationNewProject");auto* navigationOpen=button("Projekt öffnen…","navigationOpenProject");nav->addWidget(navigationNew);nav->addWidget(navigationOpen);
    nav->addSpacing(8);nav->addWidget(label("Werkzeuge","navigationSection"));auto* navigation=new QButtonGroup(this);navigation->addButton(home);
    auto* tutorial=button("Tutorial","tutorialNavigation");tutorial->setCheckable(true);navigation->addButton(tutorial);nav->addWidget(tutorial);
    auto* navigationExamples=button("Beispielschaltungen…","navigationExamples");nav->addWidget(navigationExamples);
    auto* python=button("Python-3-Skripte…","navigationPython");nav->addWidget(python);
    nav->addSpacing(8);nav->addWidget(label("Programm","navigationSection"));auto* settings=button("Einstellungen","settingsNavigation");nav->addWidget(settings);auto* changelog=button("Änderungsverlauf…","navigationChangelog");nav->addWidget(changelog);nav->addStretch();
    auto* resume=button("Zum Editor","resumeEditor");nav->addWidget(resume);nav->addWidget(label(Language::text("Version ")+THINKERCAD_VERSION,"startVersion"));sidebarScroll->setWidget(sidebar);root->addWidget(sidebarScroll);auto* divider=new QFrame;divider->setObjectName("startDivider");divider->setFrameShape(QFrame::VLine);root->addWidget(divider);
    connect(navigationNew,&QPushButton::clicked,this,&WelcomePage::newRequested);connect(navigationOpen,&QPushButton::clicked,this,&WelcomePage::openRequested);connect(navigationExamples,&QPushButton::clicked,this,&WelcomePage::examplesRequested);connect(changelog,&QPushButton::clicked,this,&WelcomePage::changelogRequested);connect(python,&QPushButton::clicked,this,&WelcomePage::pythonRequested);
    pages_=new QStackedWidget;root->addWidget(pages_,1);
    auto* scroll=new QScrollArea;scroll->setWidgetResizable(true);scroll->setFrameShape(QFrame::NoFrame);auto* content=new QWidget;auto* layout=new QVBoxLayout(content);layout->setContentsMargins(28,28,28,28);layout->setSpacing(20);
    layout->addWidget(label("Projekte","welcomeTitle"));layout->addWidget(label("ThinkCad – Schaltungen, Arduino und MicroPython","welcomeSubtitle"));
    auto* hero=new QGroupBox(Language::text("Projekt öffnen oder erstellen"));hero->setObjectName("welcomeHero");auto* heroLayout=new QVBoxLayout(hero);heroLayout->setContentsMargins(16,24,16,16);heroLayout->setSpacing(16);heroLayout->addWidget(label("Starte ein Projekt oder öffne eine lokale Schaltung, einen Arduino- oder MicroPython-Sketch.","heroSubtitle"));
    auto* actions=new QHBoxLayout;actions->setSpacing(12);auto* fresh=button("Neues Projekt","startNewProject");auto* open=button("Projekt öffnen…","startOpenProject");actions->addWidget(fresh);actions->addWidget(open);actions->addStretch();heroLayout->addLayout(actions);layout->addWidget(hero);
    auto* heading=new QHBoxLayout;heading->addWidget(label("Zuletzt geöffnet","recentTitle"));heading->addStretch();auto* clear=button("Liste leeren","clearRecentProjects");heading->addWidget(clear);layout->addLayout(heading);
    auto* recentTools=new QHBoxLayout;auto* search=new QLineEdit;search->setObjectName("recentProjectSearch");search->setPlaceholderText(Language::text("Projekte durchsuchen…"));search->setClearButtonEnabled(true);auto* sorting=new QComboBox;sorting->setObjectName("recentProjectSorting");sorting->addItem(Language::text("Zuletzt geöffnet"),"recent");sorting->addItem(Language::text("Name"),"name");recentTools->addWidget(search,1);recentTools->addWidget(sorting);layout->addLayout(recentTools);connect(search,&QLineEdit::textChanged,this,[this]{refresh();});connect(sorting,qOverload<int>(&QComboBox::currentIndexChanged),this,[this]{refresh();});
    recentLayout_=new QVBoxLayout;recentLayout_->setSpacing(0);layout->addLayout(recentLayout_);
    layout->addSpacing(4);layout->addWidget(label("Hilfe und Beispiele","recentTitle"));auto* examples=button("Beispielschaltungen entdecken","startExamples");layout->addWidget(examples,0,Qt::AlignLeft);layout->addStretch();scroll->setWidget(content);pages_->addWidget(scroll);
    auto* course=new TutorialCourse;pages_->addWidget(course);connect(course,&TutorialCourse::lessonRequested,this,&WelcomePage::lessonRequested);
    connect(home,&QPushButton::clicked,this,[this,home,settings]{pages_->setCurrentIndex(0);home->setChecked(true);settings->setChecked(false);});
    connect(settings,&QPushButton::clicked,this,&WelcomePage::preferencesRequested);
    connect(fresh,&QPushButton::clicked,this,&WelcomePage::newRequested);connect(open,&QPushButton::clicked,this,&WelcomePage::openRequested);connect(resume,&QPushButton::clicked,this,&WelcomePage::resumeRequested);connect(examples,&QPushButton::clicked,this,&WelcomePage::examplesRequested);connect(tutorial,&QPushButton::clicked,this,&WelcomePage::showCourse);
    connect(clear,&QPushButton::clicked,this,[this]{QSettings().remove("recentProjects");refresh();});refresh();
}
void WelcomePage::remember(const QString& path){
    const QFileInfo file(path);if(!file.isFile())return;const auto absolute=file.canonicalFilePath();auto paths=QSettings().value("recentProjects").toStringList();
    for(auto it=paths.begin();it!=paths.end();)if(QString::compare(*it,absolute,
#ifdef Q_OS_WIN
        Qt::CaseInsensitive
#else
        Qt::CaseSensitive
#endif
    )==0)it=paths.erase(it);else ++it;
    paths.prepend(absolute);while(paths.size()>12)paths.removeLast();QSettings().setValue("recentProjects",paths);
}
void WelcomePage::refresh(){
    findChild<QLabel*>("startVersion")->setText(Language::text("Version ")+THINKERCAD_VERSION);
    while(auto* item=recentLayout_->takeAt(0)){delete item->widget();delete item;}
    auto paths=QSettings().value("recentProjects").toStringList().mid(0,12);
    const auto query=findChild<QLineEdit*>("recentProjectSearch")->text().trimmed();if(findChild<QComboBox*>("recentProjectSorting")->currentData().toString()=="name")std::stable_sort(paths.begin(),paths.end(),[](const QString& a,const QString& b){return QFileInfo(a).fileName().compare(QFileInfo(b).fileName(),Qt::CaseInsensitive)<0;});
    int matches=0;
    findChild<QPushButton*>("clearRecentProjects")->setEnabled(!paths.isEmpty());
    if(paths.isEmpty())recentLayout_->addWidget(label("Noch keine Projekte. Erstelle ein Projekt oder öffne eine vorhandene Datei.","emptyProjects"));
    for(const auto& path:paths){
        if(!path.contains(query,Qt::CaseInsensitive))continue;++matches;
        const QFileInfo file(path);auto* card=new QFrame;card->setObjectName("recentProjectCard");auto* row=new QHBoxLayout(card);row->setContentsMargins(0,14,0,14);row->setSpacing(16);
        auto* preview=new QLabel(Language::text("Keine Vorschau"));preview->setObjectName("recentProjectPreview");preview->setFixedSize(120,72);preview->setAlignment(Qt::AlignCenter);preview->setWordWrap(true);preview->setFrameShape(QFrame::StyledPanel);preview->setAccessibleName(Language::text("Projektvorschau"));row->addWidget(preview);
        // Deferred and tied to the row lifetime: opening the menu remains immediate.
        QTimer::singleShot(0,preview,[preview,path]{const auto image=ProjectPreview::load(path);if(!image.isNull()){auto pixmap=QPixmap::fromImage(image).scaled(240,144,Qt::KeepAspectRatio,Qt::SmoothTransformation);pixmap.setDevicePixelRatio(2);preview->setPixmap(pixmap);}});
        auto* open=button(file.fileName().replace("&","&&"),"recentProject",false);open->setProperty("projectPath",path);open->setToolTip(path);open->setMinimumWidth(160);open->setMaximumWidth(240);open->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Fixed);auto* detail=new QVBoxLayout;detail->setSpacing(6);detail->addWidget(open,0,Qt::AlignLeft);auto* location=label(file.absolutePath(),"recentLocation");location->setToolTip(path);location->setWordWrap(false);location->setMinimumWidth(0);location->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);detail->addWidget(location);row->addLayout(detail,1);
        auto* date=label(file.exists()?file.lastModified().toString("dd.MM.yyyy"):Language::text("Datei fehlt"),"muted");row->addWidget(date);open->setEnabled(file.isFile());auto* remove=button("Entfernen","removeRecentProject");remove->setAccessibleName(Language::text("Aus Liste entfernen"));remove->setToolTip(remove->accessibleName());row->addWidget(remove);
        connect(open,&QPushButton::clicked,this,[this,path]{emit recentRequested(path);});connect(remove,&QPushButton::clicked,this,[this,path]{auto list=QSettings().value("recentProjects").toStringList();list.removeAll(path);QSettings().setValue("recentProjects",list);QTimer::singleShot(0,this,&WelcomePage::refresh);});recentLayout_->addWidget(card);
    }
    if(!paths.isEmpty()&&!matches)recentLayout_->addWidget(label("Keine passenden Projekte gefunden.","noMatchingProjects"));
    Language::translateTree(this);
}

void WelcomePage::showCourse(){findChild<QPushButton*>("homeNavigation")->setChecked(false);findChild<QPushButton*>("settingsNavigation")->setChecked(false);findChild<QPushButton*>("tutorialNavigation")->setChecked(true);findChild<TutorialCourse*>()->refresh();pages_->setCurrentIndex(1);}

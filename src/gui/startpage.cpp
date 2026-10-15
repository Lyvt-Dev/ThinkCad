#include "startpage.hpp"
#include "appicons.hpp"
#include "language.hpp"
#include <QCheckBox>
#include <QComboBox>
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
QPushButton* button(const QString& text,AppIcons::Symbol icon,const char* name){auto* b=new QPushButton(AppIcons::icon(icon),Language::text(text));b->setObjectName(name);b->setIconSize({24,24});b->setCursor(Qt::PointingHandCursor);return b;}
}
WelcomePage::WelcomePage(QWidget* parent):QWidget(parent){
    setObjectName("startPage");auto* root=new QHBoxLayout(this);root->setContentsMargins(0,0,0,0);root->setSpacing(0);
    auto* sidebar=new QFrame;sidebar->setObjectName("startSidebar");sidebar->setFixedWidth(208);auto* nav=new QVBoxLayout(sidebar);nav->setContentsMargins(18,26,18,22);nav->setSpacing(10);
    auto* brand=new QLabel;brand->setPixmap(QPixmap(":/brand/logo.png").scaled(48,48,Qt::KeepAspectRatio,Qt::SmoothTransformation));nav->addWidget(brand);nav->addWidget(label("ThinkCad","startBrand"));nav->addWidget(label("Circuit Studio","muted"));nav->addSpacing(28);
    auto* home=button("Meine Projekte",AppIcons::Symbol::Home,"homeNavigation");home->setCheckable(true);home->setChecked(true);nav->addWidget(home);
    auto* settings=button("Einstellungen",AppIcons::Symbol::Settings,"settingsNavigation");settings->setCheckable(true);nav->addWidget(settings);
    auto* tutorial=button("Tutorial",AppIcons::Symbol::Tutorial,"tutorialNavigation");nav->addWidget(tutorial);nav->addStretch();
    auto* resume=button("Zum Editor",AppIcons::Symbol::Circuit,"resumeEditor");nav->addWidget(resume);nav->addWidget(label("Version " THINKERCAD_VERSION,"muted"));root->addWidget(sidebar);
    pages_=new QStackedWidget;root->addWidget(pages_,1);
    auto* scroll=new QScrollArea;scroll->setWidgetResizable(true);scroll->setFrameShape(QFrame::NoFrame);auto* content=new QWidget;auto* layout=new QVBoxLayout(content);layout->setContentsMargins(32,30,32,30);layout->setSpacing(18);
    layout->addWidget(label("Dein nächstes Projekt beginnt hier.","welcomeTitle"));layout->addWidget(label("Schaltungen entwerfen, Arduino-Code testen und Ideen ausprobieren.","welcomeSubtitle"));
    auto* hero=new QFrame;hero->setObjectName("welcomeHero");auto* heroLayout=new QVBoxLayout(hero);heroLayout->setContentsMargins(24,22,24,22);heroLayout->setSpacing(12);heroLayout->addWidget(label("Von der Idee zur Schaltung","heroTitle"));heroLayout->addWidget(label("Starte mit einem leeren Projekt oder öffne eine lokale Schaltung oder einen .ino-Sketch.","heroSubtitle"));
    auto* actions=new QHBoxLayout;auto* fresh=button("Neues Projekt",AppIcons::Symbol::NewProject,"startNewProject");auto* open=button("Projekt öffnen…",AppIcons::Symbol::Folder,"startOpenProject");actions->addWidget(fresh);actions->addWidget(open);actions->addStretch();heroLayout->addLayout(actions);layout->addWidget(hero);
    auto* heading=new QHBoxLayout;heading->addWidget(label("Zuletzt geöffnet","recentTitle"));heading->addStretch();auto* clear=button("Liste leeren",AppIcons::Symbol::Reset,"clearRecentProjects");heading->addWidget(clear);layout->addLayout(heading);
    recentLayout_=new QVBoxLayout;recentLayout_->setSpacing(10);layout->addLayout(recentLayout_);
    layout->addSpacing(6);layout->addWidget(label("Lernen & ausprobieren","recentTitle"));auto* examples=button("Beispielschaltungen entdecken",AppIcons::Symbol::Circuit,"startExamples");layout->addWidget(examples);layout->addStretch();scroll->setWidget(content);pages_->addWidget(scroll);
    auto* settingsPage=new QWidget;auto* settingsLayout=new QVBoxLayout(settingsPage);settingsLayout->setContentsMargins(32,30,32,30);settingsLayout->setSpacing(20);settingsLayout->addWidget(label("Einstellungen","welcomeTitle"));settingsLayout->addWidget(label("Passe ThinkCad an deinen Arbeitsplatz an.","welcomeSubtitle"));
    auto* dark=new QCheckBox(Language::text("Dark Mode"));dark->setObjectName("startDarkMode");settingsLayout->addWidget(dark);settingsLayout->addWidget(label("Sprache","sectionTitle"));auto* language=new QComboBox;language->setObjectName("startLanguage");language->addItem("Deutsch","de");language->addItem("English","en");language->setMaximumWidth(280);settingsLayout->addWidget(language);settingsLayout->addStretch();pages_->addWidget(settingsPage);
    connect(home,&QPushButton::clicked,this,[this,home,settings]{pages_->setCurrentIndex(0);home->setChecked(true);settings->setChecked(false);});
    connect(settings,&QPushButton::clicked,this,[this,home,settings]{pages_->setCurrentIndex(1);settings->setChecked(true);home->setChecked(false);});
    connect(dark,&QCheckBox::toggled,this,&WelcomePage::darkModeChanged);
    connect(language,qOverload<int>(&QComboBox::currentIndexChanged),this,[this,language](int index){emit languageChanged(language->itemData(index).toString());});
    connect(fresh,&QPushButton::clicked,this,&WelcomePage::newRequested);connect(open,&QPushButton::clicked,this,&WelcomePage::openRequested);connect(resume,&QPushButton::clicked,this,&WelcomePage::resumeRequested);connect(examples,&QPushButton::clicked,this,&WelcomePage::examplesRequested);connect(tutorial,&QPushButton::clicked,this,&WelcomePage::tutorialRequested);
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
    auto* dark=findChild<QCheckBox*>("startDarkMode");auto* language=findChild<QComboBox*>("startLanguage");{QSignalBlocker block(dark);dark->setChecked(QSettings().value("darkMode",false).toBool());}{QSignalBlocker block(language);language->setCurrentIndex(language->findData(Language::current()));}
    while(auto* item=recentLayout_->takeAt(0)){delete item->widget();delete item;}
    const auto paths=QSettings().value("recentProjects").toStringList().mid(0,12);
    if(paths.isEmpty())recentLayout_->addWidget(label("Noch keine Projekte. Erstelle ein Projekt oder öffne eine vorhandene Datei.","emptyProjects"));
    for(const auto& path:paths){
        const QFileInfo file(path);auto* card=new QFrame;card->setObjectName("recentProjectCard");auto* row=new QHBoxLayout(card);row->setContentsMargins(14,12,14,12);auto* open=button(file.completeBaseName(),AppIcons::Symbol::Circuit,"recentProject");open->setProperty("projectPath",path);open->setToolTip(path);open->setMinimumWidth(120);row->addWidget(open);auto* detail=new QVBoxLayout;auto* name=label(file.fileName(),"recentFileName");detail->addWidget(name);auto* location=label(file.absolutePath(),"recentLocation");location->setToolTip(path);detail->addWidget(location);row->addLayout(detail,1);
        auto* date=label(file.exists()?file.lastModified().toString("dd.MM.yyyy"):Language::text("Datei fehlt"),"muted");row->addWidget(date);open->setEnabled(file.isFile());auto* remove=new QPushButton(QString::fromUtf8("×"));remove->setObjectName("removeRecentProject");remove->setAccessibleName(Language::text("Aus Liste entfernen"));remove->setToolTip(remove->accessibleName());remove->setFixedSize(30,30);row->addWidget(remove);
        connect(open,&QPushButton::clicked,this,[this,path]{emit recentRequested(path);});connect(remove,&QPushButton::clicked,this,[this,path]{auto list=QSettings().value("recentProjects").toStringList();list.removeAll(path);QSettings().setValue("recentProjects",list);QTimer::singleShot(0,this,&WelcomePage::refresh);});recentLayout_->addWidget(card);
    }
    Language::translateTree(this);
}

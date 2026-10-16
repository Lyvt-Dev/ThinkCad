#include "theme.hpp"
#include <QApplication>
#include <QPalette>
#include <QRegularExpression>
#include <QMap>
#include <QPlainTextEdit>
#include <QTextDocument>
#include <QSyntaxHighlighter>
void Theme::apply(bool dark){
    qApp->setProperty("darkMode",dark);
    QPalette p;p.setColor(QPalette::Window,dark?QColor("#101722"):QColor("#f4f7fb"));p.setColor(QPalette::WindowText,dark?QColor("#e2e8f0"):QColor("#27364b"));p.setColor(QPalette::Base,dark?QColor("#17212f"):QColor("#ffffff"));p.setColor(QPalette::AlternateBase,dark?QColor("#111b29"):QColor("#f8fafc"));p.setColor(QPalette::Text,p.color(QPalette::WindowText));p.setColor(QPalette::Button,p.color(QPalette::Base));p.setColor(QPalette::ButtonText,p.color(QPalette::Text));p.setColor(QPalette::Highlight,QColor("#0d9488"));p.setColor(QPalette::HighlightedText,Qt::white);p.setColor(QPalette::Mid,dark?QColor("#475569"):QColor("#dbe4ef"));p.setColor(QPalette::PlaceholderText,dark?QColor("#aab7c9"):QColor("#64748b"));qApp->setPalette(p);
    const QString base=p.color(QPalette::Base).name(),text=p.color(QPalette::Text).name(),border=p.color(QPalette::Mid).name();
    QString style="QToolBar {spacing:6px;padding:6px;border-bottom:1px solid "+border+";}";
    style+="QToolButton {padding:6px 8px;}";
    style+="QToolButton[raisedControl=\"true\"] {padding:6px 10px;border-radius:3px;border-top:1px solid "+(dark?QString("#607086"):QString("#ffffff"))+";border-left:1px solid "+border+";border-right:2px solid "+(dark?QString("#070b12"):QString("#aab4c1"))+";border-bottom:3px solid "+(dark?QString("#070b12"):QString("#aab4c1"))+";background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 "+(dark?QString("#334155"):QString("#ffffff"))+",stop:1 "+base+");color:"+text+";}";
    style+="QToolButton[raisedControl=\"true\"]:hover {border-top-color:#0d9488;border-left-color:#0d9488;}";
    style+="QToolButton[raisedControl=\"true\"]:pressed,QToolButton[raisedControl=\"true\"]:checked {border-top:3px solid "+border+";border-bottom:1px solid "+border+";background:"+base+";}";
    style+="QToolButton#runButton {background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #169e92,stop:1 #0f766e);color:white;border-top-color:#52c7bc;border-left-color:#149b8f;border-right-color:#064e49;border-bottom-color:#064e49;}";
    style+="QToolButton#runButton:hover {background:#138c81;}";
    style+="QToolButton#runButton:pressed {background:#0f766e;border-top-color:#064e49;}";
    style+="QLabel#brandName,QLabel#startBrand {font-weight:600;font-size:17px;} QLabel#sectionTitle,QLabel#recentTitle,QLabel#welcomeTitle {font-weight:600;} QLabel#muted,QLabel#recentLocation {color:"+(dark?QString("#aab7c9"):QString("#64748b"))+";} QPushButton#componentCard,QPushButton#startExamples,QPushButton#homeNavigation,QPushButton#settingsNavigation,QPushButton#tutorialNavigation,QPushButton#resumeEditor {text-align:left;padding:6px;} QFrame#recentProjectCard {border-bottom:1px solid "+border+";} QPlainTextEdit {background:"+base+";color:"+text+";} QToolTip {background:#111;color:white;border:1px solid #555;}";
    style+="QMenu {background:#111111;color:white;border:1px solid #444;padding:4px;} QMenu::item {padding:6px 26px 6px 18px;color:white;} QMenu::item:selected {background:#303030;color:white;} QMenu::item:disabled {color:#9ca3af;} QMenu::separator {height:1px;background:#444;margin:4px;} QMenuBar::item:selected,QMenuBar::item:pressed,QTabBar::tab:hover {background:#111;color:white;}";
    qApp->setStyleSheet(style);for(auto* widget:qApp->allWidgets())if(auto* editor=qobject_cast<QPlainTextEdit*>(widget))for(auto* highlighter:editor->document()->findChildren<QSyntaxHighlighter*>())highlighter->rehighlight();
}

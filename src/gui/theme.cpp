#include "motion.hpp"
#include "theme.hpp"
#include <QApplication>
#include <QPalette>
#include <QRegularExpression>
#include <QMap>
#include <QPlainTextEdit>
#include <QTextDocument>
#include <QSyntaxHighlighter>
void Theme::apply(bool dark){
    Motion::install();
    qApp->setProperty("darkMode",dark);
    QPalette p;p.setColor(QPalette::Window,dark?QColor("#101722"):QColor("#f4f7fb"));p.setColor(QPalette::WindowText,dark?QColor("#e2e8f0"):QColor("#27364b"));p.setColor(QPalette::Base,dark?QColor("#17212f"):QColor("#ffffff"));p.setColor(QPalette::AlternateBase,dark?QColor("#111b29"):QColor("#f8fafc"));p.setColor(QPalette::Text,p.color(QPalette::WindowText));p.setColor(QPalette::Button,p.color(QPalette::Base));p.setColor(QPalette::ButtonText,p.color(QPalette::Text));p.setColor(QPalette::Highlight,QColor("#0d9488"));p.setColor(QPalette::HighlightedText,Qt::white);p.setColor(QPalette::Mid,dark?QColor("#475569"):QColor("#dbe4ef"));p.setColor(QPalette::PlaceholderText,dark?QColor("#aab7c9"):QColor("#64748b"));qApp->setPalette(p);
    const QString base=p.color(QPalette::Base).name(),text=p.color(QPalette::Text).name(),border=p.color(QPalette::Mid).name();
    QString style="QToolBar {spacing:6px;padding:6px;border-bottom:1px solid "+border+";}";
    style+="QToolButton {padding:6px 8px;}";
    style+="QToolButton[simulationControl=\"true\"] {background:"+(dark?QString("#233247"):QString("#e4ebf3"))+";color:"+text+";border:1px solid "+(dark?QString("#52647d"):QString("#b6c4d4"))+";border-radius:6px;padding:6px 10px;}";
    style+="QToolButton[simulationControl=\"true\"]:hover {background:"+(dark?QString("#24384b"):QString("#e7f4f2"))+";border-color:#168a9e;}";
    style+="QToolButton[simulationControl=\"true\"]:pressed,QToolButton[simulationControl=\"true\"]:checked {background:"+(dark?QString("#164e49"):QString("#ccfbf1"))+";border-color:#0d9488;}";
    style+="QToolButton[simulationControl=\"true\"]:focus {border-color:#168a9e;} QToolButton[simulationControl=\"true\"]:disabled {color:#94a3b8;}";
    style+="QToolButton#runButton {background:#0d9488;border-color:#0f766e;color:white;} QToolButton#runButton:hover {background:#0f766e;} QToolButton#runButton:pressed {background:#115e59;}";
    style+="QLabel#brandName,QLabel#startBrand {font-weight:600;font-size:17px;} QLabel#sectionTitle,QLabel#recentTitle,QLabel#welcomeTitle {font-weight:600;} QLabel#muted,QLabel#recentLocation,QLabel#startVersion {color:"+(dark?QString("#aab7c9"):QString("#64748b"))+";} QPushButton#componentCard,QPushButton#startExamples,QPushButton#homeNavigation,QPushButton#settingsNavigation,QPushButton#tutorialNavigation,QPushButton#resumeEditor {text-align:left;padding:6px;} QFrame#recentProjectCard {border-bottom:1px solid "+border+";} QPlainTextEdit {background:"+base+";color:"+text+";} QToolTip {background:#111;color:white;border:1px solid #555;}";
    style+="QMenu {background:#111111;color:white;border:1px solid #444;padding:4px;} QMenu::item {padding:6px 26px 6px 18px;color:white;} QMenu::item:selected {background:#303030;color:white;} QMenu::item:disabled {color:#9ca3af;} QMenu::separator {height:1px;background:#444;margin:4px;} QMenuBar::item:selected,QMenuBar::item:pressed,QTabBar::tab:hover {background:#111;color:white;}";
    qApp->setStyleSheet(style);for(auto* widget:qApp->allWidgets())if(auto* editor=qobject_cast<QPlainTextEdit*>(widget))for(auto* highlighter:editor->document()->findChildren<QSyntaxHighlighter*>())highlighter->rehighlight();
}

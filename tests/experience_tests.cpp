#include "mainwindow.hpp"
#include "startpage.hpp"
#include "tutorialcourse.hpp"
#include "componentdesign.hpp"
#include "language.hpp"
#include "appimageupdate.hpp"
#include "theme.hpp"
#include <QtTest>
#include <QApplication>
#include <QDockWidget>
#include <QAction>
#include <QPushButton>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QSlider>
#include <QTimer>
#include <QListWidget>
#include <QProgressBar>
#include <QDialog>
#include <QDialogButtonBox>
#include <QInputDialog>
#include <QPlainTextEdit>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QGraphicsPathItem>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QSettings>
#include <QComboBox>
#include <QMenu>
#include <cmath>
#include <QCryptographicHash>
#include <QFile>
#include <QDir>
using namespace thinkercad;
class ExperienceTests:public QObject {
 Q_OBJECT
 QTemporaryDir temporary_;
 void shot(QWidget& widget,const QString& name){auto path=qEnvironmentVariable("THINKCAD_REVIEW_DIR");if(path.isEmpty())return;QDir().mkpath(path);QCoreApplication::processEvents();if(qobject_cast<MainWindow*>(&widget))widget.resize(1400,950);QTest::qWait(60);if(auto* w=qobject_cast<MainWindow*>(&widget))w->canvas()->fitCircuit();QTest::qWait(50);QVERIFY(widget.grab().save(path+"/"+name+".png"));}
 static void drop(CircuitCanvas* canvas,PartKind kind,const QString& model={},QPoint point={160,160}){QMimeData data;data.setData("application/x-thinkcad-component",QJsonDocument(QJsonObject{{"kind",int(kind)},{"model",model}}).toJson());QDragEnterEvent enter(point,Qt::CopyAction,&data,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(canvas->viewport(),&enter);QDropEvent event(point,Qt::CopyAction,&data,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(canvas->viewport(),&event);}
private slots:
 void initTestCase(){QCoreApplication::setOrganizationName("ThinkCadExperienceTests");QSettings::setDefaultFormat(QSettings::IniFormat);QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,temporary_.path());qputenv("THINKCAD_RECOVERY_DIR",temporary_.filePath("recovery").toUtf8());}
 void init(){QSettings().clear();Language::set("de");ComponentDesign::set("blocks");Theme::apply(false);}
 void dragPlacementDoesNotChangeTab(){MainWindow window;window.show();QTest::qWait(100);auto* parts=window.findChild<QDockWidget*>("partsDock");auto* properties=window.findChild<QDockWidget*>("inspectorDock");QVERIFY(parts&&properties);parts->raise();auto before=window.circuit().parts().size();auto* card=window.findChild<QPushButton*>("componentCard");QVERIFY(card);QTest::mouseClick(card,Qt::LeftButton);QCOMPARE(window.circuit().parts().size(),before);drop(window.canvas(),PartKind::Esp8266,"arduino-nano");QCOMPARE(window.circuit().parts().size(),before+1);QCOMPARE(window.circuit().parts().back().model,std::string("arduino-nano"));QVERIFY(parts->visibleRegion().boundingRect().height()>0);window.canvas()->selectPart(window.circuit().parts().back().id);QVERIFY(parts->visibleRegion().boundingRect().height()>0);window.canvas()->history()->undo();QCOMPARE(window.circuit().parts().size(),before);window.canvas()->history()->redo();QCOMPARE(window.circuit().parts().size(),before+1);window.startSimulation();QTest::qWait(50);drop(window.canvas(),PartKind::Led);QCOMPARE(window.circuit().parts().size(),before+1);window.stopSimulation();shot(window,"v12-workspace");}
 void simulationClickPassesThroughWire(){MainWindow window;window.show();QVERIFY(window.loadExample("button_led"));auto* canvas=window.canvas();std::string button,wire;for(const auto& p:window.circuit().parts())if(p.kind==PartKind::Button)button=p.id;QVERIFY(!button.empty()); // Replace below with an existing wire path; endpoints remain electrically unchanged.
 window.startSimulation();QTRY_VERIFY_WITH_TIMEOUT(window.simulationTime()>10000,15000);QGraphicsItem* buttonItem=nullptr;QGraphicsPathItem* wireItem=nullptr;for(auto* item:canvas->scene()->items()){if(item->data(0)=="part"&&item->data(1).toString()==QString::fromStdString(button))buttonItem=item;if(item->data(0)=="wire")wireItem=static_cast<QGraphicsPathItem*>(item);if(item->data(0)=="wire")QVERIFY(!(item->flags()&QGraphicsItem::ItemIsSelectable));if(item->data(0)=="part"&&(item->data(1)=="board"||window.circuit().part(item->data(1).toString().toStdString()).kind==PartKind::Resistor))QVERIFY(!(item->flags()&QGraphicsItem::ItemIsSelectable));}QVERIFY(buttonItem&&wireItem);canvas->fitCircuit();auto center=buttonItem->mapToScene(buttonItem->boundingRect().center());QPainterPath path(center-QPointF(100,0));path.lineTo(center+QPointF(100,0));wireItem->setPath(path);wireItem->setZValue(50);QTest::mouseClick(canvas->viewport(),Qt::LeftButton,Qt::NoModifier,canvas->mapFromScene(center));QVERIFY(window.circuit().part(button).pressed);window.stopSimulation();for(auto* item:canvas->scene()->items())if(item->data(0)=="wire")QVERIFY(item->flags()&QGraphicsItem::ItemIsSelectable);}
 void localAutosaveWritesNamedProject(){MainWindow window;window.show();auto* timer=window.findChild<QTimer*>("localSaveTimer");QVERIFY(timer);QCOMPARE(timer->interval(),60000);auto path=temporary_.filePath("autosave.thinkcad");QVERIFY(window.saveProjectTo(path));window.canvas()->mutate("edit",[](auto& d){d.part("board").x=123;});window.saveLocalAutomatically();QCOMPARE(ProjectIO::load(path).circuit.part("board").x,123.);QSettings().setValue("localAutosave",false);window.canvas()->mutate("edit",[](auto& d){d.part("board").x=456;});window.saveLocalAutomatically();QCOMPARE(ProjectIO::load(path).circuit.part("board").x,123.);}
 void exactCustomSpeed(){MainWindow window;window.show();auto* input=window.findChild<QDoubleSpinBox*>("simulationSpeedInput");auto* slider=window.findChild<QSlider*>("simulationSpeed");QVERIFY(input&&slider);input->setValue(1.375);QCOMPARE(input->value(),1.375);QCOMPARE(slider->value(),qRound(40+40*std::log10(1.375)));slider->setValue(0);QCOMPARE(input->value(),.1);slider->setValue(80);QCOMPARE(input->value(),10.);}
 void noteContextMenuAndPersistence(){MainWindow window;window.show();QTest::qWait(100);auto* canvas=window.canvas();auto before=window.circuit().parts().size();QTimer::singleShot(60,&window,[&]{auto* menu=qobject_cast<QMenu*>(QApplication::activePopupWidget());QVERIFY(menu);QTimer::singleShot(40,&window,[&]{auto* dialog=qobject_cast<QInputDialog*>(QApplication::activeModalWidget());QVERIFY(dialog);dialog->setTextValue("NFC reader\nSDA = GPIO21, SCL = GPIO22");dialog->accept();});menu->actions().front()->trigger();menu->close();});QTest::mouseClick(canvas->viewport(),Qt::RightButton,Qt::NoModifier,QPoint(25,25));QTRY_COMPARE(window.circuit().parts().size(),before+1);auto id=window.circuit().parts().back().id;QCOMPARE(window.circuit().part(id).kind,PartKind::Note);auto path=temporary_.filePath("notes.thinkcad");QVERIFY(window.saveProjectTo(path));QCOMPARE(ProjectIO::load(path).circuit.part(id).model,std::string("NFC reader\nSDA = GPIO21, SCL = GPIO22"));canvas->history()->undo();QCOMPARE(window.circuit().parts().size(),before);canvas->history()->redo();QCOMPARE(window.circuit().parts().size(),before+1);shot(window,"v12-notes");}
 void courseUnlocksFromActualSimulation(){MainWindow window;window.show();window.findChild<QAction*>("openLearningCourse")->trigger();auto* course=window.findChild<TutorialCourse*>();QVERIFY(course);QVERIFY(TutorialCourse::available("led_blink"));QVERIFY(!TutorialCourse::available("button_led"));QVERIFY(!TutorialCourse::complete("nfc_reader"));course->findChild<QListWidget*>("courseLessons")->setCurrentRow(1);QVERIFY(!course->findChild<QPushButton*>("startLesson")->isEnabled());course->findChild<QListWidget*>("courseLessons")->setCurrentRow(0);shot(window,"v12-learning-course");course->findChild<QPushButton*>("startLesson")->click();QVERIFY(!window.startPageVisible());window.startSimulation();QTRY_VERIFY_WITH_TIMEOUT(QSettings().value("course/completed").toStringList().contains("led_blink"),15000);QVERIFY(TutorialCourse::available("button_led"));QVERIFY(!TutorialCourse::available("analog_threshold"));window.stopSimulation();shot(window,"v12-lesson");}
 void settingsCancelAndSave(){MainWindow window;window.show();auto* action=window.findChild<QAction*>("openPreferences");QTimer::singleShot(80,&window,[&]{auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());QVERIFY(dialog);auto* input=dialog->findChild<QLineEdit*>("preferencesCompiler");input->setText("cancelled-compiler");shot(*dialog,"v12-settings");dialog->reject();});action->trigger();QVERIFY(QSettings().value("compiler").toString()!="cancelled-compiler");QTimer::singleShot(80,&window,[&]{auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());dialog->findChild<QComboBox*>("preferencesLanguage")->setCurrentIndex(1);dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Save)->click();});action->trigger();QCOMPARE(Language::current(),QString("en"));}
 void oledHasVisiblePinLabels(){MainWindow window;window.show();QVERIFY(window.loadExample("animation"));ComponentDesign::set("realistic");window.canvas()->rebuild();QStringList labels;for(auto* item:window.canvas()->scene()->items())if(item->data(0)=="pinLabel")labels<<item->data(2).toString();for(auto name:{"GND","VCC","SDA","SCL"})QVERIFY(labels.contains(name));shot(window,"v12-oled-pin-labels");}
 void appimageAtomicReplacementRejectsTampering(){auto old=temporary_.filePath("portable.AppImage"),download=temporary_.filePath("update.AppImage");auto write=[](const QString& path,const QByteArray& bytes){QFile f(path);if(!f.open(QIODevice::WriteOnly))return false;return f.write(bytes)==bytes.size();};QByteArray original="previous image",replacement(100,0);replacement.replace(0,4,QByteArray("\x7f" "ELF",4));replacement.replace(8,3,QByteArray("AI\x02",3));replacement[18]=62;QVERIFY(write(old,original));QVERIFY(write(download,replacement));QString error;QVERIFY(!replaceAppImage(old,download,QByteArray(32,'a'),&error));QFile before(old);QVERIFY(before.open(QIODevice::ReadOnly));QCOMPARE(before.readAll(),original);before.close();QVERIFY(replaceAppImage(old,download,QCryptographicHash::hash(replacement,QCryptographicHash::Sha256),&error));QFile after(old);QVERIFY(after.open(QIODevice::ReadOnly));QCOMPARE(after.readAll(),replacement);}
};
QTEST_MAIN(ExperienceTests)
#include "experience_tests.moc"

#pragma once
#include "circuitcanvas.hpp"
#include "language.hpp"
#include <QFormLayout>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QColorDialog>
#include <QSignalBlocker>
// Shared editing controls. All changes pass through undoable document snapshots.
class ComponentOptions:public QWidget {
    CircuitCanvas* canvas_;std::string id_;QComboBox* contacts_;QComboBox* meter_;QCheckBox* rails_;QPushButton* color_;
public:
    explicit ComponentOptions(CircuitCanvas* canvas):canvas_(canvas){setObjectName("componentOptions");auto* form=new QFormLayout(this);form->setContentsMargins(0,0,0,0);
        contacts_=new QComboBox;contacts_->setObjectName("breadboardContacts");for(int columns:{17,30,63})contacts_->addItem(QString::number(columns*10)+" · "+QString::number(columns)+" "+Language::text("Spalten"),columns);form->addRow(Language::text("Kontakte (ohne Stromschienen)"),contacts_);
        rails_=new QCheckBox(Language::text("Stromschienen"));rails_->setObjectName("breadboardPowerRails");form->addRow(rails_);
        meter_=new QComboBox;meter_->setObjectName("multimeterMode");meter_->addItems({Language::text("Gleichspannung · V"),Language::text("Gleichstrom · A"),Language::text("Widerstand · Ω (spannungsfrei)")});form->addRow(Language::text("Messmodus"),meter_);connect(meter_,qOverload<int>(&QComboBox::currentIndexChanged),this,[this](int mode){if(id_.empty()||!canvas_->editingEnabled())return;const auto id=id_;canvas_->mutate(Language::text("Messmodus ändern"),[&](auto& d){d.part(id).parameters["mode"]=mode;});});
        color_=new QPushButton(Language::text("Farbe wählen…"));color_->setObjectName("componentColor");form->addRow(color_);
        connect(contacts_,qOverload<int>(&QComboBox::currentIndexChanged),this,[this]{configureBoard();});connect(rails_,&QCheckBox::toggled,this,[this]{configureBoard();});
        connect(color_,&QPushButton::clicked,this,[this]{if(id_.empty())return;const auto id=id_;const auto color=QColorDialog::getColor(QColor(QString::fromStdString(canvas_->document().part(id).color)),this,Language::text("LED-Farbe"));if(color.isValid()&&canvas_->editingEnabled())canvas_->mutate(Language::text("Bauteilfarbe ändern"),[&](auto& d){d.part(id).color=color.name().toStdString();});});hide();
    }
    void configureBoard(){if(id_.empty()||!canvas_->editingEnabled())return;const auto id=id_;const int columns=contacts_->currentData().toInt();const bool rails=rails_->isChecked();canvas_->mutate(Language::text("Breadboard konfigurieren"),[&](auto& doc){auto& p=doc.part(id);p.parameters["columns"]=columns;p.parameters["rails"]=rails?1:0;const auto pins=thinkercad::CircuitDocument::pins(p);std::vector<std::string> remove;for(const auto& w:doc.wires())for(const auto& e:{w.a,w.b})if(e.part==id&&std::find(pins.begin(),pins.end(),e.pin)==pins.end())remove.push_back(w.id);for(const auto& w:remove)doc.removeWire(w);for(auto& w:const_cast<std::vector<thinkercad::CircuitWire>&>(doc.wires()))if(w.a.part==id||w.b.part==id)w.direct=false;});}
    void refresh(const QString& selected){id_=selected.toStdString();if(id_.empty()){hide();return;}const auto& p=canvas_->document().part(id_);const bool meter=p.kind==thinkercad::PartKind::Voltmeter&&p.model=="multimeter";const bool board=p.kind==thinkercad::PartKind::Breadboard30;const bool color=p.kind==thinkercad::PartKind::Led||p.kind==thinkercad::PartKind::Button||p.kind==thinkercad::PartKind::SevenSegment||(p.kind==thinkercad::PartKind::Module&&(p.model=="led-matrix"||p.model=="led-bar10"||p.model=="clock-display"));
        auto* form=static_cast<QFormLayout*>(layout());form->setRowVisible(contacts_,board);form->setRowVisible(rails_,board);form->setRowVisible(color_,color);form->setRowVisible(meter_,meter);setVisible(board||color||meter);setEnabled(canvas_->editingEnabled());QSignalBlocker a(contacts_),b(rails_),c(meter_);if(meter)meter_->setCurrentIndex(int(thinkercad::parameter(p,"mode",0)));if(board){contacts_->setCurrentIndex(contacts_->findData(thinkercad::breadboardColumns(p)));rails_->setChecked(thinkercad::breadboardRails(p));}color_->setStyleSheet("border-left: 8px solid "+QString::fromStdString(p.color));
    }
};

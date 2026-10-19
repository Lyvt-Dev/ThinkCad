#pragma once
#include "circuitcanvas.hpp"
#include "thinkercad/modules.hpp"
#include <QFrame>
#include <QFormLayout>
#include <QLabel>
#include <QSlider>
#include <QPushButton>
#include <QHBoxLayout>
#include <cmath>

// Live stimulus controls for sensors and configurable signal sources. Mechanical
// controls use direct canvas gestures instead; this panel never edits the circuit.
class RuntimeControls final: public QFrame {
public:
    RuntimeControls(CircuitCanvas* canvas,const thinkercad::CircuitPart& part):QFrame(canvas,Qt::Tool){
        using namespace thinkercad;
        setObjectName("runtimeControls");setAttribute(Qt::WA_DeleteOnClose);
        setWindowTitle(QString::fromStdString(part.kind==PartKind::Module?ModuleRegistry::get(part.model).label:componentSpec(part.kind).label));
        auto* form=new QFormLayout(this);form->setContentsMargins(16,12,16,12);
        auto slider=[&,id=part.id](const QString& label,const QString& unit,const std::string& key,double low,double high,double initial,bool integer){
            if(high<=low)return;auto* row=new QWidget(this);auto* layout=new QHBoxLayout(row);layout->setContentsMargins(0,0,0,0);
            auto* input=new QSlider(Qt::Horizontal,row);input->setObjectName("live_"+QString::fromStdString(key.empty()?"value":key));input->setMinimumWidth(190);input->setRange(0,1000);input->setValue(qRound((initial-low)/(high-low)*1000));
            auto* readout=new QLabel(QString::number(initial,'g',5)+" "+unit,row);readout->setMinimumWidth(95);layout->addWidget(input);layout->addWidget(readout);form->addRow(label,row);
            connect(input,&QSlider::valueChanged,this,[canvas,id,key,low,high,integer,readout,unit](int position){double value=low+(high-low)*position/1000.;if(integer)value=std::round(value);if(key.empty())canvas->changeValue(id,value);else canvas->changeParameter(id,key,value);readout->setText(QString::number(value,'g',5)+" "+unit);});
        };
        if(part.kind==PartKind::Module){for(const auto& field:ModuleRegistry::get(part.model).parameters){if(field.name=="rows"||field.name=="columns"||field.name=="pixels"||field.name=="cells")continue;slider(QString::fromStdString(field.label),QString::fromStdString(field.unit),field.name,field.min,field.max,parameter(part,field.name,field.initial),field.unit=="count");}}
        else {const auto& spec=componentSpec(part.kind);slider(QString::fromStdString(spec.label),QString::fromStdString(spec.unit),{},spec.min,spec.max,part.value,false);}
        connect(canvas,&CircuitCanvas::documentChanged,this,[this,canvas,id=part.id,model=part.model]{const auto& parts=canvas->document().parts();if(std::none_of(parts.begin(),parts.end(),[&](const auto& p){return p.id==id&&p.model==model;}))close();});
        adjustSize();
    }
};

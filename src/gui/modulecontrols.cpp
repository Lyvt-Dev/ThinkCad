#include "modulecontrols.hpp"
#include "circuitcanvas.hpp"
#include "language.hpp"
#include "thinkercad/modules.hpp"
#include <QFormLayout>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QLabel>
#include <QSignalBlocker>
#include <algorithm>
using namespace thinkercad;
ModuleControls::ModuleControls(CircuitCanvas* canvas,std::string id,QWidget* parent):QWidget(parent),canvas_(canvas),id_(std::move(id)){
    setObjectName("moduleControls");auto* form=new QFormLayout(this);form->setContentsMargins(0,0,0,0);form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);const auto& part=canvas_->document().part(id_);model_=part.model;const auto& model=ModuleRegistry::get(part.model);auto* label=new QLabel(Language::text(QString::fromStdString(model.label)));label->setWordWrap(true);form->addRow(label);
    for(const auto& field:model.parameters){auto* input=new QDoubleSpinBox;input->setObjectName("moduleParam_"+QString::fromStdString(field.name));input->setProperty("parameter",QString::fromStdString(field.name));input->setDecimals(field.unit=="cm"||field.unit=="g"||field.unit=="°/s"?3:2);if((model.implementation=="ws2812"&&field.name=="pixels")||field.unit=="count")input->setDecimals(0);input->setRange(field.min,field.max);input->setSuffix(" "+QString::fromStdString(field.unit));input->setKeyboardTracking(false);form->addRow(Language::text(QString::fromStdString(field.label)),input);connect(input,qOverload<double>(&QDoubleSpinBox::valueChanged),this,[this,name=field.name](double value){if(matchesDocument())canvas_->changeParameter(id_,name,value);});}
    if(model.bus=="i2c"){auto* input=new QSpinBox;input->setObjectName("moduleAddress");input->setDisplayIntegerBase(16);input->setPrefix("0x");input->setRange(model.builtin?model.address:8,model.builtin?model.address+model.addressCount-1:119);form->addRow(Language::text("I²C-Adresse"),input);connect(input,qOverload<int>(&QSpinBox::valueChanged),this,[this](int value){if(matchesDocument())canvas_->changeParameter(id_,"address",value);});}
    refresh();
}
void ModuleControls::refresh(){
    const auto& parts=canvas_->document().parts();const auto found=std::find_if(parts.begin(),parts.end(),[this](const auto& p){return p.id==id_;});if(found==parts.end()){setEnabled(false);return;}const auto& model=ModuleRegistry::get(found->model);for(const auto& field:model.parameters){auto* input=findChild<QDoubleSpinBox*>("moduleParam_"+QString::fromStdString(field.name));if(input&&(field.name=="pixels"||field.name=="rows"||field.name=="columns"))input->setEnabled(canvas_->editingEnabled());if(input&&!input->hasFocus()){const QSignalBlocker block(input);input->setValue(parameter(*found,field.name,field.initial));}}
    if(auto* address=findChild<QSpinBox*>("moduleAddress")){const QSignalBlocker block(address);address->setValue(int(parameter(*found,"address",model.address)));address->setEnabled(canvas_->editingEnabled());}
}

bool ModuleControls::matchesDocument() const {
    const auto& parts=canvas_->document().parts();
    return std::any_of(parts.begin(),parts.end(),[this](const auto& part){return part.id==id_&&part.kind==PartKind::Module&&part.model==model_;});
}

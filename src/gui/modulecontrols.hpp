#pragma once
#include <QWidget>
#include <string>
class CircuitCanvas;
class ModuleControls final:public QWidget {
public:
    ModuleControls(CircuitCanvas* canvas,std::string id,QWidget* parent=nullptr);
    const std::string& partId()const{return id_;}
    const std::string& modelId()const{return model_;}
    void refresh();
private:
    CircuitCanvas* canvas_;
    std::string id_,model_;
};

#pragma once
#include "projectio.hpp"
#include <QList>
struct CircuitExample {QString id,title,description;};
class ExampleCatalog {
public:
    static QList<CircuitExample> entries();
    static SavedProject create(const QString& id);
};

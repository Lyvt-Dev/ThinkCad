#pragma once
#include "examples.hpp"
class StarterProjects {
  public:
    static QList<CircuitExample> entries();
    static SavedProject create(const QString &id);
    static QString instructions(const QString &id);
};

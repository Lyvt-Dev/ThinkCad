#pragma once
#include "projectshare.hpp"
#include <optional>
#include <functional>
class QWidget;
void showProjectShare(QWidget *, SavedProject, const QStringList &libraryDirectories,
                      std::function<void(bool)> onCreated = {});
std::optional<DecodedShare> showShareImport(QWidget *);

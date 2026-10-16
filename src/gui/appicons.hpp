#pragma once
#include <QIcon>
namespace AppIcons {
enum class Symbol { NewProject, Folder, Save, Home, Settings, Tutorial, Circuit, Reset, Play, Stop, Pause, Step, Wire, Palette };
QIcon icon(Symbol symbol);
}

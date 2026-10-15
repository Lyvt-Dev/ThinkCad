#include "appicons.hpp"
#include <QApplication>
#include <QStyle>
QIcon AppIcons::icon(Symbol symbol){
    QStyle::StandardPixmap image=QStyle::SP_FileIcon;
    switch(symbol){
    case Symbol::NewProject:image=QStyle::SP_FileIcon;break;
    case Symbol::Folder:image=QStyle::SP_DirOpenIcon;break;
    case Symbol::Save:image=QStyle::SP_DialogSaveButton;break;
    case Symbol::Home:image=QStyle::SP_DirHomeIcon;break;
    case Symbol::Settings:image=QStyle::SP_FileDialogDetailedView;break;
    case Symbol::Tutorial:image=QStyle::SP_DialogHelpButton;break;
    case Symbol::Circuit:image=QStyle::SP_FileIcon;break;
    case Symbol::Reset:image=QStyle::SP_BrowserReload;break;
    case Symbol::Play:image=QStyle::SP_MediaPlay;break;
    case Symbol::Stop:image=QStyle::SP_MediaStop;break;
    }
    return QApplication::style()->standardIcon(image);
}

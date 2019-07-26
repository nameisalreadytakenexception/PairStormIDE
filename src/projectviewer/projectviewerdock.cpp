#include "projectviewerdock.h"

#include <QDockWidget>
#include <QException>
#include <QTreeView>
#include <QWidget>
#include <QDir>

#include "mainwindow.h"
#include "projectviewermodel.h"
#include "projecttreeview.h"

ProjectViewerDock::ProjectViewerDock(QWidget *pParent): QDockWidget(pParent)
{
    setWindowTitle("Project Viewer");
    QStringList filters;
    filters << "*.txt"<<"*.cpp"<<"*.h"<<"*.json"<<"*.c"<<"*.hpp";

    //for testing use own path
    QDir dir("C:\\Users\\igord\\Desktop\\PairStormIDE");
    ProjectViewerModel* fileSystemModel = new ProjectViewerModel(dir,filters);
    pTreeViewer = new ProjectTreeView(fileSystemModel);
    setWidget(pTreeViewer);
    auto pMainWindow = dynamic_cast<MainWindow*>(pParent);    
}

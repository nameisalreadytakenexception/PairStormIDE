#include "browserdialog.h"
#include "ui_browserdialog.h"

#include <QDir>
#include <QSizePolicy>
#include <QMdiSubWindow>
#include <QStandardPaths>
#include <QWebEngineView>
#include <QWebEnginePage>

#include "mdiarea.h"
#include "documentationsearch.h"
#include "connectionmanager.h"
#include "documentationengine.h"
#include "documentationviewer.h"
#include "htmlcontentgenerator.h"

BrowserDialog::BrowserDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BrowserDialog)
{
    ui->setupUi(this);

    resize(800,600);
    mConnectionManager = new ConnectionManager(this);
    ui->mMDIArea->setViewMode(QMdiArea::ViewMode::TabbedView);

}

BrowserDialog::~BrowserDialog()
{
    delete ui;
//    for(auto &a : ui->mMDIArea->subWindowList())
//    {
//        delete a;
//    }
//    qDebug()<<"Delete browser";

//    bool result;
//    QDir dir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);

//    result = dir.cd("PairStorm");
//    result = dir.cd("temp");

//    if(!result)
//    {
//        return;
//    }

//    dir.setNameFilters(QStringList() << "*.html");
//    dir.setFilter(QDir::Files);

//    foreach(QString dirFile, dir.entryList())
//    {
//        dir.remove(dirFile);
//    }
}

void BrowserDialog::createNewTab(const QString &keyword)
{
    DocumentationViewer *newWindow = new DocumentationViewer(this);
    newWindow->loadReferenceDocumentation(keyword);
    auto temp = ui->mMDIArea->addSubWindow(newWindow);
    temp->setWindowState(Qt::WindowState::WindowMaximized);
    newWindow->setAttribute(Qt::WA_DeleteOnClose);

}

void BrowserDialog::createEmptyTab()
{
    DocumentationViewer *newWindow = new DocumentationViewer(this);
    newWindow->loadReferenceDocumentation();
    auto temp = ui->mMDIArea->addSubWindow(newWindow);
    temp->setWindowState(Qt::WindowState::WindowMaximized);
    newWindow->setAttribute(Qt::WA_DeleteOnClose);
}


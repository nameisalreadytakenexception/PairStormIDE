#include "documentmanager.h"

#include <QMessageBox>
#include <QSplitter>
#include <QMdiArea>
#include <QVector>

#include "textdocumentholder.h"
#include "usermessages.h"
#include "filemanager.h"
#include "codeeditor.h"
#include "utils.h"

DocumentManager::DocumentManager()
{
    mpSplitter = new QSplitter;
    splitWindow();
}

void DocumentManager::splitWindow()
{
    QMdiArea *pNewArea = createMdiArea();

    mpSplitter->addWidget(pNewArea);
    mDocAreas.push_back(pNewArea);
}

QSplitter* DocumentManager::getSplitter()
{
    return mpSplitter;
}

void DocumentManager::openDocument(const QString &fileName, bool load)
{
    // create new view
    CodeEditor *newView = new CodeEditor;
    newView->setFileName(fileName);

    // place view
    selectAreaForPlacement()->addSubWindow(newView);
    newView->setWindowState(Qt::WindowMaximized);

    TextDocumentHolder *pOpenedDoc = openedDoc(fileName);

    if (pOpenedDoc)
    {
        newView->setDocument(pOpenedDoc->document());
    }
    else
    {
        QScopedPointer<TextDocumentHolder> newDoc(new TextDocumentHolder(fileName));
        mDocuments.push_back(newDoc);
        newDoc->incrementViewCount();
        newView->setDocument(newDoc->document());
    }

    if (load)
    {
        loadFile(newView, fileName);
    }
}

void DocumentManager::loadFile(CodeEditor *newView, const QString &fileName)
{
    QString readResult;

    try
    {
        readResult = FileManager().readFromFile(fileName);
    }
    catch (const QException&)
    {
        throw;
    }

    newView->setPlainText(readResult);
}

QMdiArea* DocumentManager::createMdiArea()
{
    QMdiArea *pMdiArea = new QMdiArea;
    pMdiArea->setTabsClosable(true);
    pMdiArea->setViewMode(QMdiArea::TabbedView);
    return pMdiArea;
}

QMdiArea* DocumentManager::selectAreaForPlacement()
{
    // temp solution
    return mDocAreas.front();
}

TextDocumentHolder* DocumentManager::openedDoc(const QString &fileName)
{
    // temp
    return nullptr;
}

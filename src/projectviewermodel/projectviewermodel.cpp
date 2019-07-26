#include "projectviewermodel.h"

#include <QImage>
#include <QStringList>
#include <QDebug>

ProjectViewerModel::ProjectViewerModel(const QDir &directory, const QStringList &filters, QObject *parent)
    : QFileSystemModel (parent)
{
    setRootPath(directory.path());
    setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
    setNameFilterDisables(false);
    setNameFilters(filters);

    QDirIterator it(":/img/", QDirIterator::FollowSymlinks);
    while (it.hasNext())
    {
        it.next();
        qDebug()<<it.path();
        mImages.insert(it.fileInfo().baseName().toStdString(),new QIcon(it.path()+it.fileName()));
    }
}

ProjectViewerModel::~ProjectViewerModel()
{
    foreach(auto &temp,mImages)
    {
        delete temp;
    }
}

QVariant ProjectViewerModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
    {
        return QVariant();
    }

    QFileInfo info = fileInfo(index);
    switch (role)
    {
    case Qt::DisplayRole:
        return info.fileName();
    case Qt::DecorationRole:
        if(isDir(index))
        {
            return *mImages["dir"];
        }
        else
        {
            return *mImages[fileInfo(index).suffix().toStdString()];
        }
    }
    return QVariant();
}

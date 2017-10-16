#ifndef FILELIST_H
#define FILELIST_H

#include <QTreeView>
#include <QtWidgets/QWidget>
#include <QPoint>
extern "C"
{
#include <TagFolder.h>
}


class FileList : public QTreeView
{
public:
    FileList(QWidget *centralWidget):QTreeView(centralWidget){Q_UNUSED(centralWidget)}

    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void setTagFolder(TagFolder *folder);

private:
    QPoint dragStartPosition;
    TagFolder *folder;
};

#endif // FILELIST_H

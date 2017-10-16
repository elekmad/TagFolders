#include "filelist.h"
#include <QMouseEvent>
#include <QDrag>
#include <QMimeData>
#include <QDropEvent>
#include <qdebug.h>
#include <QApplication>
#include <QItemSelectionModel>
#include "mainwindow.h"

void FileList::mousePressEvent(QMouseEvent *event)
{
    QTreeView::mousePressEvent(event);
    if (event->button() == Qt::LeftButton)
        dragStartPosition = event->pos();
    qInfo() << "Drag Drop ?" << event->pos();
}

void FileList::mouseMoveEvent(QMouseEvent *event)
 {
    QTreeView::mouseMoveEvent(event);
     if (!(event->buttons() & Qt::LeftButton))
         return;
     if ((event->pos() - dragStartPosition).manhattanLength()
          < QApplication::startDragDistance())
         return;

     QItemSelectionModel *selection_model = this->selectionModel();
     QString uri;
     if(selection_model != NULL && !selection_model->selectedIndexes().isEmpty())
     {
         MainWindow *window = qobject_cast<MainWindow*>(this->parentWidget()->parentWidget());

         int file_id = window->get_file_id_from_row_id(selection_model->selectedIndexes().first().row());
         File *file = TagFolder_get_file_with_id(folder, file_id);

         window->prepare_file_to_open(file, uri);
     }

     QDrag *drag = new QDrag(this);
     QMimeData *mimeData = new QMimeData;

     mimeData->setData(tr("text/uri-list"), QByteArray(uri.toLocal8Bit().data()));
     drag->setMimeData(mimeData);

     Qt::DropAction dropAction = drag->exec(Qt::CopyAction | Qt::MoveAction);
     qInfo() << "drag drop filename" << uri;
     qInfo() << "action = " << dropAction;
}

void FileList::setTagFolder(TagFolder *folder)
{
    this->folder = folder;
}


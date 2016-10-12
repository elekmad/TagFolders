#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "GetExistingTagName.h"
#include <qwidget.h>
#include <qlayout.h>
#include <qlogging.h>
#include <qdebug.h>
#include <qcheckbox.h>
#include <QObject>
#include <QMenu>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    file_model = NULL;
    ui->setupUi(this);
    TagFolder_init(&folder);
    QString path("/home/damien/dev/TagFolders/test");
    TagFolder_setup_folder(&folder, path.toLocal8Bit().data());
    reload_file_list();
    reload_tags_list();
}

MainWindow::~MainWindow()
{
    delete ui;
    TagFolder_finalize(&folder);
    if(file_model != NULL)
        delete file_model;
}

void MainWindow::reload_file_list(void)
{
    File *current_files;
    current_files = TagFolder_list_current_files(&folder);
    if(current_files != NULL)
    {
        File *ptr = current_files;
        QStringList list;
        QStringListModel *m = new QStringListModel();
        while(ptr != NULL)
        {
            list << ptr->name;
            ptr = File_get_next(ptr);
        }
        m->setStringList(list);

        ui->FileList->setModel(m);
        File_free(current_files);
        if(file_model != NULL)
            delete file_model;
        file_model = m;
    }
}

void MainWindow::reload_tags_list(void)
{
    Tag *ltags;
    ltags = TagFolder_list_tags(&folder);
    if(ltags != NULL)
    {
        QVBoxLayout *TagsListLayout = new QVBoxLayout();
        TagsListLayout->setSizeConstraint(TagsListLayout->SetMinimumSize);
        QWidget *TagsList = this->findChild<QWidget*>("TagsList");
        Tag *ptr = ltags;

        while(ptr != NULL)
        {
            QCheckBox *checkBox;
            QString checkbox_name(ptr->name);
            checkBox = new QCheckBox;
            checkBox->setObjectName(checkbox_name);
            checkBox->setText(checkbox_name);
            TagsListLayout->addWidget(checkBox);
            connect(checkBox, SIGNAL(clicked(bool)), this, SLOT(on_checkBox_clicked(bool)));

            ptr = Tag_get_next(ptr);
        }
        TagsList->setLayout(TagsListLayout);

        ui->retranslateUi(this);
        QMetaObject::connectSlotsByName(this);
    }
}

void MainWindow::on_checkBox_clicked(bool checked)
{
    QObject *o = sender();
    QCheckBox *chk = qobject_cast<QCheckBox*>(o);
    qInfo() << o << chk->text() << "checked : " << checked;
    if(checked)
        TagFolder_select_tag(&folder, chk->text().toLocal8Bit().data());
    else
        TagFolder_unselect_tag(&folder, chk->text().toLocal8Bit().data());
    reload_file_list();
}

void MainWindow::on_FileList_customContextMenuRequested(const QPoint &pos)
{
    QListView *filelist = this->findChild<QListView*>("FileList");
    QMenu *menu = new QMenu(qobject_cast<QWidget*>(filelist));
    QAction *action;
    action = menu->addAction("Ajouter un tag");
    connect(action, SIGNAL(triggered()), this, SLOT(add_tag_to_file_window()));
    connect(action, SIGNAL(triggered()), menu, SLOT(close()));
    filelist->selectionModel()->selectedIndexes();
    QString file_selected = filelist->selectionModel()->selectedIndexes().first().data().toString();
    qInfo() << file_selected;
    action = menu->addAction("Retirer un tag");
    connect(action, SIGNAL(triggered()), this, SLOT(del_tag_from_file_window()));
    connect(action, SIGNAL(triggered()), menu, SLOT(close()));
    menu->exec(pos + filelist->pos() + this->pos());
}

void MainWindow::add_tag_to_file_window()
{
    do_operation_on_file_window(true);
}

void MainWindow::del_tag_from_file_window()
{
    do_operation_on_file_window(false);
}

void MainWindow::do_operation_on_file_window(bool add_or_del)
{
    QListView *filelist = this->findChild<QListView*>(tr("FileList"));
    filelist->selectionModel()->selectedIndexes();
    QString file_selected = filelist->selectionModel()->selectedIndexes().first().data().toString();
    if(add_or_del)
        qInfo() << "Ajouter un tag à : " << file_selected;
    else
        qInfo() << "Retirer un tag de : " << file_selected;
    operation = new FileOperation;
    operation->file_name = file_selected;
    operation->add_or_del = add_or_del;
    Tag *tags;
    if(add_or_del)
        tags = TagFolder_list_tags(&folder);
    else
        tags = TagFolder_get_tags_tagging_specific_file(&folder, file_selected.toLocal8Bit().data());
    GetExistingTagName Dialog(this, tags);
    Dialog.exec();
}


void MainWindow::set_tag_name(const QString &name)
{
    operation->tag_name = name;
    qInfo() << "Tag name for operation : " << name;
}

void MainWindow::do_operation_on_file()
{
    if(operation->add_or_del == true)
    {
        TagFolder_tag_a_file(&folder, operation->file_name.toLocal8Bit().data(), operation->tag_name.toLocal8Bit().data());
        qInfo() << "Add tag to file operation";
    }
    else
    {
        TagFolder_untag_a_file(&folder, operation->file_name.toLocal8Bit().data(), operation->tag_name.toLocal8Bit().data());
        qInfo() << "Del tag from file operation";
    }
}

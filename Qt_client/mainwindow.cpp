#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "GetTagName.h"
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
    QString path("../test");
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
            list << File_get_name(ptr);
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
        QVBoxLayout *IncludeTagsListLayout = new QVBoxLayout();
        IncludeTagsListLayout->setSizeConstraint(IncludeTagsListLayout->SetMinimumSize);
        QWidget *IncludeTagsList = this->findChild<QWidget*>("IncludeTagsList");
        QVBoxLayout *ExcludeTagsListLayout = new QVBoxLayout();
        ExcludeTagsListLayout->setSizeConstraint(ExcludeTagsListLayout->SetMinimumSize);
        QWidget *ExcludeTagsList = this->findChild<QWidget*>("ExcludeTagsList");

        connect(IncludeTagsList, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(on_IncludeTag_customContextMenuRequested(QPoint)));
        connect(ExcludeTagsList, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(on_ExcludeTag_customContextMenuRequested(QPoint)));

        Tag *ptr = ltags;

        while(ptr != NULL)
        {
            QCheckBox *checkBox;
            QString checkbox_name(Tag_get_name(ptr));
            checkBox = new QCheckBox;
            checkBox->setObjectName(checkbox_name);
            checkBox->setText(checkbox_name);
            checkBox->setContextMenuPolicy(Qt::CustomContextMenu);
            connect(checkBox, SIGNAL(clicked(bool)), this, SLOT(on_checkBox_clicked(bool)));

            switch(Tag_get_type(ptr))
            {
                case TagTypeInclude :
                    IncludeTagsListLayout->addWidget(checkBox);
                    connect(checkBox, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(on_IncludeTag_customContextMenuRequested(QPoint)));
                    break;
                case TagTypeExclude :
                    ExcludeTagsListLayout->addWidget(checkBox);
                    connect(checkBox, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(on_ExcludeTag_customContextMenuRequested(QPoint)));
                    break;
            }

            ptr = Tag_get_next(ptr);
        }
        IncludeTagsList->setLayout(IncludeTagsListLayout);
        if(IncludeTagsListLayout->count() == 0)
            IncludeTagsList->setVisible(false);
        ExcludeTagsList->setLayout(ExcludeTagsListLayout);
        if(ExcludeTagsListLayout->count() == 0)
            ExcludeTagsList->setVisible(false);
        qInfo() << "exclude : " << ExcludeTagsListLayout->count() << "include : " << IncludeTagsListLayout->count();

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

void MainWindow::on_IncludeTag_customContextMenuRequested(const QPoint &pos)
{
    on_Tag_customContextMenuRequested(true);
    tag_operation->tag_type = TagTypeInclude;
}

void MainWindow::on_ExcludeTag_customContextMenuRequested(const QPoint &pos)
{
    on_Tag_customContextMenuRequested(false);
    tag_operation->tag_type = TagTypeExclude;
}

void MainWindow::on_Tag_customContextMenuRequested(bool including)
{
    QWidget *w_sender = qobject_cast<QWidget*>(sender());
    QCheckBox *checkBox;
    QMenu *menu = new QMenu(w_sender);
    QAction *action;
    if(including)
        action = menu->addAction(tr("Ajouter un Tag inclusif"));
    else
        action = menu->addAction(tr("Ajouter un Tag exclusif"));
    checkBox = qobject_cast<QCheckBox*>(w_sender);
    if(checkBox != NULL)
    {
        qInfo() << checkBox->text();
        action = menu->addAction(tr("Renommer \"") + checkBox->text() + tr("\""));
        action = menu->addAction(tr("Supprimer \"") + checkBox->text() + tr("\""));
    }
    menu->exec(QCursor::pos());

    //GetTagName Dialog(this);
    //Dialog.exec();
}

void MainWindow::on_FileList_customContextMenuRequested(const QPoint &pos)
{
    QListView *filelist = this->findChild<QListView*>("FileList");
    QMenu *menu = new QMenu(qobject_cast<QWidget*>(filelist));
    QString file_selected = filelist->selectionModel()->selectedIndexes().first().data().toString();
    QAction *action;
    QMap<QString, Tag*> File_Tags;
    Tag *all_tags = TagFolder_list_tags(&folder), *file_tags = TagFolder_get_tags_tagging_specific_file(&folder, file_selected.toLocal8Bit().data()), *ptr;

    file_operation = new FileOperation;
    file_operation->file_name = file_selected;
    ptr = file_tags;
    while(ptr != NULL)
    {
        File_Tags[tr(Tag_get_name(ptr))] = ptr;
        ptr = Tag_get_next(ptr);
    }
    ptr = all_tags;
    while(ptr != NULL)
    {
        action = menu->addAction(tr(Tag_get_name(ptr)));
        action->setCheckable(true);
        connect(action, SIGNAL(toggled(bool)), this, SLOT(do_operation_on_file_window(bool)));
        if(File_Tags[tr(Tag_get_name(ptr))] != NULL)
            action->setChecked(true);
        ptr = Tag_get_next(ptr);
    }
    menu->exec(QCursor::pos());
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

    file_operation->add_or_del = add_or_del;
    file_operation->tag_name = qobject_cast<QAction*>(sender())->text();
    do_operation_on_file();
}

void MainWindow::do_operation_on_file()
{
    if(file_operation->add_or_del == true)
    {
        TagFolder_tag_a_file(&folder, file_operation->file_name.toLocal8Bit().data(), file_operation->tag_name.toLocal8Bit().data());
        qInfo() << "Add tag to file operation";
    }
    else
    {
        TagFolder_untag_a_file(&folder, file_operation->file_name.toLocal8Bit().data(), file_operation->tag_name.toLocal8Bit().data());
        qInfo() << "Del tag from file operation";
    }
}

void MainWindow::do_operation_on_tag()
{
    switch(tag_operation->op_type)
    {
        case OpTypeDel :
            TagFolder_delete_tag(&folder, tag_operation->tag_name.toLocal8Bit().data());
            qInfo() << "Del a tag operation";
            break;
        case OpTypeAdd :
            TagFolder_create_tag(&folder, tag_operation->tag_name.toLocal8Bit().data(), tag_operation->tag_type);
            qInfo() << "Create a tag operation";
            break;
    }
}

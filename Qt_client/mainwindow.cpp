#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "GetTagName.h"
#include <qwidget.h>
#include <qlayout.h>
#include <qlogging.h>
#include <qdebug.h>
#include <qcheckbox.h>
#include <qdatetime.h>
#include <QObject>
#include <QMenu>
#include <treemodel.h>
#include <sys/stat.h>

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
    QStringList headers;
    QString datas;
    File *current_files, *ptr;
    current_files = TagFolder_list_current_files(&folder);
    ptr = current_files;
    files_ids.clear();
    headers << tr("Fichier") << tr("Dernière Modification");
    while(ptr != NULL)
    {
        QString data;
        QDateTime last_modif;
        last_modif.setTime_t(File_get_last_modification(ptr)->tv_sec);
        data += tr(File_get_name(ptr)) + tr("\t") + last_modif.toString(tr("d MMM yyyy hh:mm:ss"));
        files_ids << File_get_id(ptr);
        ptr = File_get_next(ptr);
        datas += data + tr("\n");
    }
    TreeModel *m = new TreeModel(headers, datas, qobject_cast<QObject*>(this));
    if(current_files != NULL)
        File_free(current_files);
    ui->FileList->setModel(m);
}

void MainWindow::reload_tags_list(void)
{
    Tag *ltags;
    ltags = TagFolder_list_tags(&folder);
    if(ltags != NULL)
    {
        QLayoutItem *child;
        QWidget *IncludeTagsList = this->findChild<QWidget*>("IncludeTagsList");
        QVBoxLayout *IncludeTagsListLayout = qobject_cast<QVBoxLayout*>(IncludeTagsList->layout());
        if(IncludeTagsListLayout == NULL)
        {
            IncludeTagsListLayout = new QVBoxLayout();
            IncludeTagsListLayout->setSizeConstraint(IncludeTagsListLayout->SetMinimumSize);
            IncludeTagsList->setLayout(IncludeTagsListLayout);
            connect(IncludeTagsList, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(on_IncludeTag_customContextMenuRequested(QPoint)));
        }
        while ((child = IncludeTagsListLayout->takeAt(0)) != 0)
        {
            delete child->widget();
            delete child;
        }

        QWidget *ExcludeTagsList = this->findChild<QWidget*>("ExcludeTagsList");
        QVBoxLayout *ExcludeTagsListLayout = qobject_cast<QVBoxLayout*>(ExcludeTagsList->layout());
        if(ExcludeTagsListLayout == NULL)
        {
            ExcludeTagsListLayout = new QVBoxLayout();
            ExcludeTagsListLayout->setSizeConstraint(ExcludeTagsListLayout->SetMinimumSize);
            ExcludeTagsList->setLayout(ExcludeTagsListLayout);
            connect(ExcludeTagsList, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(on_ExcludeTag_customContextMenuRequested(QPoint)));
        }
        while ((child = ExcludeTagsListLayout->takeAt(0)) != 0)
        {
            delete child->widget();
            delete child;
        }

        Tag *ptr = ltags;

        while(ptr != NULL)
        {
            TagCheckBox *checkBox;
            QString checkbox_name(Tag_get_name(ptr));
            checkBox = new TagCheckBox(ptr);
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
        if(IncludeTagsListLayout->count() == 0)
            IncludeTagsList->setVisible(false);
        if(ExcludeTagsListLayout->count() == 0)
            ExcludeTagsList->setVisible(false);
        qInfo() << "exclude : " << ExcludeTagsListLayout->count() << "include : " << IncludeTagsListLayout->count();

        ui->retranslateUi(this);
    }
}

void MainWindow::on_checkBox_clicked(bool checked)
{
    QObject *o = sender();
    TagCheckBox *chk = (TagCheckBox*)qobject_cast<QCheckBox*>(o);
    qInfo() << o << chk->text() << " (" << Tag_get_id(chk->get_tag()) << ")" << "checked : " << checked;
    if(checked)
        TagFolder_select_tag(&folder, Tag_get_id(chk->get_tag()));
    else
        TagFolder_unselect_tag(&folder, Tag_get_id(chk->get_tag()));
    reload_file_list();
}

void MainWindow::on_IncludeTag_customContextMenuRequested(const QPoint &pos)
{
    Q_UNUSED(pos);
    tag_operation = new TagOperation;
    tag_operation->tag_type = TagTypeInclude;
    on_Tag_customContextMenuRequested(true);
}

void MainWindow::on_ExcludeTag_customContextMenuRequested(const QPoint &pos)
{
    Q_UNUSED(pos);
    tag_operation = new TagOperation;
    tag_operation->tag_type = TagTypeExclude;
    on_Tag_customContextMenuRequested(false);
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
    connect(action, SIGNAL(triggered(bool)), this, SLOT(get_new_tag_name_window(bool)));
    checkBox = qobject_cast<QCheckBox*>(w_sender);
    if(checkBox != NULL)
    {
        qInfo() << checkBox->text();
        action = menu->addAction(tr("Renommer \"") + checkBox->text() + tr("\""));
        action = menu->addAction(tr("Supprimer \"") + checkBox->text() + tr("\""));
    }
    menu->exec(QCursor::pos());
}

void MainWindow::get_new_tag_name_window(bool)
{
    qInfo() << "Fenetre Get Tag Name";
    tag_operation->op_type = OpTypeAdd;
    GetTagName Dialog(this);
    Dialog.exec();
}

void MainWindow::set_tag_name(const QString &name)
{
    qInfo() << "set tag name" << name;
    tag_operation->tag_name = name;
}

void MainWindow::on_FileList_customContextMenuRequested(const QPoint &pos)
{
    Q_UNUSED(pos);
    QTreeView *filelist = this->findChild<QTreeView*>("FileList");
    QMenu *menu = new QMenu(qobject_cast<QWidget*>(filelist));
    QString file_selected = filelist->selectionModel()->selectedIndexes().first().data().toString();
    int file_id = files_ids[filelist->selectionModel()->selectedIndexes().first().row()];
    QAction *action;
    QMap<QString, Tag*> File_Tags;
    Tag *all_tags = TagFolder_list_tags(&folder), *file_tags = TagFolder_get_tags_tagging_specific_file(&folder, file_id), *ptr;

    file_operation = new FileOperation;
    file_operation->file_id= file_id;
    ptr = file_tags;
    while(ptr != NULL)
    {
        File_Tags[tr(Tag_get_name(ptr))] = ptr;
        ptr = Tag_get_next(ptr);
    }
    ptr = all_tags;
    while(ptr != NULL)
    {
        QVariant var(Tag_get_id(ptr));
        action = menu->addAction(tr(Tag_get_name(ptr)));
        action->setCheckable(true);
        action->setData(var);
        connect(action, SIGNAL(toggled(bool)), this, SLOT(do_operation_on_file_window(bool)));
        if(File_Tags[tr(Tag_get_name(ptr))] != NULL)
            action->setChecked(true);
        ptr = Tag_get_next(ptr);
    }
    menu->exec(QCursor::pos());
}

void MainWindow::do_operation_on_file_window(bool add_or_del)
{
    QTreeView *filelist = this->findChild<QTreeView*>(tr("FileList"));
    filelist->selectionModel()->selectedIndexes();
    QString file_selected = filelist->selectionModel()->selectedIndexes().first().data().toString();
    if(add_or_del)
        qInfo() << "Ajouter un tag à : " << file_selected;
    else
        qInfo() << "Retirer un tag de : " << file_selected;

    file_operation->add_or_del = add_or_del;
    file_operation->tag_id = qobject_cast<QAction*>(sender())->data().toInt();
    do_operation_on_file();
}

void MainWindow::do_operation_on_file()
{
    if(file_operation->add_or_del == true)
    {
        TagFolder_tag_a_file(&folder, file_operation->file_id, file_operation->tag_id);
        qInfo() << "Add tag " << file_operation->tag_id << " to file " << file_operation->file_id;
    }
    else
    {
        TagFolder_untag_a_file(&folder, file_operation->file_id, file_operation->tag_id);
        qInfo() << "Del tag " << file_operation->tag_id << " from file " << file_operation->file_id;
    }
}

void MainWindow::do_operation_on_tag()
{
    switch(tag_operation->op_type)
    {
        case OpTypeDel :
            TagFolder_delete_tag(&folder, tag_operation->tag_id);
            qInfo() << "Del a tag operation";
            break;
        case OpTypeAdd :
            TagFolder_create_tag(&folder, tag_operation->tag_name.toLocal8Bit().data(), tag_operation->tag_type);
            qInfo() << "Create a tag operation";
            break;
    }
    reload_tags_list();
}

TagCheckBox::TagCheckBox(Tag *tag)
{
    if(tag != NULL)
        this->tag = Tag_new(Tag_get_name(tag), Tag_get_id(tag), Tag_get_type(tag));
    else
        this->tag = NULL;
}

TagCheckBox::~TagCheckBox()
{
    if(tag != NULL)
        Tag_free(tag);
}

Tag *TagCheckBox::get_tag(void)
{
    return this->tag;
}

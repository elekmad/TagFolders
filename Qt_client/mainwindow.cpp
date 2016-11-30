#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "GetTagName.h"
#include <qwidget.h>
#include <qlayout.h>
#include <qlogging.h>
#include <qdebug.h>
#include <qcheckbox.h>
#include <qdatetime.h>
#include <qlabel.h>
#include <QObject>
#include <QMenu>
#include <qfiledialog.h>
#include <treemodel.h>
#include <sys/stat.h>
#include <unistd.h>
extern "C"
{
#include <String.h>
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    file_model = NULL;
    folder = NULL;
    ui->setupUi(this);
//    QString path("../test");
//    SetupTagFolder(path);
}

MainWindow::~MainWindow()
{
    delete ui;
    if(folder != NULL)
        TagFolder_free(folder);
    if(file_model != NULL)
        delete file_model;
}

void MainWindow::SetupTagFolder(QString &path)
{
    if(folder != NULL)
        TagFolder_free(folder);
    folder = TagFolder_new();
    TagFolder_setup_folder(folder, path.toLocal8Bit().data());
    reload_file_list();
    reload_tags_list();
    QLabel *label = this->findChild<QLabel*>("DirName");
    label->setText(path);
}

void MainWindow::reload_file_list(void)
{
    QStringList headers;
    QString datas;
    File *current_files, *ptr;
    current_files = TagFolder_list_current_files(folder);
    ptr = current_files;
    files_ids.clear();
    headers << tr("Fichier") << tr("Dernière Modification") << tr("Taille");
    while(ptr != NULL)
    {
        QString data;
        QDateTime last_modif;
        last_modif.setTime_t(File_get_last_modification(ptr)->tv_sec);
        data += tr(String_get_char_string(File_get_name(ptr))) + tr("\t") + last_modif.toString(tr("d MMM yyyy hh:mm:ss")) + tr("\t") + QString::number(File_get_size(ptr));
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
    ltags = TagFolder_list_tags(folder);
    QLayoutItem *child;

    //Build tags layouts and empty them if needed
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

    //If there is tags, fill layouts.
    if(ltags != NULL)
    {
        Tag *ptr = ltags;

        while(ptr != NULL)
        {
            TagCheckBox *checkBox;
            QString checkbox_name(String_get_char_string(Tag_get_name(ptr)));
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
        TagFolder_select_tag(folder, Tag_get_id(chk->get_tag()));
    else
        TagFolder_unselect_tag(folder, Tag_get_id(chk->get_tag()));
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

void MainWindow::import_file(bool)
{
    QString filename, localfilename;
    char generated_db_name[50];
    qInfo() << "Fenetre Get File Name";
    filename = QFileDialog::getOpenFileName(this, tr("Importer un fichier"), "~/", tr("Tous les fichiers (*.*)"));
    TagFolder_create_file_in_db(folder, filename.toLocal8Bit().data(), generated_db_name);
    localfilename = String_get_char_string(TagFolder_get_folder(folder));
    if(localfilename.at(localfilename.length() - 1) != '/')
        localfilename += '/';
    localfilename += generated_db_name;
    symlink(filename.toLocal8Bit().data(), localfilename.toLocal8Bit().data());
    qInfo() << filename << " linked " << localfilename;
    reload_file_list();
}

void MainWindow::delete_file(bool)
{
    File *f;
    qInfo() << "delete file " << file_operation->file_id;
    f = TagFolder_get_file_with_id(folder, file_operation->file_id);
    if(f != NULL)
    {
        QString localfilename;
        localfilename = String_get_char_string(TagFolder_get_folder(folder));
        if(localfilename.at(localfilename.length() - 1) != '/')
            localfilename += '/';
        localfilename += String_get_char_string(File_get_filename(f));
        qInfo() << "delete file name : " << localfilename;
        TagFolder_delete_file(folder, file_operation->file_id);
        unlink(localfilename.toLocal8Bit().data());
        reload_file_list();
    }
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
    QAction *action;
    QItemSelectionModel *selection_model = filelist->selectionModel();
    if(selection_model != NULL && !selection_model->selectedIndexes().isEmpty())
    {
        int file_id = files_ids[selection_model->selectedIndexes().first().row()];
        QMap<QString, Tag*> File_Tags;
        Tag *all_tags = TagFolder_list_tags(folder), *file_tags = TagFolder_get_tags_tagging_specific_file(folder, file_id), *ptr;

        file_operation = new FileOperation;
        file_operation->file_id= file_id;
        ptr = file_tags;
        while(ptr != NULL)
        {
            File_Tags[tr(String_get_char_string(Tag_get_name(ptr)))] = ptr;
            ptr = Tag_get_next(ptr);
        }
        ptr = all_tags;
        while(ptr != NULL)
        {
            QVariant var(Tag_get_id(ptr));
            action = menu->addAction(tr(String_get_char_string(Tag_get_name(ptr))));
            action->setCheckable(true);
            action->setData(var);
            connect(action, SIGNAL(toggled(bool)), this, SLOT(do_operation_on_file_window(bool)));
            if(File_Tags[tr(String_get_char_string(Tag_get_name(ptr)))] != NULL)
                action->setChecked(true);
            ptr = Tag_get_next(ptr);
        }
        action = menu->addAction(tr("Supprimer le fichier"));
        connect(action, SIGNAL(triggered(bool)), this, SLOT(delete_file(bool)));
    }
    action = menu->addAction(tr("Importer un nouveau fichier"));
    connect(action, SIGNAL(triggered(bool)), this, SLOT(import_file(bool)));
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
        TagFolder_tag_a_file(folder, file_operation->file_id, file_operation->tag_id);
        qInfo() << "Add tag " << file_operation->tag_id << " to file " << file_operation->file_id;
    }
    else
    {
        TagFolder_untag_a_file(folder, file_operation->file_id, file_operation->tag_id);
        qInfo() << "Del tag " << file_operation->tag_id << " from file " << file_operation->file_id;
    }
}

void MainWindow::do_operation_on_tag()
{
    switch(tag_operation->op_type)
    {
        case OpTypeDel :
            TagFolder_delete_tag(folder, tag_operation->tag_id);
            qInfo() << "Del a tag operation";
            break;
        case OpTypeAdd :
            TagFolder_create_tag(folder, tag_operation->tag_name.toLocal8Bit().data(), tag_operation->tag_type);
            qInfo() << "Create a tag operation";
            break;
    }
    reload_tags_list();
}

TagCheckBox::TagCheckBox(Tag *tag)
{
    if(tag != NULL)
        this->tag = Tag_new(String_get_char_string(Tag_get_name(tag)), Tag_get_id(tag), Tag_get_type(tag));
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

void MainWindow::on_OpenDir_released()
{
    QString dirname;
    dirname = QFileDialog::getExistingDirectory(this, tr("Ouvrir le dossier"));
    qInfo() << dirname;
    SetupTagFolder(dirname);
}

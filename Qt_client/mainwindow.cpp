#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "GetName.h"
#include <qwidget.h>
#include <qlayout.h>
#include <qlogging.h>
#include <qdebug.h>
#include <qcheckbox.h>
#include <qdatetime.h>
#include <qlabel.h>
#include <QObject>
#include <QMenu>
#include <QVariant>
#include <QDesktopServices>
#include <QTreeWidgetItem>
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
    this->setWindowTitle(path);
    qSetMessagePattern("[%{type}] %{appname} (%{file}:%{line}) - %{message}");
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

void MainWindow::reload_tags_list(int keep_unselect_buttons)
{
    Tag *ltags;
    ltags = TagFolder_list_tags(folder);
    QLayoutItem *child;

    QWidget *ExcludeTagsList = this->findChild<QWidget*>("ExcludeTagsList");
    QVBoxLayout *ExcludeTagsListLayout = qobject_cast<QVBoxLayout*>(ExcludeTagsList->layout());
    if(ExcludeTagsListLayout == NULL)
    {
        ExcludeTagsListLayout = new QVBoxLayout();
        ExcludeTagsListLayout->setSizeConstraint(ExcludeTagsListLayout->SetMinimumSize);
        ExcludeTagsList->setLayout(ExcludeTagsListLayout);
        connect(ExcludeTagsList, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(ExcludeTag_customContextMenuRequested(QPoint)));
    }
    while ((child = ExcludeTagsListLayout->takeAt(0)) != 0)
    {
        delete child->widget();
        delete child;
    }

    if(keep_unselect_buttons == 0)
    {
        QHBoxLayout *hbox = findChild<QHBoxLayout*>("UnselectTagsHL");
        QPushButton *button;
        while(hbox->count() > 0)
        {
            button = qobject_cast<QPushButton*>(hbox->itemAt(0)->widget());
            hbox->removeWidget(button);
            button->setParent(NULL);
        }
    }

    QTreeWidget *tw = this->findChild<QTreeWidget*>("treeWidget");
    QTreeWidgetItem *item;
    while((item = tw->topLevelItem(0)) != NULL)
    {
        tw->removeItemWidget(item, 0);
        delete item;
    }


    //If there is tags, fill layouts.
    if(ltags != NULL)
    {
        Tag *ptr = ltags;
        connect(tw, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(IncludeTag_customContextMenuRequested(QPoint)));

        while(ptr != NULL)
        {
            TagCheckBox *checkBox;
            QString checkbox_name(String_get_char_string(Tag_get_name(ptr)));


            switch(Tag_get_type(ptr))
            {
                case TagTypeInclude :
                {
                    QStringList list;
                    list << checkbox_name;
                    qInfo() << "add " << checkbox_name << "to list";
                    TagSelectItem *item = new TagSelectItem(list, ptr);
                    tw->insertTopLevelItem(0, item);
                    break;
                }
                case TagTypeExclude :
                {
                    checkBox = new TagCheckBox(ptr);
                    checkBox->setObjectName(checkbox_name);
                    checkBox->setText(checkbox_name);
                    checkBox->setContextMenuPolicy(Qt::CustomContextMenu);
                    connect(checkBox, SIGNAL(clicked(bool)), this, SLOT(Tag_checkBox_clicked(bool)));
                    ExcludeTagsListLayout->addWidget(checkBox);
                    connect(checkBox, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(ExcludeTag_customContextMenuRequested(QPoint)));
                    break;
                }
            }

            ptr = Tag_get_next(ptr);
        }
        if(ExcludeTagsListLayout->count() == 0)
            ExcludeTagsList->setVisible(false);
        qInfo() << "exclude : " << ExcludeTagsListLayout->count();// << "include : " << IncludeTagsListLayout->count();

        ui->retranslateUi(this);
    }
}

void MainWindow::Tag_checkBox_clicked(bool checked)
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

void MainWindow::Tag_unselect_button_clicked(bool clicked)
{
    Q_UNUSED(clicked);
    QObject *o = sender();
    TagUnselectButton *button = (TagUnselectButton*)qobject_cast<QPushButton*>(o);
    qInfo() << "unselect tag " << o << button->text() << " (" << Tag_get_id(button->get_tag()) << ")";
    TagFolder_unselect_tag(folder, Tag_get_id(button->get_tag()));
    reload_file_list();
    QHBoxLayout *hbox = findChild<QHBoxLayout*>("UnselectTagsHL");
    qInfo() << "button " << button << "parent " << o << "hbox " << hbox;
    hbox->removeWidget(button);
    button->setParent(NULL);
}

void MainWindow::IncludeTag_customContextMenuRequested(const QPoint &pos)
{
    qInfo() << "passage";
    tag_operation = new TagOperation;
    tag_operation->tag_type = TagTypeInclude;
    Tag_customContextMenuRequested(true, pos);
}

void MainWindow::ExcludeTag_customContextMenuRequested(const QPoint &pos)
{
    Q_UNUSED(pos);
    tag_operation = new TagOperation;
    tag_operation->tag_type = TagTypeExclude;
    Tag_customContextMenuRequested(false, pos);
}

void MainWindow::Tag_customContextMenuRequested(bool including, const QPoint &pos)
{
    QWidget *w_sender = qobject_cast<QWidget*>(sender());
    QCheckBox *checkBox = qobject_cast<QCheckBox*>(w_sender);
    QMenu *menu = new QMenu(w_sender);
    QAction *action;
    QString tag_name;
    if(including)
        action = menu->addAction(tr("Ajouter un Tag inclusif"));
    else
        action = menu->addAction(tr("Ajouter un Tag exclusif"));
    connect(action, SIGNAL(triggered(bool)), this, SLOT(get_new_tag_name_window(bool)));
    if(checkBox != NULL) // Coming from exclude checkbox list
    {
        tag_name = checkBox->text();
        tag_operation->tag_type = TagTypeExclude;
    }
    else
    {
        QTreeWidget *w = qobject_cast<QTreeWidget*>(sender());
        TagSelectItem *item = (TagSelectItem*)w->itemAt(pos);
        if(item != NULL)//Else rigth click outside item, no tag selected.
        {
            Tag *tag = item->get_tag();
            tag_name = String_get_char_string(Tag_get_name(tag));
            tag_operation->tag_id = Tag_get_id(tag);
            tag_operation->tag_type = TagTypeInclude;
        }
    }
    if(tag_name.isEmpty() == false)
    {
        qInfo() << tag_name;
        tag_operation->tag_name = tag_name;
        action = menu->addAction(tr("Renommer \"") + tag_name + tr("\""));
        connect(action, SIGNAL(triggered(bool)), this, SLOT(get_tag_new_name_window(bool)));
        action = menu->addAction(tr("Supprimer \"") + tag_name + tr("\""));
        connect(action, SIGNAL(triggered(bool)), this, SLOT(do_operation_on_tag(bool)));
    }

    menu->exec(QCursor::pos());
}

void MainWindow::get_new_tag_name_window(bool)
{
    qInfo() << "Fenetre Get Tag Name";
    tag_operation->op_type = TagOpTypeAdd;
    GetTagName Dialog(this);
    Dialog.exec();
}

void MainWindow::get_tag_new_name_window(bool)
{
    qInfo() << "Fenetre Get Tag New Name";
    tag_operation->op_type = TagOpTypeRename;
    GetTagName Dialog(this, tag_operation->tag_name);
    Dialog.exec();
}

void MainWindow::get_file_new_name_window(bool)
{
    qInfo() << "Fenetre Get File New Name";
    file_operation->op_type = FileOpTypeRename;
    GetFileName Dialog(this, file_operation->file_name);
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

void MainWindow::open_file(bool b)
{
    File *f;
    Q_UNUSED(b);
    qInfo() << "open file " << file_operation->file_id;
    f = TagFolder_get_file_with_id(folder, file_operation->file_id);
    if(f != NULL)
    {
        QString localfilename = "file://";
        localfilename += String_get_char_string(TagFolder_get_folder(folder));
        if(localfilename.at(localfilename.length() - 1) != '/')
            localfilename += '/';
        localfilename += String_get_char_string(File_get_filename(f));
        qInfo() << "delete file name : " << localfilename;
        QDesktopServices::openUrl(QUrl(localfilename, QUrl::TolerantMode));
    }
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

void MainWindow::set_file_name(const QString &name)
{
    qInfo() << "set file name" << name;
    file_operation->file_name = name;
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

        action = menu->addAction(tr("Ouvrir"));
        connect(action, SIGNAL(triggered(bool)), this, SLOT(open_file(bool)));
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
        action = menu->addAction(tr("Renommer le fichier"));
        connect(action, SIGNAL(triggered(bool)), this, SLOT(get_file_new_name_window(bool)));
        action = menu->addAction(tr("Supprimer le fichier"));
        connect(action, SIGNAL(triggered(bool)), this, SLOT(delete_file(bool)));
    }
    action = menu->addAction(tr("Importer un nouveau fichier"));
    connect(action, SIGNAL(triggered(bool)), this, SLOT(import_file(bool)));
    menu->exec(QCursor::pos());
}

void MainWindow::on_FileList_doubleClicked(const QModelIndex &index)
{
    int file_id = files_ids[index.row()];
    File *file = TagFolder_get_file_with_id(folder, file_id);
    qInfo() << "file id" << file_id << " : " << File_get_name(file);
    if(file_operation != NULL)
        delete file_operation;
    file_operation = new FileOperation;
    file_operation->file_id = file_id;
    open_file();
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

    if(add_or_del)
        file_operation->op_type = FileOpTypeAddTag;
    else
        file_operation->op_type = FileOpTypeDelTag;
    file_operation->tag_id = qobject_cast<QAction*>(sender())->data().toInt();
    do_operation_on_file();
}

void MainWindow::do_operation_on_file()
{
    switch(file_operation->op_type)
    {
        case FileOpTypeAddTag :
            TagFolder_tag_a_file(folder, file_operation->file_id, file_operation->tag_id);
            qInfo() << "Add tag " << file_operation->tag_id << " to file " << file_operation->file_id;
            break;
        case FileOpTypeDelTag :
            TagFolder_untag_a_file(folder, file_operation->file_id, file_operation->tag_id);
            qInfo() << "Del tag " << file_operation->tag_id << " from file " << file_operation->file_id;
            break;
        case FileOpTypeDel ://Done in delete_file
            break;
        case FileOpTypeAdd ://Done in import_file
            break;
        case FileOpTypeRename :
            TagFolder_rename_file(folder, file_operation->file_id, file_operation->file_name.toLocal8Bit().data());
            reload_file_list();
            break;
    }
}

void MainWindow::do_operation_on_tag(bool b)
{
    Q_UNUSED(b);
    int must_remove_button = 0, must_rename_button = 0, must_reload_tags = 0, must_reload_files = 0;
    switch(tag_operation->op_type)
    {
        case TagOpTypeDel :
            TagFolder_delete_tag(folder, tag_operation->tag_id);
            qInfo() << "Del a tag operation";
            must_reload_files = 1;
            must_remove_button = 1;
            break;
        case TagOpTypeAdd :
            TagFolder_create_tag(folder, tag_operation->tag_name.toLocal8Bit().data(), tag_operation->tag_type);
            qInfo() << "Create a tag operation";
            must_reload_tags = 1;
            break;
        case TagOpTypeRename :
            TagFolder_rename_tag(folder, tag_operation->tag_id, tag_operation->tag_name.toLocal8Bit().data());
            qInfo() << "Rename a tag operation";
            must_rename_button = 1;
            break;
    }
    if(must_remove_button || must_rename_button)
    {
        if(tag_operation->tag_type == TagTypeInclude)
        {
            int index = 0;
            QHBoxLayout *hbox = findChild<QHBoxLayout*>("UnselectTagsHL");
            QLayoutItem *i = hbox->itemAt(index);
            TagUnselectButton *button = NULL;
            if(i != NULL)
                button = (TagUnselectButton*)qobject_cast<QPushButton*>(i->widget());
            while(button != NULL && Tag_get_id(button->get_tag()) != tag_operation->tag_id)
            {
                i = hbox->itemAt(++index);
                if(i != NULL)
                    button = (TagUnselectButton*)qobject_cast<QPushButton*>(i->widget());
            }
            if(button != NULL)
            {
                if(must_remove_button)
                {
                    hbox->removeWidget(button);
                    button->setParent(NULL);
                }
                else
                    button->setText(tag_operation->tag_name);
            }
        }
        reload_tags_list(1);
    }
    if(must_reload_tags)
        reload_tags_list();
    if(must_reload_files)
        reload_file_list();
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

TagSelectItem::TagSelectItem(QStringList &list, Tag *tag):QTreeWidgetItem(list, 0)
{
    if(tag != NULL)
        this->tag = Tag_new(String_get_char_string(Tag_get_name(tag)), Tag_get_id(tag), Tag_get_type(tag));
    else
        this->tag = NULL;
}

TagSelectItem::~TagSelectItem()
{
    if(tag != NULL)
        Tag_free(tag);
}

Tag *TagSelectItem::get_tag(void)
{
    return this->tag;
}

TagUnselectButton::TagUnselectButton(Tag *tag, QWidget *parent):QPushButton(parent)
{
    if(tag != NULL)
    {
        this->tag = Tag_new(String_get_char_string(Tag_get_name(tag)), Tag_get_id(tag), Tag_get_type(tag));
        this->setText(tr(String_get_char_string(Tag_get_name(tag))));
    }
    else
        this->tag = NULL;
}

TagUnselectButton::~TagUnselectButton()
{
    if(tag != NULL)
        Tag_free(tag);
}

Tag *TagUnselectButton::get_tag(void)
{
    return this->tag;
}

void MainWindow::on_treeWidget_itemDoubleClicked(QTreeWidgetItem *i, int column)
{
    TagSelectItem *item = (TagSelectItem*)i;
    qInfo() << item->text(column);
    qInfo() << "Select " << item->text(column) << " (" << Tag_get_id(item->get_tag()) << ")";
    if(TagFolder_select_tag(folder, Tag_get_id(item->get_tag())) == 0)
    {
        reload_file_list();
        QGroupBox *selectedtags = this->findChild<QGroupBox*>("SelectedTags");
        QHBoxLayout *hbox = selectedtags->findChild<QHBoxLayout*>("UnselectTagsHL");
        TagUnselectButton *button = new TagUnselectButton(item->get_tag(), selectedtags);
        qInfo() << "button " << button << "parent " << button->parent();
        connect(button, SIGNAL(clicked(bool)), this, SLOT(Tag_unselect_button_clicked(bool)));
        hbox->addWidget(button, 1, Qt::AlignLeft);
        qInfo() << "button " << button << "parent " << button->parent() << "group " << selectedtags << "layout " << hbox;
    }
}

void MainWindow::on_action_Open_triggered()
{
    QString dirname;
    dirname = QFileDialog::getExistingDirectory(this, tr("Ouvrir le dossier"));
    qInfo() << dirname;
    SetupTagFolder(dirname);
}

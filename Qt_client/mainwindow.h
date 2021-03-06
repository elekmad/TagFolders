#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <qcheckbox.h>
#include <qaction.h>
#include <QStringListModel>
#include <QTreeWidgetItem>
#include <QPushButton>
#include <QList>
extern "C"
{
#include <TagFolder.h>
}

enum TagOperationType
{
    TagOpTypeDel,
    TagOpTypeAdd,
    TagOpTypeRename,
};
typedef enum TagOperationType TagOperationType;

enum FileOperationType
{
    FileOpTypeDel,
    FileOpTypeAdd,
    FileOpTypeRename,
    FileOpTypeDelTag,
    FileOpTypeAddTag
};
typedef enum FileOperationType FileOperationType;

namespace Ui {
class MainWindow;
}

class FileOperation
{
public :
    int tag_id;
    QString file_name;//For creations
    int file_id;
    FileOperationType op_type;
};

class TagOperation
{
public :
    QString tag_name;//For creations
    int tag_id;
    TagOperationType op_type;
    TagType tag_type;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void reload_file_list(void);
    void reload_tags_list(int keep_unselect_buttons = 0);
    int get_file_id_from_row_id(int);
    void prepare_file_to_open(File *f, QString &opening_filename);

private slots:

    void on_filesList_customContextMenuRequested(const QPoint &pos);
    void IncludeTag_customContextMenuRequested(const QPoint &pos);
    void ExcludeTag_customContextMenuRequested(const QPoint &pos);
    void Tag_customContextMenuRequested(bool including, const QPoint &pos);
    void do_operation_on_file_window(bool);
    void get_new_tag_name_window(bool);
    void get_tag_new_name_window(bool);
    void get_file_new_name_window(bool);
    void create_folder_from_tags(QString &path);
    void open_file(bool b = true);
    void open_folder_created_by_tags(bool b);
    void import_file(bool);
    void delete_file(bool);
    void SetupTagFolder(QString &path);
    int check_generating_folder(QString &path);


    void on_treeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column);

    void on_filesList_doubleClicked(const QModelIndex &index);

    void on_action_Open_triggered();

public slots:
    void Tag_checkBox_clicked(bool checked);
    void Tag_unselect_button_clicked(bool clicked);
    void do_operation_on_file();
    void set_file_name(const QString &name);
    void do_operation_on_tag(bool b=true);
    void set_tag_name(const QString &name);

private:
    Ui::MainWindow *ui;
    QStringListModel *file_model;
    QList<int> files_ids;
    TagFolder *folder;
    QString generating_folder;
    FileOperation *file_operation;
    TagOperation *tag_operation;
};

class TagCheckBox : public QCheckBox
{
public:
    TagCheckBox(Tag *tag);
    ~TagCheckBox();
    Tag *get_tag(void);
private:
    Tag *tag;
};

class TagSelectItem : public QTreeWidgetItem
{
public:
    TagSelectItem(QStringList &list, Tag *tag);
    ~TagSelectItem();
    Tag *get_tag(void);
private:
    Tag *tag;
};

class TagUnselectButton : public QPushButton
{
public:
    TagUnselectButton(Tag *tag, QWidget *parent);
    ~TagUnselectButton();
    Tag *get_tag(void);
private:
    Tag *tag;
};

#endif // MAINWINDOW_H

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

enum OperationType
{
    OpTypeDel,
    OpTypeAdd,
};
typedef enum OperationType OperationType;

namespace Ui {
class MainWindow;
}

class FileOperation
{
public :
    int tag_id;
    QString file_name;//For creations
    int file_id;
    bool add_or_del;
};

class TagOperation
{
public :
    QString tag_name;//For creations
    int tag_id;
    OperationType op_type;
    TagType tag_type;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void reload_file_list(void);
    void reload_tags_list(void);

private slots:

    void on_FileList_customContextMenuRequested(const QPoint &pos);
    void IncludeTag_customContextMenuRequested(const QPoint &pos);
    void ExcludeTag_customContextMenuRequested(const QPoint &pos);
    void Tag_customContextMenuRequested(bool including, const QPoint &pos);
    void do_operation_on_file_window(bool);
    void get_new_tag_name_window(bool);
    void open_file(bool b = true);
    void import_file(bool);
    void delete_file(bool);
    void SetupTagFolder(QString &path);

    void on_treeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column);

    void on_FileList_doubleClicked(const QModelIndex &index);

    void on_action_Open_triggered();

public slots:
    void Tag_checkBox_clicked(bool checked);
    void Tag_unselect_button_clicked(bool clicked);
    void do_operation_on_file();
    void do_operation_on_tag();
    void set_tag_name(const QString &name);

private:
    Ui::MainWindow *ui;
    QStringListModel *file_model;
    QList<int> files_ids;
    TagFolder *folder;
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

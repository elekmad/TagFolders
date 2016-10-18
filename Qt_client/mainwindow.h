#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStringListModel>
extern "C"
{
#include "/home/damien/dev/TagFolders/TagFolder.h"
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
    QString tag_name;
    QString file_name;
    bool add_or_del;
};

class TagOperation
{
public :
    QString tag_name;
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
    void on_IncludeTag_customContextMenuRequested(const QPoint &pos);
    void on_ExcludeTag_customContextMenuRequested(const QPoint &pos);
    void on_Tag_customContextMenuRequested(bool including);
    void do_operation_on_file_window(bool);
    void get_new_tag_name_window(bool);

public slots:
    void on_checkBox_clicked(bool checked);
    void do_operation_on_file();
    void do_operation_on_tag();
    void set_tag_name(const QString &name);

private:
    Ui::MainWindow *ui;
    QStringListModel *file_model;
    TagFolder folder;
    FileOperation *file_operation;
    TagOperation *tag_operation;
};

#endif // MAINWINDOW_H

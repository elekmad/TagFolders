#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStringListModel>
extern "C"
{
#include "/home/damien/dev/TagFolders/TagFolder.h"
}

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
    void add_tag_to_file_window();
    void del_tag_from_file_window();
    void do_operation_on_file_window(bool);

public slots:
    void on_checkBox_clicked(bool checked);
    void do_operation_on_file();
    void set_tag_name(const QString&);

private:
    Ui::MainWindow *ui;
    QStringListModel *file_model;
    TagFolder folder;
    FileOperation *operation;
};

#endif // MAINWINDOW_H

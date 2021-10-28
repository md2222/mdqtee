#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSplitter>
#include <QTreeView>
#include <QHeaderView>
#include <QPlainTextEdit>
#include <QFileSystemModel>
#include "textedit.h"


class FileTreeView : public QTreeView
{
    Q_OBJECT

public:
    FileTreeView(QWidget *parent = nullptr) : QTreeView(parent) {};
    ~FileTreeView() {};

signals:
    void sigKeyEnter(const QModelIndex &index);
    void sigDirChanged(const QString dir);

private:
     void keyPressEvent(QKeyEvent *ev) override;
     bool viewportEvent(QEvent *ev) override;

private slots:
};


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    QSplitter *split;
    QString configPath;
    QString iniFileName;
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void onMainMessage(QString message);
    void onFileBrowserShow(bool show);

private:
    Ui::MainWindow *ui;
    FileTreeView* fileTree;
    TextEdit* textEdit;
    QPoint currPos;
    QSize currSize;
    bool isShowMaximized;
    void createFileTree(const QString& path);
    void setFileTreeRoot(const QString& path);
    void setFileTreeDir(QString path);

private slots:
    void onFileTreeMenu(const QPoint& pos);
    void onFileTreeMenuAct();
    void onCreateFolder();
    void onRenameFolder();
    void onDeleteFolder();
    void onOpenExplorer();
    void onFileTreeDoubleClicked(const QModelIndex& index);
    void onFileTreeKeyEnter(const QModelIndex &index);
    void closeEvent(QCloseEvent *ev);
    void onFilesChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int>& roles);
    void onFilesMoved(const QModelIndex &parent, int start, int end, const QModelIndex &destination, int row);
    void onDirectoryLoaded(const QString &path);
    void onFileRenamed(const QString &path, const QString &oldName, const QString &newName);
    void onFileMoved(QString file, QString path);
    void onFileTreeSelChanged(const QItemSelection &selected, const QItemSelection &deselected);
};




class FileSystemModel : public QFileSystemModel
{
    Q_OBJECT

public:
    FileSystemModel(QWidget *parent = nullptr)
    {
    };
    ~FileSystemModel() {};

signals:
    void sigFileMoved(QString file, QString path);

private slots:

private:
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
    {
        Q_UNUSED(action);
        Q_UNUSED(row);
        //Q_UNUSED(parent);

        bool moved = QFileSystemModel::dropMimeData(data, action, row, column, parent);

        if (moved)
            sigFileMoved(data->text().replace("file://", ""), filePath(parent));

        return moved;
    }

    Qt::ItemFlags flags(const QModelIndex &index) const
    {
        static int n = 0;

        Qt::ItemFlags defaultFlags = QFileSystemModel::flags(index);

        if (!index.isValid())
            return defaultFlags;

        const QFileInfo& fileInfo = this->fileInfo(index);

        n++;

        if (fileInfo.isDir())
        {
            return  defaultFlags & (~Qt::ItemIsDragEnabled);
        }

        return defaultFlags;
    }
};


#endif // MAINWINDOW_H

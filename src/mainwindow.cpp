#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QToolTip>
#include <QDesktopServices>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    statusBar()->setVisible(false);

    printf("Keep it simple ...\n");
    setStyleSheet("QDialogButtonBox { dialogbuttonbox-buttons-have-icons: 0; }");
    setWindowTitle(appName);


    QString appBaseName = QFileInfo(QCoreApplication::applicationFilePath()).baseName();
    configPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/" + appBaseName;
    printf("configDir=%s\n", configPath.toUtf8().data());

    QDir configDir(configPath);
    if (!configDir.exists())
    {
        if (!configDir.mkpath(configPath))
        {
            QMessageBox::critical(this, appName, "Config directory create error.\n" + configPath, QMessageBox::Ok);
        }
    }

    iniFileName = configPath + "/" + appBaseName + ".cfg";
    qDebug() << "iniFileName=" << iniFileName;

    iconDir = ":/images/linux/";
    if (this->palette().background().color().lightness() < 100)
        iconDir = ":/images/linux/dark/";

    setWindowIcon(QIcon(":/images/linux/app2.png"));


    QSettings set(iniFileName, QSettings::IniFormat);


    split = new QSplitter(this);

    setCentralWidget(split);

    fileTree = new FileTreeView(this);
    connect(fileTree, &FileTreeView::sigKeyEnter, this, &MainWindow::onFileTreeKeyEnter);

    textEdit = new TextEdit(this, set);

    split->addWidget(fileTree);
    split->addWidget(textEdit);
    split->setStretchFactor(1, 2);

    connect(textEdit, &TextEdit::sigFileBrowserShow, this, &MainWindow::onFileBrowserShow);  //, Qt::QueuedConnection


    set.beginGroup("private");

    currPos = set.value("mainWinPos", QPoint(440, 80)).toPoint();
    currSize = set.value("mainWinSize", QSize(1000, 800)).toSize();

    isShowMaximized = false;
    if (set.value("mainWinState", Qt::WindowNoState).toInt() == Qt::WindowMaximized)
        setWindowState(Qt::WindowMaximized);

    QByteArray mainWinLayoutState = set.value("mainWinLayoutState", QByteArray()).toByteArray();
    if (!mainWinLayoutState.isEmpty())
        restoreState(mainWinLayoutState);

    if (set.contains("mainWinSplitState"))
        split->restoreState(set.value("mainWinSplitState").toByteArray());

    QString fileTreeRootPath = set.value("fileTreeRootPath", QDir::homePath()).toString();
    QString fileTreeDir = set.value("fileTreeDir", "").toString();

    bool isFileBrowserVisible = set.value("fileBrowser", false).toBool();

    set.endGroup();


    createFileTree(fileTreeRootPath);
    setFileTreeRoot(fileTreeRootPath);

    fileTree->setVisible(isFileBrowserVisible);
    textEdit->setFileBrowserChecked(isFileBrowserVisible);
    textEdit->iniFileName = iniFileName;

    move(currPos);
    resize(currSize);

    delay(1000);
    setFileTreeDir(fileTreeDir);
}


MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::closeEvent(QCloseEvent *ev)
{
    QSettings set(iniFileName, QSettings::IniFormat);

    set.beginGroup("private");

    if (!textEdit->close(set))
    {
        ev->ignore();
        return;
    }

    if (!this->isMaximized())
    {
        set.setValue("mainWinPos", pos());
        set.setValue("mainWinSize", size());
    }
    set.setValue("mainWinState", (int)windowState());
    set.setValue("mainWinLayoutState", saveState());
    set.setValue("mainWinSplitState", split->saveState());

    QFileSystemModel* model = (QFileSystemModel*)fileTree->model();
    QModelIndex index = fileTree->rootIndex();
    if (index.isValid())
        set.setValue("fileTreeRootPath", model->fileInfo(index).filePath());

    index = fileTree->currentIndex();
    if (index.isValid())
    {
        QString path;

        if (fileTree->isExpanded(index))
            path = model->fileInfo(index).filePath();
        else if (index.parent() == fileTree->rootIndex())
            ;
        else if (index.parent().isValid())
            path = model->fileInfo(index.parent()).filePath();

        set.setValue("fileTreeDir", path);
        qDebug() << model->fileInfo(index).filePath() << fileTree->isExpanded(index) << model->fileInfo(index.parent()).filePath() << path;
    }

    set.setValue("fileBrowser", fileTree->isVisible());

    set.endGroup();


    QMainWindow::closeEvent(ev);
}


void MainWindow::createFileTree(const QString& path)
{
    FileSystemModel* model = new FileSystemModel(this);
    model->setRootPath("");
    model->setOption(QFileSystemModel::DontUseCustomDirectoryIcons);
    model->setNameFilterDisables(false);
    QStringList filter;
    filter << "*.maff" << "*.html";
    model->setNameFilters(filter);
    model->setReadOnly(false);

    connect(model, &FileSystemModel::sigFileMoved, this, &MainWindow::onFileMoved, Qt::QueuedConnection);

    fileTree->setModel(model);
    fileTree->hideColumn(1);
    fileTree->hideColumn(2);
    fileTree->hideColumn(3);

    QModelIndex rootIndex = model->setRootPath(QDir::rootPath());
    if (rootIndex.isValid())
    {
        fileTree->setRootIndex(rootIndex);
    }

    fileTree->setAnimated(false);
    fileTree->setIndentation(20);

    fileTree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(fileTree, &QTreeView::customContextMenuRequested, this, &MainWindow::onFileTreeMenu);
    connect(fileTree, &QTreeView::doubleClicked, this, &MainWindow::onFileTreeDoubleClicked);

    fileTree->model()->setHeaderData(0, Qt::Horizontal, tr("Files"), Qt::DisplayRole);  //--

    fileTree->setSelectionMode(QAbstractItemView::SingleSelection);
    fileTree->setDragEnabled(true);
    fileTree->setAcceptDrops(true);
    fileTree->setDropIndicatorShown(true);
    fileTree->setDragDropMode(QAbstractItemView::InternalMove);

    fileTree->setEditTriggers(QAbstractItemView::NoEditTriggers);

    connect(fileTree->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::onFileTreeSelChanged, Qt::QueuedConnection);
}


void MainWindow::onFileTreeSelChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    QModelIndex index = selected.indexes().first();
    if (!index.isValid())  return;
    QFileSystemModel* model = (QFileSystemModel*)index.model();
    qDebug() << "MainWindow::onFileTreeSelChanged:    " << model->filePath(index);
    QString dir;

    if (index.parent().isValid())
        dir = model->filePath(index.parent());

    qDebug() << "MainWindow::onFileTreeSelChanged:    dir=" << dir;

    textEdit->fileBrowserDir = dir;
}


void MainWindow::setFileTreeRoot(const QString& path)
{
    QModelIndex index = ((QFileSystemModel*)fileTree->model())->index(path);
    if (index.isValid())
    {
        fileTree->setRootIndex(index);
        fileTree->model()->sort(0, Qt::AscendingOrder);
    }
}


void MainWindow::setFileTreeDir(QString path)
{
    QFileSystemModel* model = (QFileSystemModel*)fileTree->model();
    if (!model)  return;

    QModelIndex index;

    if (path.isEmpty())
        index = model->index(0, 0, fileTree->rootIndex());
    else
        index = model->index(path);

    if (!index.isValid())  return;

    qDebug() << "MainWindow::setFileTreeDir:    " << model->fileInfo(index).filePath();
    fileTree->setCurrentIndex(index);

    if (!path.isEmpty())
        while (index.isValid())
        {
            fileTree->expand(index);
            index = index.parent();
        }
}


void MainWindow::onFileTreeMenu(const QPoint& pos)
{
    QMenu* fileMenu = new QMenu(this);
    QAction* a;

    QModelIndex index = fileTree->currentIndex();

    if (index.isValid())
    {
        QFileSystemModel* model = (QFileSystemModel*)fileTree->model();

        if (index.parent() == fileTree->rootIndex() && index.parent().parent().isValid())
        {
            a = new QAction("Show parent", this);
            a->setData("showParent");
            connect(a, &QAction::triggered, this, &MainWindow::onFileTreeMenuAct);
            fileMenu->addAction(a);
        }

        if (model->isDir(index))
        {
            a = new QAction("Set as root", this);
            a->setData("setRoot");
            connect(a, &QAction::triggered, this, &MainWindow::onFileTreeMenuAct);
            fileMenu->addAction(a);

            a = new QAction(tr("Rename folder"), this);
            connect(a, &QAction::triggered, this, &MainWindow::onRenameFolder);
            fileMenu->addAction(a);


            a = new QAction(tr("Delete empty folder"), this);
            connect(a, &QAction::triggered, this, &MainWindow::onDeleteFolder);
            fileMenu->addAction(a);

            QDir dir(model->filePath(index));
            bool isDirEmpty = true;
            QStringList sl = dir.entryList();
            for (int i = 0; i < sl.count(); i++)
                if (sl[i] != "." && sl[i] != "..")
                {
                    isDirEmpty = false;
                    break;
                }

            if (!isDirEmpty)
                a->setEnabled(false);


            a = new QAction(tr("Open in file manager"), this);
            connect(a, &QAction::triggered, this, &MainWindow::onOpenExplorer);
            fileMenu->addAction(a);

            fileMenu->addSeparator();

            a = new QAction(tr("Create folder"), this);
            connect(a, &QAction::triggered, this, &MainWindow::onCreateFolder);
            fileMenu->addAction(a);
        }

        if (fileMenu->actions().count())
            fileMenu->popup(fileTree->viewport()->mapToGlobal(pos));
    }

}


void MainWindow::onFileTreeMenuAct()
{
    QString s = qobject_cast<QAction*>(sender())->data().toString();


    if (s == "showParent")
    {
        QModelIndex index = fileTree->rootIndex();
        if (!index.isValid())  return;

        index = index.parent();
        if (!index.isValid())  return;

        fileTree->setRootIndex(index);
        fileTree->model()->sort(0, Qt::AscendingOrder);  //--
    }
    else if (s == "setRoot")
    {
        QModelIndex index = fileTree->currentIndex();
        fileTree->setRootIndex(index);
    }
}


void MainWindow::onCreateFolder()
{
    QModelIndex index = fileTree->currentIndex();
    QFileSystemModel* model = (QFileSystemModel*)fileTree->model();

    if (model->isDir(index))
    {
        QModelIndex newIndex = model->mkdir(index, "new");
        fileTree->setCurrentIndex(newIndex);
        fileTree->edit(newIndex);
        model->sort(1, Qt::AscendingOrder);
        model->sort(0, Qt::AscendingOrder);
    }
}


void MainWindow::onRenameFolder()
{
    QFileSystemModel* model = (QFileSystemModel*)fileTree->model();
    QModelIndex index = fileTree->currentIndex();
    if (model->isDir(index))
    {
        fileTree->edit(index);
    }
}


void MainWindow::onDeleteFolder()
{
    QFileSystemModel* model = (QFileSystemModel*)fileTree->model();
    QModelIndex index = fileTree->currentIndex();

    if (model->isDir(index))
    {
        qDebug() << "MainWindow::onDeleteFolder:    rmdir: " << model->filePath(index);
        if (!model->rmdir(index))
        {
            qDebug() << "MainWindow::onDeleteFolder:    rmdir error ";
        }
    }
}


void MainWindow::onOpenExplorer()
{
    QFileSystemModel* model = (QFileSystemModel*)fileTree->model();
    QModelIndex index = fileTree->currentIndex();
    if (model->isDir(index))
    {
        QDesktopServices::openUrl(QUrl::fromLocalFile(model->filePath(index)));
    }
}


void MainWindow::onFileTreeDoubleClicked(const QModelIndex& index)
{
    QFileSystemModel* model = (QFileSystemModel*)fileTree->model();

    if (model->fileInfo(index).isFile())
    {
        QString fileName = model->filePath(index);
        if (!fileName.isEmpty())
        {
            textEdit->load(fileName);
            fileTree->setFocus();
        }
    }
}


void MainWindow::onFileTreeKeyEnter(const QModelIndex &index)
{
    qDebug() << "MainWindow::onFileTreeKeyEnter";
    QFileSystemModel* model = (QFileSystemModel*)index.model();

    if (model->fileInfo(index).isFile())
    {
        QString fileName = model->filePath(index);
        if (!fileName.isEmpty())
        {
            textEdit->load(fileName);
            fileTree->setFocus();
        }
    }
    else if (model->isDir(index))
    {
        fileTree->expand(index);
    }
}


void MainWindow::onMainMessage(QString message)
{
    if (message.isEmpty())  return;

    textEdit->load(message);
}


void MainWindow::onFileBrowserShow(bool show)
{
    fileTree->setVisible(show);
}


void MainWindow::onFileMoved(QString file, QString path)
{
    textEdit->onFileMoved(file, path);
}


void MainWindow::onFilesChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int>& roles)
{
    qDebug() << "MainWindow::onFilesChanged:";

}


void MainWindow::onFilesMoved(const QModelIndex &parent, int start, int end, const QModelIndex &destination, int row)
{
    qDebug() << "MainWindow::onFilesMoved:";

}


void MainWindow::onDirectoryLoaded(const QString &path)
{
    qDebug() << "onDirectoryLoaded:    " << path;
    QFileSystemModel* model = (QFileSystemModel*)fileTree->model();
    qDebug() << "MainWindow::onDirectoryLoaded:    " << model->fileInfo(model->index(0, 0, fileTree->rootIndex())).filePath();
    fileTree->setCurrentIndex(model->index(0, 0, fileTree->rootIndex()));
}


void MainWindow::onFileRenamed(const QString &path, const QString &oldName, const QString &newName)
{
    qDebug() << "MainWindow::onFileRenamed:    " << path;
}


void FileTreeView::keyPressEvent(QKeyEvent *ev)
{
    if (state() != QAbstractItemView::EditingState)
    {
        if (ev->key() == Qt::Key_Return)
            emit sigKeyEnter(currentIndex());
        else if (ev->key() == Qt::Key_Backspace)
        {
            const QFileSystemModel* fsModel = (QFileSystemModel*)model();
            if (fsModel)
            {
                QModelIndex index = currentIndex();

                if (!fsModel->isDir(index))
                {
                    index = fsModel->parent(index);
                    setCurrentIndex(index);
                }

                collapse(index);
            }
        }
    }

    QTreeView::keyPressEvent(ev);
}


bool FileTreeView::viewportEvent(QEvent *ev)
{
    if (ev->type() == QEvent::ToolTip)
    {
        QHelpEvent *helpEvent = static_cast<QHelpEvent*>(ev);
        QModelIndex index = indexAt(helpEvent->pos());

        if (index.isValid())
        {
            QSize sizeHint = itemDelegate(index)->sizeHint(viewOptions(), index);
            QRect rItem(0, 0, sizeHint.width(), sizeHint.height());
            QRect rVisual = visualRect(index);
            if (rItem.width() > rVisual.width())
            {
                QFileSystemModel* model = (QFileSystemModel*)index.model();
                QToolTip::showText(helpEvent->globalPos(), model->fileName(index), this, rVisual, 6000);
                return true;  // other QTreeView::viewportEvent closing tooltip
                // Returns true to indicate to the event system that the event has been handled, and needs no further processing;
                // otherwise returns false to indicate that the event should be propagated further.
            }
        }
    }

    return QTreeView::viewportEvent(ev);
}

#include <QtDebug>
#include <QAction>
#include <QClipboard>
#include <QColorDialog>
#include <QComboBox>
#include <QFontComboBox>
#include <QFontDialog>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QMenu>
#include <QMenuBar>
#include <QTextCodec>
#include <QTextEdit>
#include <QStatusBar>
#include <QToolBar>
#include <QTextCursor>
#include <QTextDocumentWriter>
#include <QTextList>
#include <QCloseEvent>
#include <QMessageBox>
#include <QMimeData>
#include <QMimeDatabase>
#if defined(QT_PRINTSUPPORT_LIB)
#include <QtPrintSupport/qtprintsupportglobal.h>
#if QT_CONFIG(printer)
#if QT_CONFIG(printdialog)
#include <QPrintDialog>
#endif
#include <QPrinter>
#if QT_CONFIG(printpreviewdialog)
#include <QPrintPreviewDialog>
#endif
#endif
#endif
#include <QDesktopServices>
#include <QPainter>
#include <QTextTable>
#include "textedit.h"
#include "cconfigdialog.h"
#include <QtGui/private/qzipwriter_p.h>
#include <QtGui/private/qzipreader_p.h>


TextEdit *winMain = 0;
QString appName = "mdqtee";
QString appIconFileName;
QString iconDir;
QHash<QString, QString> mimeTypes;
QString untitledFileName = "untitled";
QSize fileDialogSize;
QString fileDialogCurrFilter;
QString dlgCurrFilter[6];

int CWaitCursor::waitCursorStatus = 0;


QString cutStr(QString str, int len)
{
    if (len > 0 && str.length() > len)
        return str.left(len - 3) + "...";
    else
        return str;
}


void delay(int msec)
{
    QTime t = QTime::currentTime().addMSecs(msec);
    while (QTime::currentTime() < t)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}


TextEdit::TextEdit(QWidget *parent, QSettings& set)
    : QMainWindow(parent)
{
    setWindowFlags(windowFlags() | Qt::Widget);

    QVBoxLayout* vb = new QVBoxLayout();
    vb->setContentsMargins(0, 0, 0, 0);

    wtTabs = new QTabWidget(this);
    wtTabs->setTabsClosable(true);
    wtTabs->setMovable(true);

    findTextPanel = new CFindTextPanel(this);
    findTextPanel->setPrevIcon(QIcon(iconDir + "go-up.png"));
    findTextPanel->setNextIcon(QIcon(iconDir + "go-down.png"));
    findTextPanel->setCloseIcon(QIcon(iconDir + "close.png"));
    findTextPanel->setFlat(true);
    findTextPanel->setVisible(false);

    vb->addWidget(wtTabs);
    vb->addWidget(findTextPanel);

    win = new QWidget();
    win->setLayout(vb);

    setCentralWidget(win);

    appPath = QCoreApplication::applicationDirPath();
    QString appBaseName = QFileInfo(QCoreApplication::applicationFilePath()).baseName();
    //configPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/" + appBaseName;
    //dataPath = configPath;
    homePath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    saveDir = homePath;
    //tempDir = "/tmp";
    tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    printf("appDir=%s\n", appPath.toUtf8().data());
    printf("tempDir=%s\n", tempDir.toUtf8().data());

    //mimeTypes[""] = "All files (*)";
    mimeTypes["html"] = "HTML files (*.html)";
    mimeTypes["maff"] = "Maff files (*.maff)";
    //mimeTypes["application/zip"] = "zip";
    mimeTypes["odt"] = "OpenDocument text (*.odt)";
    mimeTypes["md"] = "Markdown files (*.md)";
    mimeTypes["txt"] = "Text files (*.txt)";
    mimeTypes["pdf"] = "PDF files (*.pdf)";
    mimeTypes["epub"] = "Epub files (*.epub)";

    dlgCurrFilter[1] = mimeTypes["maff"];

    connect(wtTabs, &QTabWidget::currentChanged, this, &TextEdit::onTabChanged);
    connect(wtTabs, &QTabWidget::tabCloseRequested, this, &TextEdit::onTabCloseRequest);
    wtTabs->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(wtTabs, &QTabBar::customContextMenuRequested, this, &TextEdit::onTabContextMenu);

    connect(QApplication::clipboard(), &QClipboard::dataChanged, this, &TextEdit::clipboardDataChanged);

    connect(this, &TextEdit::httpAllFinished, this, &TextEdit::onHttpAllFinished, Qt::QueuedConnection);

    keyF3 = new QShortcut(this);
    keyF3->setKey(Qt::Key_F3);
    connect(keyF3, &QShortcut::activated, this, &TextEdit::onFindText);

    keyShF3 = new QShortcut(this);
    keyShF3->setKey(Qt::SHIFT + Qt::Key_F3);
    connect(keyShF3, &QShortcut::activated, this, &TextEdit::onFindText);

    keyEsc = new QShortcut(this);
    keyEsc->setKey(Qt::Key_Escape);
    connect(keyEsc, &QShortcut::activated, this, &TextEdit::onFindText);

    ctrlR = new QShortcut(this);
    ctrlR->setKey(QKeySequence("Ctrl+R"));
    connect(ctrlR, &QShortcut::activated, this, &TextEdit::onTabRefresh);

    ctrlW = new QShortcut(this);
    ctrlW->setKey(QKeySequence("Ctrl+W"));
    connect(ctrlW, &QShortcut::activated, this, &TextEdit::onTabClose);

    ctrlTab = new QShortcut(this);
    ctrlTab->setKey(QKeySequence("Ctrl+Tab"));
    connect(ctrlTab, SIGNAL(activated()), this, SLOT(onNextTab()));

    defaultFont = QFont("Noto Sans", 11);

    setupParams();

    set.beginGroup( "private" );

    editorLayoutState = set.value("editorLayoutState", QByteArray()).toByteArray();

    fileDialogSize = set.value("fileDialogSize", QSize(760, 600)).toSize();

    lastSuffix = set.value("lastSuffix", "").toString();

    saveDir = set.value("saveDir", "").toString();
    if (saveDir.isEmpty())  saveDir = homePath;

    recentFiles = set.value("recentFiles", QStringList()).toStringList();

    slFgColors
        << "#202020"
        << "#0320d3"
        << "#008d00"
        << "#e20000"
        << "#d47202"
        << "#ba2346"
        << "#8f1cdc"
        << "#818100"
        << "#308f94";
    slFgColors = set.value("fgColors", slFgColors).toStringList();
    currFgColorName = set.value("fgColorName", slFgColors[0]).toString();

    slBgColors
        << "#ffff86"
        << "#ffddaa"
        << "#ffcaca"
        << "#edd7ff"
        << "#abe1fb"
        << "#d8f8c4"
        << "#9af0ee"
        << "#e0e0e0";
    slBgColors = set.value("bgColors", slBgColors).toStringList();
    currBgColorName = set.value("bgColorName", slBgColors[0]).toString();

    winConf = new CConfigDialog(this);
    winConf->resize(set.value("winConfSize", QSize(630, 440)).toSize());
    winConf->setHeaderState(set.value("configListHeader", QByteArray()).toByteArray());

    isShrinkHtml = set.value("shrinkHtml", false).toBool();

    set.endGroup();
    set.beginGroup( "public" );

    setPublicParams(set);

    set.endGroup();


    setToolButtonStyle(Qt::ToolButtonIconOnly);
    setupFileActions();
    setupTextActions();

    if (!editorLayoutState.isEmpty())
        restoreState(editorLayoutState);

    nm = 0;
    isHttpRequest = false;
    httpReqCount = 0;
    httpCountdown = -1;
}


void TextEdit::setPublicParams(QSettings &set)
{
    textFont.fromString(set.value("textFont", defaultFont.toString()).toString());
    QString s = set.value("textStyle", "").toString().replace('|', ';');
    if (!s.isEmpty())
        textSS = s;

    thumbHeight = set.value("thumbHeight", 100).toInt();

    isUseNativeFileDlg = set.value("useNativeFileDialogs", true).toBool();
    isNetworkEnabled = set.value("networkEnabled", true).toBool();

    lineWrapWidth = set.value("lineWrapWidth", 0).toInt();
    if (lineWrapWidth < 100 || lineWrapWidth > 3840)
        lineWrapWidth = 0;

    pdfMargins = set.value("pdfMargins", QRect(0, 0, 0, 0)).toRect();

    tabTitleLen = set.value("tabTitleLen", 30).toInt();
}


bool TextEdit::close(QSettings& set)
{
    if (wtTabs->count())
    {
        if (currText()->document()->isModified())
        {
            if (!maybeSave())
                return false;
        }

        int firstIndex = wtTabs->currentIndex();

        for (int i = 0; i < wtTabs->count(); i++)
            if (i != firstIndex && ((CTextEdit*)wtTabs->widget(i))->document()->isModified())
            {
                wtTabs->setCurrentIndex(i);

                if (!maybeSave())
                {
                    return false;
                }
            }
    }

    set.setValue("editorLayoutState", saveState());

    set.setValue("fileDialogSize", fileDialogSize);

    set.setValue("lastSuffix", lastSuffix);
    set.setValue("saveDir", saveDir);
    set.setValue("recentFiles", recentFiles);

    set.setValue("fgColors",  btFgColor->getCustomColorNames());
    set.setValue("fgColorName", currFgColorName);
    set.setValue("bgColors",  btBgColor->getCustomColorNames());
    set.setValue("bgColorName", currBgColorName);

    set.setValue("winConfSize", winConf->size());
    set.setValue("configListHeader", winConf->headerState());

    set.setValue("shrinkHtml", isShrinkHtml);

    return true;
}


void TextEdit::setupParams()
{
    PARAM_STRUCT param;

    param.label = "Default text font";  param.name = "textFont";  param.type = CConfigList::TypeFont;
        param.defValue = defaultFont;  param.comment = "";  params.append(param);

    param.label = "Line wrap width";  param.name = "lineWrapWidth"; param.type = CConfigList::TypeNumber;
        param.defValue = 0;  param.comment = "Pixels. Default 0, no wrap.";  params.append(param);

    param.label = "Thumbnail height";  param.name = "thumbHeight"; param.type = CConfigList::TypeNumber;
        param.defValue = 100;  param.comment = "Pixels. Default 100.";  params.append(param);

    param.label = "Use native file dialogs";  param.name = "useNativeFileDialogs";  param.type = CConfigList::TypeBool;
        param.defValue = true;  param.comment = "";  params.append(param);

    param.label = "Network enabled";  param.name = "networkEnabled";  param.type = CConfigList::TypeBool;
        param.defValue = true;  param.comment = "For paste HTML with images.";  params.append(param);

    param.label = "Tab title length";  param.name = "tabTitleLen";  param.type = CConfigList::TypeNumber;
        param.defValue = TAB_TITLE_LEN;  param.comment = "Characters. Default 30.";  params.append(param);
}


void TextEdit::setupFileActions()
{
    QToolBar *tb = addToolBar(tr("Base actions"));
    tb->setObjectName("Main toolbar");
    tb->setIconSize(QSize(22, 22));

    QMenu *menu = menuBar()->addMenu(tr("&File"));

    recentMenu = new QMenu("&Recent files", this);
    addRecentFile();

    //const QIcon newIcon = QIcon::fromTheme("document-new", QIcon(iconDir + "filenew.png"));
    const QIcon newIcon(iconDir + "filenew.png");
    QAction *a = menu->addAction(newIcon,  tr("&New"), this, &TextEdit::fileNew);
    a->setShortcut(QKeySequence::New);
    tb->addAction(a);

    //const QIcon openIcon = QIcon::fromTheme("document-open", QIcon(iconDir + "fileopen.png"));
    const QIcon openIcon = QIcon(iconDir + "fileopen.png");
    a = menu->addAction(openIcon, tr("&Open"), this, &TextEdit::fileOpen);
    a->setShortcut(QKeySequence::Open);
    btOpen = new QToolButton(this);
    btOpen->setDefaultAction(a);
    btOpen->setPopupMode(QToolButton::MenuButtonPopup);
    btOpen->setMenu(recentMenu);
    tb->addWidget(btOpen);

    //const QIcon saveIcon = QIcon::fromTheme("document-save", QIcon(iconDir + "filesave.png"));
    const QIcon saveIcon = QIcon(iconDir + "filesave.png");
    actionSave = menu->addAction(saveIcon, tr("&Save"), this, &TextEdit::onFileSave);
    actionSave->setShortcut(QKeySequence::Save);
    actionSave->setEnabled(false);
    tb->addAction(actionSave);
    editActs.append(actionSave);

    const QIcon saveAsIcon = QIcon(iconDir + "filesaveas.png");
    a = menu->addAction(saveAsIcon, tr("Save &As"), this, &TextEdit::fileSaveAs);
    a->setPriority(QAction::LowPriority);
    editActs.append(a);

    const QIcon expAsIcon = QIcon(iconDir + "document-export.png");
    a = menu->addAction(expAsIcon, tr("&Export to"), this, &TextEdit::fileExportTo);
    editActs.append(a);

#ifndef QT_NO_PRINTER
    //const QIcon printIcon = QIcon::fromTheme("document-print", QIcon(iconDir + "/fileprint.png"));
    const QIcon printIcon = QIcon(iconDir + "/fileprint.png");
    a = menu->addAction(printIcon, tr("&Print"), this, &TextEdit::filePrint);
    a->setPriority(QAction::LowPriority);
    a->setShortcut(QKeySequence::Print);
    editActs.append(a);

    //const QIcon filePrintIcon = QIcon::fromTheme("fileprint", QIcon(iconDir + "fileprint.png"));
    const QIcon filePrintPreIcon = QIcon(iconDir + "fileprintpreview.png");
    a = menu->addAction(filePrintPreIcon, tr("Print Pre&view"), this, &TextEdit::filePrintPreview);
    editActs.append(a);
#endif

    menu->addSeparator();

    menu->addMenu(recentMenu);
    addRecentFile();

    tb->addSeparator();
    menu->addSeparator();

    a = menu->addAction(tr("&Quit"), this, &QWidget::close);
    a->setShortcut(Qt::CTRL + Qt::Key_Q);
    editActs.append(a);

//------------------------------------------------------------------------------------------------
//setupEditActions

    QMenu *editMenu = menuBar()->addMenu(tr("&Edit"));

    //const QIcon copyIcon = QIcon::fromTheme("edit-copy", QIcon(iconDir + "editcopy.png"));
    const QIcon copyIcon = QIcon(iconDir + "editcopy.png");
    actionCopy = editMenu->addAction(copyIcon, tr("&Copy"), this, &TextEdit::onCopy);
    actionCopy->setPriority(QAction::LowPriority);
    actionCopy->setShortcut(QKeySequence::Copy);
    tb->addAction(actionCopy);
    editActs.append(actionCopy);

    //const QIcon pasteIcon = QIcon::fromTheme("edit-paste", QIcon(iconDir + "editpaste.png"));
    const QIcon pasteIcon = QIcon(iconDir + "editpaste.png");
    actionPaste = editMenu->addAction(pasteIcon, tr("&Paste"), this, &TextEdit::onPaste);
    actionPaste->setPriority(QAction::LowPriority);
    actionPaste->setShortcut(QKeySequence::Paste);
    tb->addAction(actionPaste);
    if (const QMimeData *md = QApplication::clipboard()->mimeData())
        actionPaste->setEnabled(md->hasText());
    editActs.append(actionPaste);

    //const QIcon cutIcon = QIcon::fromTheme("edit-cut", QIcon(iconDir + "editcut.png"));
    const QIcon cutIcon = QIcon(iconDir + "editcut.png");
    actionCut = editMenu->addAction(cutIcon, tr("&Cut"), this, &TextEdit::onCut);
    actionCut->setPriority(QAction::LowPriority);
    actionCut->setShortcut(QKeySequence::Cut);
    tb->addAction(actionCut);
    editActs.append(actionCut);

    //const QIcon undoIcon = QIcon::fromTheme("edit-undo", QIcon(iconDir + "editundo.png"));
    const QIcon undoIcon = QIcon(iconDir + "editundo.png");
    actionUndo = editMenu->addAction(undoIcon, tr("&Undo"), this, &TextEdit::onUndo);
    actionUndo->setShortcut(QKeySequence::Undo);
    actionUndo->setToolTip("Undo\nCtrl+    -  undo all");
    tb->addAction(actionUndo);
    editActs.append(actionUndo);

    const QIcon redoIcon = QIcon(iconDir + "editredo.png");
    actionRedo = editMenu->addAction(redoIcon, tr("&Redo"), this, &TextEdit::onRedo);
    //actionRedo->setShortcut(QKeySequence::Redo);
    actionRedo->setShortcut(Qt::CTRL + Qt::Key_Y);
    actionRedo->setToolTip("Redo\nCtrl+    -  redo all");
    tb->addAction(actionRedo);
    editActs.append(actionRedo);

    const QIcon findIcon = QIcon(iconDir + "editfind.png");
    actFind = editMenu->addAction(findIcon, tr("&Find"), this, &TextEdit::onFindTextPanel);
    actFind->setShortcut(QKeySequence::Find);
    tb->addAction(actFind);
    editActs.append(actFind);

    const QIcon attachIcon = QIcon(iconDir + "attach.png");
    QAction* actAttach = editMenu->addAction(attachIcon, tr("Insert file"), this, &TextEdit::fileAttach);
    editActs.append(actAttach);

    editMenu->addSeparator();

    const QIcon optionIcon = QIcon(iconDir + "options.png");
    a = editMenu->addAction(optionIcon, tr("Options"), this, &TextEdit::onOptions);

    tb->addSeparator();

//------------------------------------------------------------------------------------------------
//setupTextActions

    menu = menuBar()->addMenu(tr("F&ormat"));

    const QIcon boldIcon = QIcon(iconDir + "textbold.png");
    actionTextBold = menu->addAction(boldIcon, tr("&Bold"), this, &TextEdit::textBold);
    //actionTextBold->setShortcut(Qt::CTRL + Qt::Key_B);
    actionTextBold->setPriority(QAction::LowPriority);
    QFont bold;
    bold.setBold(true);
    actionTextBold->setFont(bold);
    tb->addAction(actionTextBold);
    actionTextBold->setCheckable(true);
    editActs.append(actionTextBold);

    const QIcon italicIcon = QIcon(iconDir + "textitalic.png");
    actionTextItalic = menu->addAction(italicIcon, tr("&Italic"), this, &TextEdit::textItalic);
    actionTextItalic->setPriority(QAction::LowPriority);
    //actionTextItalic->setShortcut(Qt::CTRL + Qt::Key_I);
    QFont italic;
    italic.setItalic(true);
    actionTextItalic->setFont(italic);
    tb->addAction(actionTextItalic);
    actionTextItalic->setCheckable(true);
    editActs.append(actionTextItalic);

    const QIcon underlineIcon = QIcon(iconDir + "textunder.png");
    actionTextUnderline = menu->addAction(underlineIcon, tr("&Underline"), this, &TextEdit::textUnderline);
    //actionTextUnderline->setShortcut(Qt::CTRL + Qt::Key_U);
    actionTextUnderline->setPriority(QAction::LowPriority);
    QFont underline;
    underline.setUnderline(true);
    actionTextUnderline->setFont(underline);
    tb->addAction(actionTextUnderline);
    actionTextUnderline->setCheckable(true);
    editActs.append(actionTextUnderline);

    const QIcon largerIcon = QIcon(iconDir + "zoomin.png");
    actionTextLarger = menu->addAction(largerIcon, tr("&Larger"), this, &TextEdit::onTextLarger);
    //actionTextLarger->setShortcut(Qt::CTRL + Qt::Key_L);
    actionTextLarger->setToolTip("Larger\nCtrl+    -  +1px for image\n              -  +1% for table column\nShift+  -  original size for image");
    tb->addAction(actionTextLarger);
    editActs.append(actionTextLarger);

    const QIcon smallerIcon = QIcon(iconDir + "zoomout.png");
    actionTextSmaller = menu->addAction(smallerIcon, tr("&Smaller"), this, &TextEdit::onTextSmaller);
    //actionTextLarger->setShortcut(Qt::CTRL + Qt::Key_M);
    actionTextSmaller->setToolTip("Smaller\nCtrl+     -  -1px for image\n               -  -1% for table column");
    tb->addAction(actionTextSmaller);
    editActs.append(actionTextSmaller);

    tb->addSeparator();
    menu->addSeparator();

    const QIcon indentMoreIcon = QIcon(iconDir + "format-indent-more.png");
    actionIndentMore = menu->addAction(indentMoreIcon, tr("Indent"), this, &TextEdit::indent);
    actionIndentMore->setShortcut(Qt::CTRL + Qt::Key_BracketRight);
    actionIndentMore->setPriority(QAction::LowPriority);
    editActs.append(actionIndentMore);

    const QIcon indentLessIcon = QIcon(iconDir + "format-indent-less.png");
    actionIndentLess = menu->addAction(indentLessIcon, tr("Unindent"), this, &TextEdit::unindent);
    actionIndentLess->setShortcut(Qt::CTRL + Qt::Key_BracketLeft);
    actionIndentLess->setPriority(QAction::LowPriority);
    editActs.append(actionIndentLess);

    tb->addAction(actionIndentMore);
    tb->addAction(actionIndentLess);
    menu->addAction(actionIndentMore);
    menu->addAction(actionIndentLess);

    QMenu* alignMenu = new QMenu(this);

    const QIcon leftIcon = QIcon(iconDir + "textleft.png");
    a = new QAction(leftIcon, tr("Left"), this);
    a->setData((int)Qt::AlignLeft|Qt::AlignAbsolute);
    connect(a, SIGNAL(triggered()), this, SLOT(onAlignMenuAct()));
    alignMenu->addAction(a);

    const QIcon centerIcon = QIcon(iconDir + "textcenter.png");
    a = new QAction(centerIcon, tr("Center"), this);
    a->setData((int)Qt::AlignHCenter);
    connect(a, SIGNAL(triggered()), this, SLOT(onAlignMenuAct()));
    alignMenu->addAction(a);

    const QIcon rightIcon = QIcon(iconDir + "textright.png");
    a = new QAction(rightIcon, tr("Right"), this);
    a->setData((int)Qt::AlignRight|Qt::AlignAbsolute);
    connect(a, SIGNAL(triggered()), this, SLOT(onAlignMenuAct()));
    alignMenu->addAction(a);

    const QIcon fillIcon = QIcon(iconDir + "textjustify.png");
    a = new QAction(fillIcon, tr("Justify"), this);
    a->setData((int)Qt::AlignJustify);
    connect(a, SIGNAL(triggered()), this, SLOT(onAlignMenuAct()));
    alignMenu->addAction(a);

    btAlign = new QToolButton(this);
    btAlign->setIcon(leftIcon);
    btAlign->setToolTip("Text align");
    btAlign->setPopupMode(QToolButton::MenuButtonPopup);
    btAlign->setMenu(alignMenu);
    tb->addWidget(btAlign);

    btAlign->setProperty("align", (int)Qt::AlignLeft|Qt::AlignAbsolute);
    connect(btAlign, SIGNAL(clicked(bool)), this, SLOT(onTextAlign(bool)));

    const QIcon checkboxIcon = QIcon(iconDir + "checkbox-checked.png");
    actionToggleCheckState = menu->addAction(checkboxIcon, tr("Task list"), this, &TextEdit::setChecked);
    //actionToggleCheckState->setShortcut(Qt::CTRL + Qt::Key_K);
    actionToggleCheckState->setCheckable(true);
    actionToggleCheckState->setPriority(QAction::LowPriority);
    tb->addAction(actionToggleCheckState);
    editActs.append(actionToggleCheckState);

    QMenu* tableMenu = new QMenu(this);
    tableMenu->setStyleSheet("QMenu::icon { width: 22; height:22; }");
    tableMenu->setToolTipsVisible(true);

    const QIcon createTabIcon = QIcon(iconDir + "table-create.png");
    a = new QAction(createTabIcon, tr("Create table"), this);
    a->setToolTip("Shift+  -  with % column width");
    a->setData("CT");
    connect(a, &QAction::triggered, this, &TextEdit::onTableMenuAct);
    tableMenu->addAction(a);

    const QIcon addTabRowIcon = QIcon(iconDir + "table-add-row.png");
    a = new QAction(addTabRowIcon, tr("Add row"), this);
    a->setData("AR");
    connect(a, &QAction::triggered, this, &TextEdit::onTableMenuAct);
    tableMenu->addAction(a);

    const QIcon addTabColIcon = QIcon(iconDir + "table-add-column.png");
    a = new QAction(addTabColIcon, tr("Add column"), this);
    a->setData("AC");
    connect(a, &QAction::triggered, this, &TextEdit::onTableMenuAct);
    tableMenu->addAction(a);

    const QIcon insTabRowIcon = QIcon(iconDir + "table-insert-row.png");
    a = new QAction(insTabRowIcon, tr("Insert row"), this);
    a->setData("IR");
    connect(a, &QAction::triggered, this, &TextEdit::onTableMenuAct);
    tableMenu->addAction(a);

    const QIcon insTabColIcon = QIcon(iconDir + "table-insert-column.png");
    a = new QAction(insTabColIcon, tr("Insert column"), this);
    a->setData("IC");
    connect(a, &QAction::triggered, this, &TextEdit::onTableMenuAct);
    tableMenu->addAction(a);

    const QIcon delTabRowIcon = QIcon(iconDir + "table-delete-row.png");
    a = new QAction(delTabRowIcon, tr("Delete row"), this);
    a->setData("DR");
    connect(a, &QAction::triggered, this, &TextEdit::onTableMenuAct);
    tableMenu->addAction(a);

    const QIcon delTabColIcon = QIcon(iconDir + "table-delete-column.png");
    a = new QAction(delTabColIcon, tr("Delete column"), this);
    a->setData("DC");
    connect(a, &QAction::triggered, this, &TextEdit::onTableMenuAct);
    tableMenu->addAction(a);

    const QIcon mergeCellIcon = QIcon(iconDir + "table-cell-merge.png");
    a = new QAction(mergeCellIcon, tr("Merge selected cells"), this);
    a->setData("MC");
    connect(a, &QAction::triggered, this, &TextEdit::onTableMenuAct);
    tableMenu->addAction(a);

    const QIcon splitCellIcon = QIcon(iconDir + "table-cell-split.png");
    a = new QAction(splitCellIcon, tr("Split cell"), this);
    a->setData("SC");
    connect(a, &QAction::triggered, this, &TextEdit::onTableMenuAct);
    tableMenu->addAction(a);

    const QIcon tableIcon = QIcon(iconDir + "table.png");
    actTableNoAct = new QAction(tableIcon, tr("Table menu"), this);

    btTable = new QToolButton(this);
    //btTable->setIcon(tableIcon);
    //btTable->setToolTip("Table menu");
    btTable->setDefaultAction(actTableNoAct);
    btTable->setPopupMode(QToolButton::MenuButtonPopup);
    btTable->setMenu(tableMenu);
    tb->addWidget(btTable);

    tb->addSeparator();
    menu->addSeparator();

    btFgColor = new CColorToolButton(this);
    btFgColor->setToolTip("Text color");
    btFgColor->setCustomColors(slFgColors);
    btFgColor->setCurrentColor(QColor(currFgColorName));
    tb->addWidget(btFgColor);

    connect(btFgColor, &CColorToolButton::selected, this, &TextEdit::onFgColor);

    btBgColor = new CColorToolButton(this);
    btBgColor->setToolTip("Marker color\nShift+  -  for text block");
    btBgColor->setCustomColors(slBgColors);
    btBgColor->setCurrentColor(QColor(currBgColorName));
    tb->addWidget(btBgColor);

    connect(btBgColor, &CColorToolButton::selected, this, &TextEdit::onBgColor);

    tb->addSeparator();

    tb->addAction(actAttach);

    setEditActsEnabled(false);

    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));

    actFileBrowser = viewMenu->addAction(tr("File browser"), this, &TextEdit::onFileBrowser);
    actFileBrowser->setCheckable(true);
    //a->setShortcut(QKeySequence::F2);
    //actFileBrowser->setChecked(isFileBrowserVisible);


    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(tr("&Help"), this, &TextEdit::onHelp);
    helpMenu->addAction(tr("&About"), this, &TextEdit::about);
    helpMenu->addAction(tr("About &Qt"), qApp, &QApplication::aboutQt);
}


void TextEdit::setEditActsEnabled(bool enabled)
{
    foreach (QAction* act, editActs)
    {
        act->setEnabled(enabled);
    }

    btAlign->setEnabled(enabled);
    btTable->setEnabled(enabled);
    btFgColor->setEnabled(enabled);
    btBgColor->setEnabled(enabled);
}


void TextEdit::setupTextActions()
{
    tbFont = addToolBar(tr("Font family actions"));
    tbFont->setObjectName("Font toolbar");
    tbFont->setIconSize(QSize(22, 22));
    tbFont->setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);
    addToolBarBreak(Qt::TopToolBarArea);
    addToolBar(tbFont);

    comboStyle = new QComboBox(tbFont);
    tbFont->addWidget(comboStyle);
    comboStyle->addItem("Standard");
    comboStyle->addItem("Bullet List (Disc)");
    comboStyle->addItem("Bullet List (Circle)");
    comboStyle->addItem("Bullet List (Square)");
    comboStyle->addItem("Task List (Unchecked)");
    comboStyle->addItem("Task List (Checked)");
    comboStyle->addItem("Ordered List (Decimal)");
    comboStyle->addItem("Ordered List (Alpha lower)");
    comboStyle->addItem("Ordered List (Alpha upper)");
    comboStyle->addItem("Ordered List (Roman lower)");
    comboStyle->addItem("Ordered List (Roman upper)");
    comboStyle->addItem("Heading 1");
    comboStyle->addItem("Heading 2");
    comboStyle->addItem("Heading 3");
    comboStyle->addItem("Heading 4");
    comboStyle->addItem("Heading 5");
    comboStyle->addItem("Heading 6");

    connect(comboStyle, QOverload<int>::of(&QComboBox::activated), this, &TextEdit::textStyle);

    tbFont->addSeparator();

    comboFont = new QFontComboBox(tbFont);
    tbFont->addWidget(comboFont);
    connect(comboFont, &QComboBox::textActivated, this, &TextEdit::textFamily);

    comboSize = new QComboBox(tbFont);
    comboSize->setObjectName("comboSize");
    comboSize->setEditable(true);

    stFontSizes = QFontDatabase::standardSizes();
    for (int size : stFontSizes)
    {
        comboSize->addItem(QString::number(size));
        //qDebug() << "stFontSizes:  " << size;
    }
    tbFont->addWidget(comboSize);

    connect(comboSize, &QComboBox::textActivated, this, &TextEdit::textSize);
}


void TextEdit::onHelp()
{
    QMessageBox::information(this, appName, tr("Shortcuts:\n\n"
             "Shift+Space or Shift+Enter  -  break current text format, keep current font\n"
             "Ctrl+Space or Ctrl+Enter  -  break current text format, set default font\n"
             "Ctrl+Down  -  create new block, set default font\n"
             "\nSee tooltips for buttons and menus."
        ));
}


void TextEdit::about()
{
    QMessageBox::about(this, tr("About"), tr("mdqtee 1.1.2     (MD QTextEdit Example)        \n"
        "MD 1.11.2021\n\n"
        "Qt Project MESSAGE (about zip): This project is using private headers and will therefore be tied "
        "to this specific Qt module build version. Running this project against other versions "
        "of the Qt modules may crash at any arbitrary point. "
        "This is not a bug, but a result of using Qt internals. You have been warned!"
        ));
}


void TextEdit::addTab(QString filePath)
{
    bool isNewDoc = filePath == untitledFileName;

    CTextEdit* ted = new CTextEdit(this);
    ted->setStyleSheet("QTextEdit { background: #fafafa; color: #202020; font-family:'" + textFont.family()
                       + "'; font-size:" + QString::number(textFont.pointSize()) + "pt; font-style:normal; }");
    ted->setFontPointSize(textFont.pointSize());  //++ for first input
    ted->document()->setDefaultStyleSheet("table { border-collapse: collapse; border: 1px solid black; } ");
    ted->document()->setDocumentMargin(6);
    ted->setThumbnailHeight(thumbHeight);

    connect(ted, &QTextEdit::cursorPositionChanged, this, &TextEdit::cursorPositionChanged, Qt::QueuedConnection);
    connect(ted, &QTextEdit::currentCharFormatChanged, this, &TextEdit::currentCharFormatChanged);
    connect(ted, &CTextEdit::linkClicked, this, &TextEdit::onLinkClicked);
    connect(ted, &CTextEdit::cursorOnLink, this, &TextEdit::onCursorOnLink);
    connect(ted, &CTextEdit::doubleClick, this, &TextEdit::onTextDoubleClick, Qt::QueuedConnection);
    connect(ted, &CTextEdit::error, this, &TextEdit::onTedError, Qt::QueuedConnection);
    connect(ted, &CTextEdit::saveAsFile, this, &TextEdit::onSaveResAsFile, Qt::QueuedConnection);
    connect(ted, &CTextEdit::addResFile, this, &TextEdit::onAddResFile);

    connect(ted->document(), &QTextDocument::modificationChanged, this, &TextEdit::onTextChanged);
    connect(ted->document(), &QTextDocument::undoAvailable, actionUndo, &QAction::setEnabled);
    connect(ted->document(), &QTextDocument::redoAvailable, actionRedo, &QAction::setEnabled);

    actionCut->setEnabled(false);
    connect(ted, &QTextEdit::copyAvailable, actionCut, &QAction::setEnabled);
    actionCopy->setEnabled(false);
    connect(ted, &QTextEdit::copyAvailable, actionCopy, &QAction::setEnabled);

    if (wtTabs->count() == 0)
        setEditActsEnabled(true);

    QString fileName = QFileInfo(filePath).fileName();

    int i = wtTabs->addTab(ted, cutStr(fileName, tabTitleLen));
    if (!isNewDoc)
    {
        wtTabs->setTabToolTip(i, filePath);
        ted->zipFileName = filePath;   //  for setCurrentFileName  in  onTabChanged()
    }
    wtTabs->setCurrentIndex(i);

    ted->setNetworkEnabled(isNetworkEnabled);

    QFontMetrics fm(textFont);
    ted->setTabStopWidth(4 * fm.width(' '));

    if (lineWrapWidth)
    {
        ted->setLineWrapMode(QTextEdit::FixedPixelWidth);
        ted->setLineWrapColumnOrWidth(lineWrapWidth);
    }
}


void TextEdit::onTabClose()
{
    if (isHttpRequest)  return;

    if (maybeSave())
        closeTab();
}


void TextEdit::onTabCloseRequest(int index)
{
    if (isHttpRequest)  return;

    int currIndex = wtTabs->currentIndex();
    QGuiApplication::restoreOverrideCursor();

    if (maybeSave(index))
    {
        closeTab(index);
        if (currIndex < wtTabs->count())
            wtTabs->setCurrentIndex(currIndex);
    }
}


void TextEdit::closeTab(int index)
{
    int i = index;
    if (index < 0)  i =  wtTabs->currentIndex();
    if (i < 0)  return;
    CTextEdit* ted = (CTextEdit*)wtTabs->widget(i);

    disconnect(ted, &QTextEdit::cursorPositionChanged, this, &TextEdit::cursorPositionChanged);
    disconnect(ted, &QTextEdit::currentCharFormatChanged, this, &TextEdit::currentCharFormatChanged);
    disconnect(ted, &CTextEdit::linkClicked, this, &TextEdit::onLinkClicked);
    disconnect(ted, &CTextEdit::doubleClick, this, &TextEdit::onTextDoubleClick);
    disconnect(ted, &CTextEdit::cursorOnLink, this, &TextEdit::onCursorOnLink);
    disconnect(ted, &CTextEdit::error, this, &TextEdit::onTedError);
    disconnect(ted, &CTextEdit::saveAsFile, this, &TextEdit::onSaveResAsFile);
    disconnect(ted, &CTextEdit::addResFile, this, &TextEdit::onAddResFile);

    disconnect(ted->document(), &QTextDocument::modificationChanged, this, &TextEdit::onTextChanged);
    disconnect(ted->document(), &QTextDocument::undoAvailable, actionUndo, &QAction::setEnabled);
    disconnect(ted->document(), &QTextDocument::redoAvailable, actionRedo, &QAction::setEnabled);

    disconnect(ted, &QTextEdit::copyAvailable, actionCut, &QAction::setEnabled);
    disconnect(ted, &QTextEdit::copyAvailable, actionCopy, &QAction::setEnabled);

    wtTabs->removeTab(i);
}


CTextEdit* TextEdit::currText() const
{
    if (!wtTabs->count())  return nullptr;
    return qobject_cast<CTextEdit*>(wtTabs->currentWidget());
}


void TextEdit::setTabModified(bool m)
{
    int i = wtTabs->currentIndex();
    QString s = wtTabs->tabText(i);
    if (s.indexOf('*') >= 0 != m)
    {
        s = s.remove('*') + (m ? "*" : "");
        wtTabs->setTabText(i, s);
    }
}


void TextEdit::onTextChanged(bool m)
{
    // called once, no repetitions when entering letters
    actionSave->setEnabled(m);
    setTabModified(m);
    setWindowModified(m);
}


void TextEdit::onUndoAvailable(bool m)
{
    actionUndo->setEnabled(m);
}


void TextEdit::onRedoAvailable(bool m)
{
    actionRedo->setEnabled(m);
}


void TextEdit::onAlignMenuAct()
{
    QAction* a = qobject_cast<QAction*>(sender());
    int align = a->data().toInt();

    currText()->setAlignment((Qt::Alignment)align);

    btAlign->setIcon(a->icon());
    btAlign->setProperty("align", align);
}


void TextEdit::onTextAlign(bool checked)
{
    int align = btAlign->property("align").toInt();
    currText()->setAlignment((Qt::Alignment)align);
}


void TextEdit::onTabChanged(int index)
{
    if (index < 0)
    {
        setEditActsEnabled(false);
        setCurrentFileName("");
        return;
    }

    CTextEdit* ted = currText();
    if (!ted)  return;

    cursorPositionChanged();  // lists and headings only
    fontChanged(ted->currentCharFormat().font());

    actionSave->setEnabled(ted->document()->isModified());
    actionUndo->setEnabled(ted->document()->isUndoAvailable());
    actionRedo->setEnabled(ted->document()->isRedoAvailable());

    findTextPanel->setTextEdit(ted);

    QString fileName = ted->zipFileName;
    if (fileName.isEmpty())
        fileName = untitledFileName;
    setCurrentFileName(fileName);
    setWindowModified(ted->document()->isModified());
}


bool TextEdit::load(const QString &f, int mode)
{
    if (!QFile::exists(f))
    {
        recentFiles.removeAll(f);
        addRecentFile("");
        QMessageBox::warning(this, appName, "File not found.\n" + f, QMessageBox::Ok);
        return false;
    }

    CTextEdit* ted;

    for (int i = 0; i < wtTabs->count(); i++)
    {
        ted = (CTextEdit*)wtTabs->widget(i);
        if (ted->zipFileName == f)
        {
            wtTabs->setCurrentIndex(i);
            return true;
        }
    }

    if (mode == 0)
        addTab(f);
    ted = currText();  // after addTab!

    QString fn = f;
    QString suffix = QFileInfo(fn).suffix();

    CWaitCursor cur;

    if (suffix == "html" || suffix == "md")
    {
        if (!openTempDir(f)) return false;
        fn = ted->getFileName();
    }

    if (suffix == "maff")
    {
        if (!unpackMaff(f)) return false;
        fn = ted->getFileName();
    }

    QFile file(fn);
    if (!file.open(QFile::ReadOnly))
        return false;

    QByteArray data = file.readAll();
    QTextCodec *codec = Qt::codecForHtml(data);
    QString str = codec->toUnicode(data);
    QUrl baseUrl = (fn.front() == QLatin1Char(':') ? QUrl(f) : QUrl::fromLocalFile(f)).adjusted(QUrl::RemoveFilename);

    ted->document()->setBaseUrl(baseUrl);
    ted->setFileName(fn);
    ted->mime = suffix;
    if (suffix != "txt")
        ted->setFilesDir(QFileInfo(fn).path() + "/" + QFileInfo(fn).baseName() + "_files");
    ted->zipFileName = f;

    //if (Qt::mightBeRichText(str))
    if (suffix == "html" || suffix == "maff")
    {
        ted->setHtml(str);
    }
    else if (suffix == "md")
    {
        ted->setMarkdown(str);
    }
    else
    {
        ted->setPlainText(QString::fromLocal8Bit(data));
        ted->mime = "txt";
    }

    ted->setFocus();
    ted->moveCursor(QTextCursor::Start);  // = cursorPositionChanged();  fontChanged();

    statusBarMessage(tr("temp file: %1").arg(ted->getFileName()));
    addRecentFile(ted->zipFileName);
    saveDir = QFileInfo(ted->zipFileName).path();

    return true;
}


bool TextEdit::maybeSave(int index)
{
    if (wtTabs->count() == 0)  return true;
    if (index >= wtTabs->count())  return true;

    int i = index;
    if (index < 0)  i =  wtTabs->currentIndex();
    CTextEdit* ted = (CTextEdit*)wtTabs->widget(i);

    if (!ted->document()->isModified())
        return true;

    if (index >= 0)
        wtTabs->setCurrentIndex(index);

    const QMessageBox::StandardButton ret =
        QMessageBox::warning(this, appName,
                             tr("The document [%1] has been modified.\n"
                                "Do you want to save your changes?").arg(wtTabs->currentIndex() + 1),
                             QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    if (ret == QMessageBox::Save)
        return fileSave("");
    else if (ret == QMessageBox::Cancel)
        return false;

    return true;
}


void TextEdit::setCurrentFileName(const QString &fileName)
{
    if (fileName.isEmpty())
        setWindowTitle(QCoreApplication::applicationName());
    else
        setWindowTitle(tr("%1[*] - %2").arg(cutStr(QFileInfo(fileName).fileName(), 100), QCoreApplication::applicationName()));

    setWindowModified(false);
}


void TextEdit::fileNew()
{
    addTab(untitledFileName);

    QString docDir = tempDir + "/" + genTempDirName();
    QString filesDir = docDir + "/index_files";
    QDir dir(docDir);
    dir.removeRecursively();

    if (!createFilesDir(docDir))  return;
    if (!createFilesDir(filesDir))  return;

    CTextEdit* ted = currText();

    ted->setFileName(docDir + "/index.html");
    ted->setFilesDir(filesDir);
    ted->tempPath = docDir;

    QString html = "<html><head><style type='text/css'>" + textSS.replace("|", ";") + "</style></head><body></body></html>";
    ted->document()->setHtml(html);  // can set text color for body
    ted->document()->setModified(false);
    ted->setFocus();

    setCurrentFileName(untitledFileName);
    statusBarMessage(tr("temp file dir: %1").arg(docDir));
}


void TextEdit::fileOpen()
{
    QFileDialog dlg(this, tr("Open file"));
    if (!isUseNativeFileDlg)
        dlg.setOption(QFileDialog::DontUseNativeDialog);
    //dlg.setFilter(QDir::Hidden | QDir::NoDotAndDotDot);
    dlg.setNameFilters(QStringList()
        << "All files (*)"
        << mimeTypes["maff"]
        << mimeTypes["html"]
        << mimeTypes["md"]
    );
    dlg.setAcceptMode(QFileDialog::AcceptOpen);
    dlg.setFileMode(QFileDialog::ExistingFile);
    dlg.setViewMode(QFileDialog::Detail);
    dlg.resize(fileDialogSize);
    dlg.selectNameFilter(dlgCurrFilter[0]);

    if (dlg.exec() != QDialog::Accepted)
        return;

    QString fn = dlg.selectedFiles().first();
    fileDialogSize = dlg.size();
    dlgCurrFilter[0] = dlg.selectedNameFilter();

    load(fn);
}


void TextEdit::onFileSave()
{
    if (!wtTabs->count())  return;

    fileSave("");
}


bool TextEdit::fileSave(QString name, QString suff)
{
    if (!wtTabs->count())  return true;
    CTextEdit* ted = currText();

    QString fileName = name;
    QString suffix = suff;
    if (fileName.isEmpty())
        fileName = ted->zipFileName;
    if (suffix.isEmpty())
        suffix = ted->mime;

    if (fileName.isEmpty())
        return fileSaveAs();
    if (suffix.isEmpty())
        suffix = "html";

    QString filesDir  = QFileInfo(fileName).path() + "/" + QFileInfo(fileName).baseName() + "_files";
    bool ok = false;

    CWaitCursor cur;

    if (suffix == "pdf")
    {
        QPrinter printer(QPrinter::HighResolution);
        printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setOutputFileName(fileName);
        ted->document()->print(&printer);

        statusBarMessage(tr("Exported \"%1\"").arg(QDir::toNativeSeparators(fileName)));
        return true;
    }
    else if (suffix == "maff")
    {
        lastSuffix = suffix;
        ok = saveToMaff(fileName);
    }
    else if (suffix == "html" || suffix == "md")
    {
        lastSuffix = suffix;
        ok = saveToHtml(fileName, suffix);
    }
    else
    {
        QTextDocumentWriter writer(fileName);
        writer.setFormat(suffix.toLatin1());

        ok = writer.write(ted->document());
    }

    if (ok)
    {
        ted->document()->setModified(false);

        if (suffix == "md" && ted->mime != "md")
            load(fileName, 1);
        else
        {
            ted->zipFileName = fileName;
            ted->mime = suffix;

            setCurrentFileName(ted->zipFileName);
            addRecentFile(ted->zipFileName);
        }

        int i = wtTabs->currentIndex();
        wtTabs->setTabText(i, cutStr(QFileInfo(ted->zipFileName).fileName(), tabTitleLen));
        wtTabs->setTabToolTip(i, ted->zipFileName);
        statusBarMessage(tr("saved: \"%1\"").arg(QDir::toNativeSeparators(fileName)));
    }
    else
    {
        statusBarMessage(tr("Could not save to file \"%1\"").arg(QDir::toNativeSeparators(fileName)));
    }

    return ok;
}


bool TextEdit::fileSaveAs()
{
    if (!wtTabs->count())  return false;

    CTextEdit* ted = currText();

    QFileDialog dlg(this, tr("Save as"));
    dlg.setAcceptMode(QFileDialog::AcceptSave);
    if (!isUseNativeFileDlg)
        dlg.setOption(QFileDialog::DontUseNativeDialog);
    //dlg.setFilter(QDir::NoDotAndDotDot);  //--  // QDir::Hidden - List hidden files (starting with a ".").
    if (ted->zipFileName.isEmpty())
    {
        if (!fileBrowserDir.isEmpty())
            dlg.setDirectory(fileBrowserDir + "/");
        else
            dlg.setDirectory(saveDir + "/");
    }
    QStringList filters;
    if (ted->mime == "md")
        filters << mimeTypes["md"];
    else
        filters << mimeTypes["maff"] << mimeTypes["html"]  << mimeTypes["md"];
    dlg.setNameFilters(filters);
    dlg.setViewMode(QFileDialog::Detail);
    dlg.selectNameFilter(dlgCurrFilter[1]);
    dlg.selectFile(QFileInfo(ted->zipFileName).baseName() + "." + mimeTypes.key(dlgCurrFilter[1]));
    dlg.resize(fileDialogSize);

    if (dlg.exec() != QDialog::Accepted)
        return false;

    QString fn = dlg.selectedFiles().first();
    saveDir = dlg.directory().path();
    fileDialogSize = dlg.size();
    dlgCurrFilter[1] = dlg.selectedNameFilter();
    QString suffix = mimeTypes.key(dlgCurrFilter[1]);

    return fileSave(fn, suffix);
}


bool TextEdit::fileExportTo()
{
    if (!wtTabs->count())  return false;
    bool ok = false;

    CTextEdit* ted = currText();

    QFileDialog dlg(this, tr("Export to"));
    if (!isUseNativeFileDlg)
        dlg.setOption(QFileDialog::DontUseNativeDialog);
    dlg.setNameFilters(QStringList()
        << mimeTypes["pdf"]
        << mimeTypes["epub"]
        << mimeTypes["odt"]
        << mimeTypes["md"]
        << mimeTypes["txt"]
    );
    dlg.setAcceptMode(QFileDialog::AcceptSave);
    dlg.selectFile(QFileInfo(ted->zipFileName).baseName());
    dlg.setViewMode(QFileDialog::Detail);
    dlg.resize(fileDialogSize);
    dlg.selectNameFilter(dlgCurrFilter[2]);

    if (dlg.exec() != QDialog::Accepted)
        return false;

    QString suffix = mimeTypes.key(dlg.selectedNameFilter());
    QString fn = dlg.selectedFiles().first();
    fileDialogSize = dlg.size();
    dlgCurrFilter[2] = dlg.selectedNameFilter();

    if (QFileInfo(fn).suffix().isEmpty())
        fn.append("." + suffix);

    CWaitCursor cur;

    if (suffix == "pdf")
    {
        QMargins margins(pdfMargins.left(), pdfMargins.top(), pdfMargins.width(), pdfMargins.height());

        // For PDF printing, sets the resolution of the PDF driver to 1200 dpi.
        QPrinter::PrinterMode res = QPrinter::HighResolution;
        if (!margins.isNull())  res = QPrinter::ScreenResolution;

        QPrinter printer(res);
        printer.setPageSize(QPrinter::A4);
        //printer.setFullPage(true); // must be before setmargin
        printer.setPageMargins(margins); // qreal left, top, right, bottom
        printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setOutputFileName(fn);

        qDebug() << "fileExportTo:  pageRect=" << printer.pageRect().size();
        QTextDocument doc(ted->document());

        if (!margins.isNull())
            doc.setPageSize(QSizeF(printer.pageRect().size()));

        doc.setHtml(ted->document()->toHtml("utf-8"));
        doc.print(&printer);

        ok = true;
    }
    else if (suffix == "epub")
    {
        ok = saveToEpub(fn);
    }
    else
    {
        QTextDocumentWriter writer(fn);
        writer.setFormat(suffix.toLatin1());
        ok = writer.write(ted->document());
    }

    if (!ok)
        QMessageBox::warning(this, appName, "Export error.", QMessageBox::Ok);
    else
        statusBarMessage(tr("Exported \"%1\"").arg(QDir::toNativeSeparators(fn)));

    return ok;
}


void TextEdit::filePrint()
{
    if (!wtTabs->count())  return;

#if QT_CONFIG(printdialog)
    QPrinter printer(QPrinter::PrinterResolution);
    QPrintDialog *dlg = new QPrintDialog(&printer, this);

    CTextEdit* ted = currText();

    if (ted->textCursor().hasSelection())
        dlg->addEnabledOption(QAbstractPrintDialog::PrintSelection);

    dlg->setWindowTitle(tr("Print Document"));

    if (dlg->exec() == QDialog::Accepted)
        ted->print(&printer);

    delete dlg;
#endif
}


void TextEdit::filePrintPreview()
{
#if QT_CONFIG(printpreviewdialog)
    QPrinter printer(QPrinter::HighResolution);
    QPrintPreviewDialog preview(&printer, this);
    connect(&preview, &QPrintPreviewDialog::paintRequested, this, &TextEdit::printPreview);
    preview.exec();
#endif
}


void TextEdit::printPreview(QPrinter *printer)
{
    if (!wtTabs->count())  return;

#ifdef QT_NO_PRINTER
    Q_UNUSED(printer);
#else
    currText()->print(printer);
#endif
}


void TextEdit::filePrintPdf()
{
    if (!wtTabs->count())  return;

#ifndef QT_NO_PRINTER
//! [0]
    QFileDialog dlg(this, tr("Export to"));
    if (!isUseNativeFileDlg)
        dlg.setOption(QFileDialog::DontUseNativeDialog);
    dlg.setMimeTypeFilters(QStringList("application/pdf"));
    dlg.setDefaultSuffix("pdf");
    dlg.setViewMode(QFileDialog::Detail);
    dlg.setAcceptMode(QFileDialog::AcceptSave);
    dlg.resize(fileDialogSize);

    if (dlg.exec() != QDialog::Accepted)
        return;

    QString fileName = dlg.selectedFiles().first();
    fileDialogSize = dlg.size();

    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(fileName);
    currText()->document()->print(&printer);
    statusBarMessage(tr("Exported \"%1\"").arg(QDir::toNativeSeparators(fileName)));
//! [0]
#endif
}


void TextEdit::textBold()
{
    QTextCharFormat fmt;
    fmt.setFontWeight(actionTextBold->isChecked() ? QFont::Bold : QFont::Normal);
    mergeFormatOnWordOrSelection(fmt);
}


void TextEdit::textUnderline()
{
    QTextCharFormat fmt;
    fmt.setFontUnderline(actionTextUnderline->isChecked());
    mergeFormatOnWordOrSelection(fmt);
}


void TextEdit::textItalic()
{
    QTextCharFormat fmt;
    fmt.setFontItalic(actionTextItalic->isChecked());
    mergeFormatOnWordOrSelection(fmt);
}


void TextEdit::textFamily(const QString &f)
{
    QTextCharFormat fmt;
    fmt.setFontFamily(f);
    mergeFormatOnWordOrSelection(fmt);
}


void TextEdit::textSize(const QString &p)
{
    qreal pointSize = p.toFloat();
    if (p.toFloat() > 0)
    {
        QTextCharFormat fmt;
        fmt.setFontPointSize(pointSize);
        mergeFormatOnWordOrSelection(fmt);
    }
}


void TextEdit::textStyle(int styleIndex)
{
    CTextEdit* ted = currText();
    QTextCursor cursor = ted->textCursor();
    QTextListFormat::Style style = QTextListFormat::ListStyleUndefined;
    QTextBlockFormat::MarkerType marker = QTextBlockFormat::MarkerType::NoMarker;

    switch (styleIndex)
    {
    case 1:
        style = QTextListFormat::ListDisc;
        break;
    case 2:
        style = QTextListFormat::ListCircle;
        break;
    case 3:
        style = QTextListFormat::ListSquare;
        break;
    case 4:
        if (cursor.currentList())
            style = cursor.currentList()->format().style();
        else
            style = QTextListFormat::ListDisc;
        marker = QTextBlockFormat::MarkerType::Unchecked;
        break;
    case 5:
        if (cursor.currentList())
            style = cursor.currentList()->format().style();
        else
            style = QTextListFormat::ListDisc;
        marker = QTextBlockFormat::MarkerType::Checked;
        break;
    case 6:
        style = QTextListFormat::ListDecimal;
        break;
    case 7:
        style = QTextListFormat::ListLowerAlpha;
        break;
    case 8:
        style = QTextListFormat::ListUpperAlpha;
        break;
    case 9:
        style = QTextListFormat::ListLowerRoman;
        break;
    case 10:
        style = QTextListFormat::ListUpperRoman;
        break;
    default:
        break;
    }

    cursor.beginEditBlock();

    QTextBlockFormat blockFmt = cursor.blockFormat();

    if (style == QTextListFormat::ListStyleUndefined)
    {
        blockFmt.setObjectIndex(-1);

        int headingLevel = styleIndex >= 11 ? styleIndex - 11 + 1 : 0; // H1 to H6, or Standard

        blockFmt.setHeadingLevel(headingLevel);
        cursor.setBlockFormat(blockFmt);

        int sizeAdjustment = headingLevel ? 4 - headingLevel : 0; // H1 to H6: +3 to -2

        QTextCharFormat fmt;
        fmt.setFontWeight(headingLevel ? QFont::Bold : QFont::Normal);
        fmt.setProperty(QTextFormat::FontSizeAdjustment, sizeAdjustment);
        fmt.setFontPointSize(11);

        cursor.select(QTextCursor::LineUnderCursor);
        cursor.mergeCharFormat(fmt);
        ted->mergeCurrentCharFormat(fmt);
    }
    else
    {
        blockFmt.setMarker(marker);
        cursor.setBlockFormat(blockFmt);
        QTextListFormat listFmt;
        if (cursor.currentList())
        {
            listFmt = cursor.currentList()->format();
        }
        else
        {
            listFmt.setIndent(blockFmt.indent() + 1);
            blockFmt.setIndent(0);
            cursor.setBlockFormat(blockFmt);
        }

        listFmt.setStyle(style);
        cursor.createList(listFmt);
    }

    cursor.endEditBlock();
}


void TextEdit::setChecked(bool checked)
{
    textStyle(checked ? 5 : 4);
}


void TextEdit::indent()
{
    modifyIndentation(1);
}


void TextEdit::unindent()
{
    modifyIndentation(-1);
}


void TextEdit::modifyIndentation(int amount)
{
    QTextCursor cursor = currText()->textCursor();

    cursor.beginEditBlock();

    if (cursor.currentList())
    {
        QTextListFormat listFmt = cursor.currentList()->format();
        int listIndent = listFmt.indent();
        // See whether the line above is the list we want to move this item into,
        // or whether we need a new list.
        QTextCursor above(cursor);
        above.movePosition(QTextCursor::Up);

        if (above.currentList() && listIndent + amount == above.currentList()->format().indent())
        {
            above.currentList()->add(cursor.block());
        }
        else
        {
            listFmt.setIndent(listIndent + amount);
            cursor.createList(listFmt);
        }
    }
    else
    {
        QTextBlockFormat blockFmt = cursor.blockFormat();
        int currIndent = blockFmt.indent();

        if ((amount > 0 && currIndent < 100) || (amount < 0 && currIndent > 0))
        {
            blockFmt.setIndent(currIndent + amount);
            cursor.setBlockFormat(blockFmt);
        }
    }

    cursor.endEditBlock();
}


void TextEdit::currentCharFormatChanged(const QTextCharFormat &format)
{
    fontChanged(format.font());
}


void TextEdit::cursorPositionChanged()
{
    CTextEdit* ted = currText();
    QTextList *list = ted->textCursor().currentList();

    if (list)
    {
        switch (list->format().style())
        {
        case QTextListFormat::ListDisc:
            comboStyle->setCurrentIndex(1);
            break;
        case QTextListFormat::ListCircle:
            comboStyle->setCurrentIndex(2);
            break;
        case QTextListFormat::ListSquare:
            comboStyle->setCurrentIndex(3);
            break;
        case QTextListFormat::ListDecimal:
            comboStyle->setCurrentIndex(6);
            break;
        case QTextListFormat::ListLowerAlpha:
            comboStyle->setCurrentIndex(7);
            break;
        case QTextListFormat::ListUpperAlpha:
            comboStyle->setCurrentIndex(8);
            break;
        case QTextListFormat::ListLowerRoman:
            comboStyle->setCurrentIndex(9);
            break;
        case QTextListFormat::ListUpperRoman:
            comboStyle->setCurrentIndex(10);
            break;
        default:
            comboStyle->setCurrentIndex(-1);
            break;
        }

        switch (ted->textCursor().block().blockFormat().marker())
        {
        case QTextBlockFormat::MarkerType::NoMarker:
            actionToggleCheckState->setChecked(false);
            break;
        case QTextBlockFormat::MarkerType::Unchecked:
            comboStyle->setCurrentIndex(4);
            actionToggleCheckState->setChecked(false);
            break;
        case QTextBlockFormat::MarkerType::Checked:
            comboStyle->setCurrentIndex(5);
            actionToggleCheckState->setChecked(true);
            break;
        }
    }
    else
    {
        int headingLevel = ted->textCursor().blockFormat().headingLevel();
        comboStyle->setCurrentIndex(headingLevel ? headingLevel + 10 : 0);
    }
}


void TextEdit::clipboardDataChanged()
{
#ifndef QT_NO_CLIPBOARD
    if (const QMimeData *md = QApplication::clipboard()->mimeData())
        actionPaste->setEnabled(md->hasText());
#endif
}


void TextEdit::mergeFormatOnWordOrSelection(const QTextCharFormat &format)
{
    CTextEdit* ted = currText();
    QTextCursor cur = ted->textCursor();

    if (!cur.hasSelection())
        cur.select(QTextCursor::WordUnderCursor);

    cur.mergeCharFormat(format);
}


void TextEdit::fontChanged(const QFont &f)
{
    CTextEdit* ted = currText();
    if (ted->document()->isEmpty())
    {
        comboFont->setCurrentFont(textFont);
        comboSize->setCurrentIndex(stFontSizes.indexOf(textFont.pointSize()));
        return;
    }

    currFont = f;

    comboFont->setCurrentFont(f);
    comboSize->setCurrentIndex(stFontSizes.indexOf(f.pointSize()));

    actionTextBold->setChecked(f.bold());
    actionTextItalic->setChecked(f.italic());
    actionTextUnderline->setChecked(f.underline());
}


void TextEdit::onFgColor(QColor color)
{
    if (wtTabs->count() == 0)  return;
    if (!color.isValid())  return;

    currFgColorName = color.name();

    QTextCharFormat fmt;
    fmt.setForeground(color);
    mergeFormatOnWordOrSelection(fmt);
}


void TextEdit::onBgColor(QColor color)
{
    if (wtTabs->count() == 0)  return;
    if (!color.isValid())  return;

    if (QApplication::keyboardModifiers() & Qt::ShiftModifier)
    {
        QTextBlockFormat blockFormat = currText()->textCursor().blockFormat();
        blockFormat.setBackground(color);
        currText()->textCursor().setBlockFormat(blockFormat);
        return;
    }

    currBgColorName = color.name();

    QTextCharFormat fmt;
    fmt.setBackground(color);
    mergeFormatOnWordOrSelection(fmt);
}


/*
h1 - font-size: 2em - font-weight: bolder - 32 px
h2 - font-size: 1.5em - font-weight: bolder - 24 px
h3 - font-size: 1.17em - font-weight: bolder - 18.72 px
h4 - font-size: 1em - font-weight: bolder - 16 px - The height of 1 em is usually 16 pixels.
h5 - font-size: .83em - font-weight: bolder - 13.28 px
h6 - font-size: .67em - font-weight: bolder - 10.72 px
*/
void TextEdit::changeTextSize(int value)
{
    CTextEdit* ted = currText();
    QTextCursor cursor = ted->textCursor();
    QTextCharFormat fmt = cursor.charFormat();

    if (fmt.isImageFormat())
    {
        int step = 20;
        if (QApplication::keyboardModifiers() & Qt::CTRL)  step = 1;

        QTextImageFormat tif = fmt.toImageFormat();
        QSize imageSize = ted->getImageSize(QFileInfo(tif.name()).fileName());
        qreal w = tif.width();
        qreal h = tif.height();
        qreal ratio;
        int diff;

        // original size
        if (QApplication::keyboardModifiers() & Qt::ShiftModifier)
        {
            w = imageSize.width();
            h = imageSize.height();
        }
        else
        {
            if (w <= 0 && h <= 0)
            {
                w = imageSize.width();
                h = imageSize.height();
            }

            if (w > 0 && h > 0)
                ratio = w/h;

            if (w > 0)
            {
                diff = abs(imageSize.width() - w);
                if (diff && diff < step)
                    w += (value < 0 ? -1 : 1) * diff;
                else
                    w += value * step;
                if (h > 0)  h = w/ratio;
            }
            else if (h > 0)
            {
                diff = abs(imageSize.height() - h);
                if (diff && diff < step)
                    h += (value < 0 ? -1 : 1) * diff;
                else
                    h += value * step;
                if (w > 0)  w = h * ratio;
            }
            else
            {
                w = (ted->width() - 30)/2;
                h = 0;
            }
        }

        if (w > 0)  tif.setWidth(w);
        if (h > 0)  tif.setHeight(h);
        if (tif.isValid())
        {
            cursor.setCharFormat(tif);
            statusBarMessage(tr("%1x%2 [%3x%4]").arg(QString::number(w), QString::number(h),
                                    QString::number(imageSize.width()), QString::number(imageSize.height())));
        }
    }
    else if (QApplication::keyboardModifiers() & Qt::ShiftModifier)
    {
        if (value > 0) ted->zoomIn();
        if (value < 0) ted->zoomOut();
    }
    else if (QApplication::keyboardModifiers() & Qt::CTRL)
    {
        QTextCursor cur = ted->textCursor();
        QTextTable* tab = cur.currentTable();
        if (!tab)  return;

        if (tab->format().columnWidthConstraints().first().type() == QTextLength::PercentageLength)
        {
            QTextTableFormat fmt = tab->format();
            QVector<QTextLength> constraints(fmt.columnWidthConstraints());
            int maxColWidth = 100;

            for (int i = 0; i < tab->columns(); i++)
                maxColWidth -= constraints.at(i).rawValue();
            if (maxColWidth < 0)  maxColWidth = 0;

            int col = tab->cellAt(cur).column();
            int colWidth = constraints.at(col).rawValue();
            maxColWidth += colWidth;
            int w = 0;

            if (value > 0 && colWidth < maxColWidth)  w = colWidth + 1;
            if (value < 0 && colWidth > 1)  w = colWidth - 1;

            if (w)
            {
                QTextLength len(QTextLength::PercentageLength, w);
                constraints.replace(col, len);
                fmt.setColumnWidthConstraints(constraints);
                if (fmt.isValid())
                {
                    tab->setFormat(fmt);
                    statusBarMessage(tr("cell width: %1%").arg(QString::number(w)));
                }
            }
        }
    }
    else
    {
        int headingLevel = cursor.blockFormat().headingLevel();

        if (headingLevel == 0)
        {
            qreal pointSize = currFont.pointSizeF();
            if (pointSize > 0)
            {
                int i = stFontSizes.indexOf(pointSize);
                if (i < 0)  i = stFontSizes.indexOf(11);
                i += value;
                if (i >= 0 && i < stFontSizes.count() - 1)
                {
                    pointSize = stFontSizes[i];
                    QTextCharFormat fmt;
                    fmt.setFontPointSize(pointSize);
                    mergeFormatOnWordOrSelection(fmt);
                    statusBarMessage(tr("%1 pt").arg(QString::number(pointSize)));
                }
            }
        }
        else
        {
            headingLevel += -value;
            qDebug() << "new headingLevel=" << headingLevel;
            if (headingLevel > 0 && headingLevel <= 6)
            {
                textStyle(headingLevel ? headingLevel + 10 : 0);
                cursorPositionChanged();
                statusBarMessage(tr("H%1").arg(QString::number(headingLevel)));
            }
        }
    }
}


void TextEdit::onCopy()
{
    CTextEdit* ted = currText();
    ted->copy();
}


void TextEdit::onPaste()
{
    CTextEdit* ted = currText();
    ted->paste();
}


void TextEdit::onCut()
{
    CTextEdit* ted = currText();
    ted->cut();
}


void TextEdit::onUndo()
{
    CTextEdit* ted = currText();

    ted->undo();

    if (QApplication::keyboardModifiers() & Qt::CTRL)
        while (actionUndo->isEnabled())
            ted->undo();

    cursorPositionChanged();
    currentCharFormatChanged(ted->currentCharFormat());
}


void TextEdit::onRedo()
{
    CTextEdit* ted = currText();
    qDebug() << "onRedo:  " << ted->document()->isRedoAvailable();

    ted->redo();

    if (QApplication::keyboardModifiers() & Qt::CTRL)
        while (actionRedo->isEnabled())
            ted->redo();
}


void TextEdit::onTextLarger()
{
    changeTextSize(1);
}


void TextEdit::onTextSmaller()
{
    changeTextSize(-1);
}


void TextEdit::onSelectionChanged()
{
   qDebug() << "onSelectionChanged";
}


void TextEdit::onTextDoubleClick()
{
    CTextEdit* ted = currText();
    QTextCursor cursor = ted->textCursor();
    QTextCharFormat fmt = cursor.charFormat();

    if (fmt.isImageFormat())
    {
        QTextImageFormat tif = fmt.toImageFormat();
        QString fileName = QFileInfo(tif.name()).fileName();
        QSize imageSize = ted->getImageSize(fileName);

        statusBarMessage(tr("%1: %2x%3 [%4x%5]").arg(fileName, QString::number(tif.width()), QString::number(tif.height()),
                                     QString::number(imageSize.width()),  QString::number(imageSize.height())));
    }
}


void TextEdit::fileAttach()
{
    if (!wtTabs->count())  return;
    CTextEdit* ted = currText();

    QFileDialog dlg(this, tr("Insert file"));
    if (!isUseNativeFileDlg)
        dlg.setOption(QFileDialog::DontUseNativeDialog);
    dlg.setFileMode(QFileDialog::ExistingFile);
    //dlg.setFilter(QDir::Hidden | QDir::NoDotAndDotDot);   // was empty
    dlg.setViewMode(QFileDialog::Detail);
    dlg.setAcceptMode(QFileDialog::AcceptOpen);
    dlg.resize(fileDialogSize);
    dlg.selectNameFilter(dlgCurrFilter[3]);

    if (dlg.exec() != QDialog::Accepted)
        return;

    QString filePath = dlg.selectedFiles().first();
    fileDialogSize = dlg.size();
    dlgCurrFilter[3] = dlg.selectedNameFilter();

    ted->insertFile(filePath);
}


int TextEdit::createFilesDir(QString path)
{
    QDir dir(path);
    if (!dir.exists())
        if (!dir.mkpath("."))
            {
                QMessageBox::critical(this, appName, "Create directory error.\n" + tempDir, QMessageBox::Ok);
                return 0;
            }
    return 1;
}


void TextEdit::onCursorOnLink(QString href)
{
    statusBarMessage(href.replace("%20", " "));
}


bool TextEdit::isHttpLink(QString link)
{
    return link.indexOf("http") == 0;
}


void TextEdit::onLinkClicked(QString link)
{
    QUrl url(link);

    if (!isHttpLink(link))
    {
        QString fileName = link.replace("%20", " ");
        CTextEdit* ted = currText();
        fileName = ted->getFilesDir() + "/" + QFileInfo(fileName).fileName();

        QFile file(fileName);
        QString err;

        if (!file.exists())
            err = "File not exists [3]\n" + fileName;
        else if(!file.open(QIODevice::ReadOnly))
            err = "File open error\n" + fileName;

        if (!err.isEmpty())
        {
            QMessageBox::warning(this, appName, err, QMessageBox::Ok);
            return;
        }

        url.setUrl("file://" + fileName);
    }

    QDesktopServices::openUrl(url);
}


QString TextEdit::shrinkHtml(QString html)
{
    QString s = html;
    s.remove(" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;");
    s.remove(" style=\"\"");
    s.replace("</style>", "p, li, h1, h2, h3, h4, h5, h6 { \n"
                             "margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px; \n"
                             "} \n</style>");
    //if (s.indexOf("charset=UTF-8", Qt::CaseInsensitive) < 0)
        //s.replace("<style", "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\" /><style");

    return s;
}


bool TextEdit::writeShrinkHtml(QString path, QString html)
{
    QFile file(path);
    if (file.open(QIODevice::WriteOnly|QIODevice::Text))
    {
        file.write(shrinkHtml(html).toUtf8());
        file.close();
        return true;
    }

    return false;
}


int TextEdit::saveToHtml(QString newFileName, QString format)
{
    CTextEdit* ted = currText();

    QString filesDir  = QFileInfo(newFileName).path() + "/" + QFileInfo(newFileName).baseName() + "_files";

    if (!createFilesDir(filesDir))  return false;

    QList<QString> fileList = ted->getResourceFileList();
    QString err;

    QString path = ted->getFilesDir() + "/";
    QDirIterator it(path, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);

    while (it.hasNext())
    {
        QString filePath = it.next();
        QString fileName = QFileInfo(filePath).fileName();

        if (it.fileInfo().isFile())
        {
            QFile file(filePath);
            QString destFileName = filesDir + "/" + fileName;

            if (fileList.contains(fileName))
            {
                if (!file.open(QIODevice::ReadOnly))
                {
                    err = "File open error.\n" + filePath;
                    break;
                }

                if (QFileInfo(destFileName).exists())  continue;

                if (!file.copy(destFileName))
                {
                    err = "File copy error.\n" + filePath + "\nto " + destFileName;
                    break;
                }
            }
            else
            {
                QFile destFile(destFileName);
                if (destFile.exists())
                    if (!destFile.remove())
                    {
                        err = "File delete error.\n" + destFileName;
                        break;
                    }
            }
        }
    }

    if (!err.isEmpty())
    {
        QMessageBox::warning(this, appName, err, QMessageBox::Ok);
        return 0;
    }

    if (filesDir != ted->getFilesDir())
        ted->setResourceDir(QFileInfo(filesDir).fileName());

    QTextDocumentWriter writer(newFileName);
    bool ok = false;
    if (format == "md")
    {
        writer.setFormat("markdown");
        ok = writer.write(ted->document());
    }
    else
    {
        if (isShrinkHtml)
            ok = writeShrinkHtml(newFileName, ted->document()->toHtml("utf-8"));
        else
        {
            writer.setFormat("HTML");
            ok = writer.write(ted->document());
        }
    }

    if (!ok)
    {
        QMessageBox::warning(this, appName, "Save document error.", QMessageBox::Ok);
        return 0;
    }

    // working file staying temp
    // html file only in editor?
    return 1;
}


QString TextEdit::genTempDirName()
{
    int t = time(NULL);
    static int n = 0;  n++;
    return QString::number(t) + QString::number(t/2).right(5) + QString::number(n);
}


int TextEdit::saveToMaff(QString newFileName)
{
    CTextEdit* ted = currText();

    QString tempFileDir = QFileInfo(ted->getFileName()).path();
    QString tempFileName = ted->getFileName();  //it is more reliable

    QString zipFileName = newFileName;
    if (zipFileName.isEmpty())
    {
        QMessageBox::warning(this, appName, "No name for save file.", QMessageBox::Ok);
        return 0;
    }

    QFile zf(zipFileName);
    if (zf.exists())
    {
        zipFileName += ".tmp";
    }

    QZipWriter zip(zipFileName);
    if (zip.status() != QZipWriter::NoError)
        return 0;

    ted->setResourceDir("index_files");

    //save editor text to temp dir
    if (isShrinkHtml)
        writeShrinkHtml(tempFileName, ted->document()->toHtml("utf-8"));
    else
    {
        QTextDocumentWriter writer(tempFileName);
        writer.setFormat("html");
        writer.write(ted->document());
    }

    QList<QString> fileList = ted->getResourceFileList();

    zip.setCompressionPolicy(QZipWriter::AutoCompress);

    //QString path = QLatin1String("/path/to/dir/"); // trailing '/' is very important
    QString path = tempFileDir + "/";
    QDirIterator it(path, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    QFile::Permissions dirPerm = QFile::ReadOwner|QFile::WriteOwner|QFile::ExeOwner |
            QFile::ReadGroup|QFile::WriteGroup|QFile::ExeGroup |
            QFile::ReadOther|QFile::ExeOther;
    QString err;

    QString zipDir = genTempDirName();
    zip.setCreationPermissions(dirPerm);
    zip.addDirectory(zipDir);

    while (it.hasNext())
    {
        QString filePath = it.next();
        QString fileName = QFileInfo(filePath).fileName();

        if (it.fileInfo().isDir())
        {
            zip.setCreationPermissions(dirPerm);
            zip.addDirectory(zipDir +"/" + filePath.remove(path));
        }
        else if (it.fileInfo().isFile())
        {
            if (fileName != "index.html")
                if (!fileList.contains(fileName))  continue;

            QFile file(filePath);

            if (!file.open(QIODevice::ReadOnly))
            {
                err = "File open error.\n" + filePath;
                break;
            }

            zip.setCreationPermissions(QFile::permissions(filePath));
            QByteArray ba = file.readAll();
            zip.addFile(zipDir +"/" + filePath.remove(path), ba);
        }
    }

    zip.close();

    if (!err.isEmpty())
    {
        QMessageBox::warning(this, appName, err, QMessageBox::Ok);
        return 0;
    }

    if (zipFileName.indexOf(".tmp") > 0)
    {
        zf.remove();
        QFile zfTemp(zipFileName);
        zfTemp.rename(zipFileName.remove(".tmp"));
    }

    // temp files not changed
    return 1;
}


int TextEdit::saveToEpub(QString newFileName)
{
    CTextEdit* ted = currText();

    QString tempFileDir = QFileInfo(ted->getFileName()).path();
    //QString tempFileName = tempFileDir + "/index.html";  //it is more reliable
    QString tempFileName = ted->getFileName();  //it is more reliable

    QString zipFileName = newFileName;
    if (zipFileName.isEmpty())
    {
        QMessageBox::warning(this, appName, "No name for save file.", QMessageBox::Ok);
        return 0;
    }

    QFile zf(zipFileName);
    if (zf.exists())
    {
        zipFileName += ".tmp";
    }

    QZipWriter zip(zipFileName);
    if (zip.status() != QZipWriter::NoError)
        return 0;

    QTextDocument doc(ted->document());
    doc.setHtml(ted->document()->toHtml("utf-8"));

    ted->setResourceDir("index_files", &doc);

    QList<QString> fileList = ted->getResourceFileList(&doc);

    zip.setCompressionPolicy(QZipWriter::AutoCompress);

    QString path = tempFileDir + "/";
    QDirIterator it(path, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    QFile::Permissions filePerm = QFile::ReadOwner|QFile::WriteOwner |
            QFile::ReadGroup |
            QFile::ReadOther;
    QFile::Permissions dirPerm = filePerm;
    QString err;
    QString s;

    s = "application/epub+zip";
    zip.setCreationPermissions(filePerm);
    zip.addFile("mimetype", s.toUtf8());

    QString zipDir = "META-INF";
    zip.setCreationPermissions(dirPerm);
    zip.addDirectory(zipDir);

    s = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<container xmlns=\"urn:oasis:names:tc:opendocument:xmlns:container\" version=\"1.0\">\n"
        "<rootfiles>\n"
            "<rootfile full-path=\"OPS/content.opf\" media-type=\"application/oebps-package+xml\"/>\n"
        "</rootfiles>\n"
    "</container>  \n";

    zip.setCreationPermissions(filePerm);
    zip.addFile(zipDir + "/container.xml", s.toUtf8());

    zipDir = "OPS";
    zip.setCreationPermissions(dirPerm);
    zip.addDirectory(zipDir);

    while (it.hasNext())
    {
        QString filePath = it.next();
        QString fileName = QFileInfo(filePath).fileName();

        if (it.fileInfo().isDir())
        {
            zip.setCreationPermissions(dirPerm);
            zip.addDirectory(zipDir + "/" + filePath.remove(path));
        }
        else if (it.fileInfo().isFile())
        {
            if (!fileList.contains(fileName))  continue;

            QFile file(filePath);

            if (!file.open(QIODevice::ReadOnly))
            {
                err = "File open error.\n" + filePath;
                break;
            }

            zip.setCreationPermissions(QFile::permissions(filePath));
            QByteArray ba = file.readAll();
            zip.addFile(zipDir + "/" + filePath.remove(path), ba);
        }
    }

    QString docTitle = cutStr(QFileInfo(newFileName).baseName(), tabTitleLen);

    s = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<package version=\"2.0\" xmlns=\"http://www.idpf.org/2007/opf\" unique-identifier=\"BookId\">\n"

        "<metadata xmlns:dc=\"http://purl.org/dc/elements/1.1/\" xmlns:opf=\"http://www.idpf.org/2007/opf\">\n"
        "<meta content=\"cover\" name=\"cover\"/>\n"
        "<dc:title>" + docTitle + "</dc:title>\n"
        "<dc:creator>unknown</dc:creator>\n"
        "<dc:subject>unknown</dc:subject>\n"
        "<dc:identifier id=\"unknown\">unknown</dc:identifier>\n"
        "<dc:publisher>unknown</dc:publisher>\n"
        "<dc:language>en</dc:language>\n"
        "</metadata>\n"

        "<manifest>\n"
        "<item id=\"ncx\" href=\"toc.ncx\" media-type=\"application/x-dtbncx+xml\"/>\n"
        "<item id=\"item-1\" href=\"index.xhtml\" media-type=\"application/xhtml+xml\"/>\n"
        "</manifest>\n"

        "<spine toc=\"ncx\">\n"
        "<itemref idref=\"item-1\" linear=\"yes\"/>\n"
        "</spine>\n"

        "</package>\n";

    zip.setCreationPermissions(filePerm);
    zip.addFile(zipDir + "/content.opf", s.toUtf8());

    s = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<!DOCTYPE ncx PUBLIC \"-//NISO//DTD ncx 2005-1//EN\" \"http://www.daisy.org/z3986/2005/ncx-2005-1.dtd\">\n"

    "<ncx version=\"2005-1\" xml:lang=\"bg\" xmlns=\"http://www.daisy.org/z3986/2005/ncx/\">\n"

        "<head>\n"
            "<meta name=\"dtb:uid\" content=\"unknown\"/>\n"
            "<meta name=\"dtb:depth\" content=\"3\"/>\n"
            "<meta name=\"dtb:totalPageCount\" content=\"0\"/>\n"
            "<meta name=\"dtb:maxPageNumber\" content=\"0\"/>\n"
            "<meta name=\"epub-creator\" content=\"mdqtee\"/>\n"
        "</head>\n"

        "<docTitle><text>" + docTitle + "</text></docTitle>\n"
        "<docAuthor><text>unknown</text></docAuthor>\n"

        "<navMap>\n"
            "<navPoint class=\"chapter\" id=\"navpoint-1\" playOrder=\"1\"><navLabel><text>Document</text></navLabel><content src=\"index.xhtml\"/></navPoint>\n"
        "</navMap>\n"

    "</ncx>    \n";

    zip.setCreationPermissions(filePerm);
    zip.addFile(zipDir + "/toc.ncx", s.toUtf8());

    s = shrinkHtml(doc.toHtml("utf-8"));
    QString xhtml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.1//EN\" \"http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd\">\n"
        "<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"bg\">\n"
        "<head>\n"
        "<meta http-equiv=\"Content-Type\" content=\"application/xhtml+xml; charset=utf-8\"/>\n";
    if (lineWrapWidth)
        //"<meta name=\"viewport\" content=\"width=1000, height=1000\"></meta>\n"
        xhtml.append("<meta name=\"viewport\" content=\"width=" + QString::number(lineWrapWidth + 30) + "\"></meta>\n");

    xhtml.append(s.mid(s.indexOf("<style")));

    zip.setCreationPermissions(filePerm);
    zip.addFile(zipDir + "/index.xhtml", xhtml.toUtf8());

    zip.close();

    if (!err.isEmpty())
    {
        QMessageBox::warning(this, appName, err, QMessageBox::Ok);
        return 0;
    }

    if (zipFileName.indexOf(".tmp") > 0)
    {
        zf.remove();
        QFile zfTemp(zipFileName);
        zfTemp.rename(zipFileName.remove(".tmp"));
    }

    // temp files not changed
    return 1;
}


int TextEdit::unpackMaff(QString zipName)
{
    CTextEdit* ted = currText();

    QString dataDir;
    QDir dir(tempDir);

    QZipReader unzip(zipName, QIODevice::ReadOnly);
    QList<QZipReader::FileInfo> allFiles = unzip.fileInfoList().toList();
    QZipReader::FileInfo fi;

    foreach (QZipReader::FileInfo fi, allFiles)
    {
        const QString absPath = tempDir + QDir::separator() + fi.filePath;
        if (fi.isDir)
        {
            if (!dir.mkpath(fi.filePath))
                 return 0;
            //if (!QFile::setPermissions(absPath, fi.permissions))
            if (!QFile::setPermissions(absPath, QFile::ReadOwner|QFile::WriteOwner|QFile::ExeOwner |
                                           QFile::ReadGroup|QFile::WriteGroup|QFile::ExeGroup |
                                           QFile::ReadOther|QFile::ExeOther))  //++
                return 0;

            if (dataDir.isEmpty())
                dataDir = tempDir + "/" + fi.filePath;
        }
    }

    foreach (QZipReader::FileInfo fi, allFiles)
    {
        const QString absPath = tempDir + QDir::separator() + fi.filePath;
        if (fi.isFile)
        {
            QFile file(absPath);
            if( file.open(QFile::WriteOnly) )
            {
                file.write(unzip.fileData(fi.filePath), unzip.fileData(fi.filePath).size());
                file.setPermissions(fi.permissions);
                file.close();
            }
        }
    }

    unzip.close();

    ted->setFileName(dataDir + "/index.html");
    ted->setFilesDir(dataDir + "index_files");
    ted->tempPath = dataDir;

    return 1;
}


int TextEdit::copyFile(QString fromName, QString toName)
{
    QString err;

    QFile file(fromName);

    if (!file.open(QIODevice::ReadOnly))
    {
        err = "File open error.\n" + fromName;
    }
    else if (!file.copy(toName))
    {
        err = "File copy error.\n" + fromName + "\nto " + toName;
    }

    if (!err.isEmpty())
    {
        QMessageBox::warning(this, appName, err, QMessageBox::Ok);
        return 0;
    }

    return 1;
}


int TextEdit::openTempDir(QString origFileName)
{
    CTextEdit* ted = currText();

    QString tempDirName = tempDir + "/" + genTempDirName();

    if (!createFilesDir(tempDirName))  return 0;

    //QString tempFilesDir  = tempDirName + "/" + QFileInfo(origFileName).baseName() + "_files";
    QString tempFileName  = tempDirName + "/index.html";
    QString tempFilesDir  = tempDirName + "/index_files";

    if (!createFilesDir(tempFilesDir))  return false;

    QString filesDir  = QFileInfo(origFileName).path() + "/" + QFileInfo(origFileName).baseName() + "_files";

    QString path = filesDir + "/";
    QDirIterator it(path, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);

    while (it.hasNext())
    {
        QString filePath = it.next();

        if (it.fileInfo().isFile())
            if (!copyFile(filePath, tempFilesDir + "/" + QFileInfo(filePath).fileName()))  break;
    }

    if (!copyFile(origFileName, tempFileName))
        return 0;

    ted->setFileName(tempFileName);
    ted->setFilesDir(tempFilesDir);
    ted->tempPath = tempDirName;

    return 1;
}


void TextEdit::onHttpFinished(QNetworkReply *resp)
{
    static int maxHttpReqCount = 0;
    static int httpErrCount = 0;
    static int httpOkCount = 0;
    if (!maxHttpReqCount) maxHttpReqCount = httpReqCount;
    CTextEdit* ted = currText();
    //CTextEdit* ted = resp->property("ted").value<CTextEdit*>();

    if(resp->error() != QNetworkReply::NoError)
    {
       printf("onHttpFinished:  Error: %s\n", resp->errorString().toUtf8().data());
       httpErrCount++;
    }
    else
    {
        if (ted)
        {
            QString fileName = ted->getFilesDir() + "/" + QFileInfo(resp->url().toString()).fileName();
            QByteArray respData = resp->readAll();
            QFile file(fileName);
            file.open(QIODevice::WriteOnly);
            file.write((respData));
            file.close();

            QString key = QFileInfo(ted->tempPath).fileName() + "=" + resp->request().url().toString();
            QString path = "index_files/" + QFileInfo(resp->url().toString()).fileName();
            resCache[key] = path;
            httpOkCount++;
            statusBarMessage(tr("loaded: %1 files").arg(QString::number(httpOkCount)));
        }
    }

    resp->deleteLater();

    if (--httpReqCount == 0)
    {
        maxHttpReqCount = 0;
        httpErrCount = 0;
        httpOkCount = 0;
        setOtherTabEnabled(wtTabs->currentIndex(), true);
        QGuiApplication::restoreOverrideCursor();
        isHttpRequest = false;
        qDebug() << "onHttpFinished:  httpOkCount=" << httpOkCount;

        if (ted)
        {
            qDebug() << "onHttpFinished:  httpErrCount=" << httpErrCount;
            if (httpOkCount == 0)
                ted->setNetworkEnabled(false);

            ted->refresh();
        }
    }
}


void TextEdit::onAddResFile(QTextImageFormat &fmt)
{
    QString filePath = fmt.name().remove("file://");
    fmt.setName("");
    QString flags = fmt.property(1).toString();

    if (!wtTabs->count())  return;

    //CTextEdit* ted = currText();
    CTextEdit* ted = qobject_cast<CTextEdit*>(sender());

    QString filesDir = ted->getFilesDir();
    if (!createFilesDir(filesDir))  return;

    QString err;

    // if from clipboard
    if (flags.indexOf("C") >= 0)
    {
        fmt.setHeight(0);  // for thumbnails height is better

        const QClipboard *clip = QApplication::clipboard();
        const QMimeData *mimeData = clip->mimeData();
        QString fileName;

        if (mimeData->hasImage())
        {
            QPixmap pix = qvariant_cast<QPixmap>(mimeData->imageData());
            qDebug() << "onAddResFile:  pix:  " << pix.isNull() << pix.isQBitmap() << pix.isDetached(); // false false true
            QPixmap pix2 = pix.scaled(thumbHeight * 3, thumbHeight, Qt::KeepAspectRatio,Qt::SmoothTransformation);

            // if thumbnail
            if (flags.indexOf("T") >= 0 && !fmt.anchorHref().isEmpty())
            {
                QString mime = QMimeDatabase().mimeTypeForFile(fmt.anchorHref()).name();
                if (mime.indexOf("video") >= 0 || QFileInfo(fmt.anchorHref()).suffix() == "ts" || mime.indexOf("audio") >= 0)
                {
                    QPixmap pixPlay(":/images/linux/play.png");
                    QPainter p(&pix2);
                    p.drawPixmap(0, pix2.height() - pixPlay.height(), pixPlay);
                    p.end();
                }
            }

            fileName = filesDir + "/" + QString("image_%1.png").arg(time(NULL));
            //fmt.setWidth(pix2.width());
            fmt.setHeight(pix2.height());

            if (!pix2.save(fileName, 0, -1))
            {
                QMessageBox::warning(this, appName, "Save clipboard image error.\n" + fileName, QMessageBox::Ok);
            }
            else
            {
                fmt.setName(QFileInfo(filesDir).fileName() + "/" + QFileInfo(fileName).fileName());
                statusBarMessage(tr("saved: %1").arg(fileName));
            }
        }

        return;
    }

    // from HTTP
    if (flags.indexOf("H") >= 0)
    {
        QString key = QFileInfo(ted->tempPath).fileName() + "=" + filePath;

        if (resCache.contains(key))
            fmt.setName(resCache[key]);
        else
        {
            if (!nm)
            {
                nm = new QNetworkAccessManager;
                // https://doc.qt.io/qt-5/qnetworkrequest.html#RedirectPolicy-enum
                nm->setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);  //++
                //connect(nm, &QNetworkAccessManager::finished, this, &TextEdit::onHttpFinished);
            }

            QNetworkRequest req;
            req.setUrl(QUrl(filePath));
            req.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 6.3; Win64; x64; rv:73.0) Gecko/20100101 Firefox/73.0");
            QNetworkReply *resp = nm->get(req);

            CWaitObj wo;
            connect(resp, &QNetworkReply::finished, &wo, &CWaitObj::exit);

            CWaitCursor* cur = new CWaitCursor;
            cur->deleteLater();

            statusBarMessage(tr("loading ..."));
            wo.wait();

            if (resp->error() != QNetworkReply::NoError )
            {
                ted->setNetworkEnabled(false);
                printf("Network request  error: %s\n", resp->errorString().toUtf8().data());
                statusBarMessage(tr("Error: ") + resp->errorString());
            }
            else if (wo.errCode != 0)
            {
                ted->setNetworkEnabled(false);
                printf("Network request  error: timeout [15 sec]\n");
                statusBarMessage(tr("Error: timeout [15 sec]"));
            }
            else
            {
                qDebug() << "    loaded=" << resp->url();
                QString fileName = filesDir + "/" + QFileInfo(resp->url().toString()).fileName();
                QByteArray respData = resp->readAll();
                QFile file(fileName);
                if (file.exists())  file.remove();
                file.open(QIODevice::WriteOnly);
                file.write((respData));
                file.close();

                fmt.setName(QFileInfo(filesDir).fileName() + "/" + QFileInfo(fileName).fileName());
                statusBarMessage(tr(""));
            }

            resp->deleteLater();
        }

        return;
    }

    //if (flags.indexOf("F") >= 0)
    QString fileName = QFileInfo(filePath).fileName();

    QFile file(filePath);
    QString tempFileName;

    if (!file.exists())
        err = "File not exists [1]";
    else
    {
        if (file.size() > 512 * 1024 * 1024)
            err = "File is too large. File cannot be added. [> 512 Mb]";
        else if(!file.open(QIODevice::ReadOnly))
            err = "File open error";
        else
        {
            tempFileName = filesDir + "/" + fileName;
            int n = 0;

            while (n++ < 100)
            {
                QFile tempFile(tempFileName);
                if (!tempFile.exists())  break;
                tempFile.close();
                tempFileName = filesDir + "/" + QFileInfo(fileName).baseName()
                        + " (" + QString::number(n) + ")." + QFileInfo(fileName).suffix();
            }

            if (n >= 100)
                err = "File copy error. Too many files with the same name.";
            else
            {
                CWaitCursor cur;
                if(!file.copy(tempFileName))
                    err = "File copy error";
            }
        }
    }

    if (!err.isEmpty())
    {
        QMessageBox::warning(this, appName, err + "\n" + filePath, QMessageBox::Ok);
        return;
    }

    if (!tempFileName.isEmpty())
    {
        statusBarMessage(tr("file added: %1").arg(tempFileName));
        tempFileName =  QFileInfo(filesDir).fileName() + "/" + QFileInfo(tempFileName).fileName();
        fmt.setName(tempFileName);
    }

    return;
}


void TextEdit::onTedError(QString text)
{
    QMessageBox::warning(this, appName, text, QMessageBox::Ok);
}


void TextEdit::onSaveResAsFile(QString name)
{
    CTextEdit* ted = currText();
    QString filesDir = ted->getFilesDir();
    QString resFileName = QFileInfo(name).fileName().replace("%20", " ");
    QString resFilePath = filesDir + "/" + resFileName;

    QFile resFile(resFilePath);
    if (!resFile.exists())
    {
        QMessageBox::warning(this, appName, "File not found.\n" + resFilePath, QMessageBox::Ok);
        return;
    }

    QFileDialog dlg(this, tr("Save as"));
    if (!isUseNativeFileDlg)
        dlg.setOption(QFileDialog::DontUseNativeDialog);
    dlg.setNameFilters(QStringList()
        << "All files (*)"
    );
    dlg.setAcceptMode(QFileDialog::AcceptSave);
    dlg.setViewMode(QFileDialog::Detail);
    dlg.resize(fileDialogSize);
    dlg.selectNameFilter(dlgCurrFilter[4]);
    dlg.selectFile(resFileName);

    if (dlg.exec() != QDialog::Accepted)
        return;

    QString fn = dlg.selectedFiles().first();
    fileDialogSize = dlg.size();
    dlgCurrFilter[4] = dlg.selectedNameFilter();

    if (!resFile.copy(fn))
    {
        QMessageBox::warning(this, appName, "File copy error.\n" + resFilePath + "\nto " + fn, QMessageBox::Ok);
        return;
    }
}


void TextEdit::onTableMenuAct(bool checked)
{
    if (!wtTabs->count())  return;

    QAction* a = qobject_cast<QAction*>(sender());
    QString act = a->data().toString();

    CTextEdit* ted = currText();
    QTextCursor cur = ted->textCursor();

    if (act == "CT")
    {
        QTextTableFormat tf;
        tf.setCellPadding(4);
        tf.setBorderCollapse(true);
        //tf.setMargin(5);
        //tf.setWidth(100);  // not changes when add column
        if (QApplication::keyboardModifiers() & Qt::ShiftModifier)
        {
            QVector<QTextLength> constraints;
            for (int i = 0; i < 2; i++)
                constraints.append(QTextLength(QTextLength::PercentageLength, 100/2));
            tf.setColumnWidthConstraints(constraints);
        }

        QTextTable* tab = cur.insertTable(2, 2, tf);
    }

    if (!cur.currentTable())  return;

    QTextTable* tab = ted->textCursor().currentTable();
    if (!tab)  return;

    if (act == "AR")
    {
        tab->appendRows(1);
    }
    else if (act == "AC")
    {
        tab->appendColumns(1);
        adjustTableFormat(tab);
    }
    else if (act == "IR")
    {
        tab->insertRows(tab->cellAt(cur).row(), 1);
    }
    else if (act == "IC")
    {
        tab->insertColumns(tab->cellAt(cur).column(), 1);
        adjustTableFormat(tab);
    }
    else if (act == "DR")
    {
        if (tab->rows() < 2)  return;
        int row = tab->cellAt(cur).row();
        if (tab->rows() == 2 && tab->columns() < 2 && row == 0)  return;  // otherwise crash
        tab->removeRows(row, 1);
    }
    else if (act == "DC")
    {
        if (tab->columns() < 2)  return;
        tab->removeColumns(tab->cellAt(cur).column(), 1);
        adjustTableFormat(tab);
    }
    else if (act == "MC")
    {
        tab->mergeCells(cur);
        //tab->mergeCells(tab->cellAt(cur).row(), tab->cellAt(cur).column(), 1, 2);
    }
    else if (act == "SC")
    {
        tab->splitCell(tab->cellAt(cur).row(), tab->cellAt(cur).column(), 1, 1);  //--
    }

    if (QString("AR|AC|IR|IC|DR|DC").indexOf(act) >= 0)
        btTable->setDefaultAction(a);
    else
        btTable->setDefaultAction(actTableNoAct);
}


void TextEdit::adjustTableFormat(QTextTable* tab)
{
    if (tab->format().columnWidthConstraints().first().type() == QTextLength::PercentageLength)
    {
        QTextTableFormat fmt = tab->format();
        QVector<QTextLength> constraints;
        for (int i = 0; i < tab->columns(); i++)
            constraints.append(QTextLength(QTextLength::PercentageLength, 100/tab->columns()));
        fmt.setColumnWidthConstraints(constraints);
        if (fmt.isValid())
            tab->setFormat(fmt);
    }
}


void TextEdit::addRecentFile(QString fileName)
{
    if (!fileName.isEmpty())
    {
        recentFiles.push_front(fileName);
        recentFiles.removeDuplicates();
        if (recentFiles.count() > RECENT_LIST_LEN)
            recentFiles.removeLast();
    }

    recentMenu->clear();
    QAction* act;

    foreach (QString name, recentFiles)
        act = recentMenu->addAction(name, this, &TextEdit::onRecentFileMenuAct);

    recentMenu->addSeparator();
    act = recentMenu->addAction("Clear list", this, [=]() {  recentFiles.clear();  addRecentFile("");  });
}


void TextEdit::onRecentFileMenuAct()
{
    QAction* a = qobject_cast<QAction*>(sender());
    QString fileName = a->text();

    load(fileName);
}


void TextEdit::onDefaultFontSelect()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok, textFont, this, "Default text font", QFontDialog::DontUseNativeDialog);

    if (ok)
    {
        textFont = font;
        actDefFont->setText("Default text font:  " + textFont.family() + ", " + QString::number(textFont.pointSize()));
    }
}


void TextEdit::onOptionsMenuAct()
{
    QAction* a = qobject_cast<QAction*>(sender());
    QString act = a->data().toString();

    if (act == "defFont")
    {
        bool ok;
        QFont font = QFontDialog::getFont(&ok, textFont, this, "Default text font", QFontDialog::DontUseNativeDialog);

        if (ok)
        {
            textFont = font;
            actDefFont->setText("Default text font:  " + textFont.family() + ", " + QString::number(textFont.pointSize()));
        }
    }
    else if (act == "natDlg")
    {
        isUseNativeFileDlg = !isUseNativeFileDlg;
        actUseNatDlg->setChecked(isUseNativeFileDlg);
    }
    else if (act == "netEnbl")
    {
        isNetworkEnabled = !isNetworkEnabled;
        actNetEnbl->setChecked(isNetworkEnabled);
        for (int i = 0; i < wtTabs->count(); i++)
            ((CTextEdit*)wtTabs->widget(i))->setNetworkEnabled(isNetworkEnabled);
    }
}


void TextEdit::onFindTextPanel()
{
    if (!findTextPanel->isVisible())
    {
        findTextPanel->setVisible(true);
        findTextPanel->setFocus();
    }
    else
    {
        findTextPanel->setVisible(false);
        if (wtTabs->count() > 0)
            currText()->setFocus();
    }
}


void TextEdit::onFindText()
{
    QShortcut* shortcut = qobject_cast<QShortcut*>(sender());
    QKeySequence key = shortcut->key();

    if (key == QKeySequence("F3"))
    {
        if (!findTextPanel->isVisible())
        {
            findTextPanel->setVisible(true);
            findTextPanel->setFocus();
        }
        else
            findTextPanel->findNext();
    }
    else if (key == QKeySequence("Shift+F3"))
    {
        if (findTextPanel->isVisible())
            findTextPanel->findPrev();
    }
    else if (key == QKeySequence("Esc"))
    {
        if (findTextPanel->isVisible())
        {
            findTextPanel->setVisible(false);
            if (wtTabs->count() > 0)
                currText()->setFocus();
        }
    }
}


void TextEdit::onTabRefresh()
{
    if (wtTabs->count())
    {
        httpReqCount = 0;
        isHttpRequest = false;
        currText()->setNetworkEnabled(isNetworkEnabled);
        currText()->refresh();
    }
}


void TextEdit::setOtherTabEnabled(int index, bool enable)
{
    for (int i = 0; i < wtTabs->count(); i++)
        if (i != index)
            wtTabs->setTabEnabled(i, enable);
}


void TextEdit::onNextTab()
{
    if (wtTabs->count() < 2)  return;

    int i = wtTabs->currentIndex();
    i = i < wtTabs->count() - 1  ?  i + 1  :  0;
    wtTabs->setCurrentIndex(i);
}


void TextEdit::onTabContextMenu(const QPoint &point)
{
    if (wtTabs->count() < 2)  return;

    QMenu *menu = new QMenu(this);
    QAction* a;

    for (int i = 0; i < wtTabs->count(); i++)
    {
        a = new QAction(wtTabs->tabText(i), this);
        a->setData(i);
        connect(a, &QAction::triggered, this, [=](bool checked ) {
            wtTabs->setCurrentIndex(qobject_cast<QAction*>(sender())->data().toInt());
        } );
        if (i == wtTabs->currentIndex())
            a->setEnabled(false);
        menu->addAction(a);
    }

    menu->exec(wtTabs->mapToGlobal(point));
    delete menu;
}


void TextEdit::onOptions()
{
    if (iniFileName.isEmpty())
    {
        printf("TextEdit::onOptions:    iniFileName is empty.\n");
        return;
    }

    QSettings set(iniFileName, QSettings::IniFormat);
    set.beginGroup( "public" );

    for (int i = 0; i < params.size(); i++)
    {
        QVariant value = set.value(params[i].name, params[i].defValue);
        if (params[i].type == CConfigList::TypeNumber)
            params[i].value = value.toInt();
        else if (params[i].type == CConfigList::TypeFloat)
            params[i].value = value.toFloat();
        else if (params[i].type == CConfigList::TypeBool)
            params[i].value = value.toBool();
        else
            params[i].value = value.toString();
    }

    winConf->setParams(params);
    winConf->setUseNativeFileDlg(isUseNativeFileDlg);

    if (winConf->exec())
    {
        for (int i = 0; i < winConf->params().count(); i++)
        {
            QVariant value = winConf->params()[i].value;

            if (value.isNull())
                set.setValue(winConf->params()[i].name, winConf->params()[i].defValue);
            else if (winConf->params()[i].isEdited)
                set.setValue(winConf->params()[i].name, value);
        }

        setPublicParams(set);
    }

    set.endGroup();
}


void TextEdit::statusBarMessage(const QString& text)
{
    //if (statBar)
    {
        statusBar()->showMessage(text);
    }
}


void TextEdit::setFileBrowserChecked(bool checked)
{
    actFileBrowser->setChecked(checked);
}


void TextEdit::onFileBrowser(bool checked)
{
    emit sigFileBrowserShow(checked);

}


void TextEdit::onFileMoved(QString file, QString path)
{
    for (int i = 0; i < wtTabs->count(); i++)
    {
        CTextEdit* ted = (CTextEdit*)wtTabs->widget(i);
        if (ted->zipFileName == file)
        {
            if (QMessageBox::question(this, appName,
                      QString("File moved:  " + file + "\nto  " + path + "\n\nUpdate path for file in editor?"),
                      QMessageBox::Yes|QMessageBox::No) == QMessageBox::Yes)
            {
                ted->zipFileName = path + "/" + QFileInfo(ted->zipFileName).fileName();
                statusBarMessage("Path changed: " + ted->zipFileName);
            }
            break;
        }
    }
}

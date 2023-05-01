/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the demonstration applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef TEXTEDIT_H
#define TEXTEDIT_H

#include <QtCore>
#include <QApplication>
#include <QMainWindow>
#include <QSplitter>
#include <QTreeView>
#include <QHeaderView>
#include <QFileSystemModel>
#include <QTabWidget>
#include <QMap>
#include <QPointer>
#include <QToolButton>
#include <QShortcut>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include "ctextedit.h"
#include "ccolortoolbutton.h"
#include "cfindtextpanel.h"
#include "ctabwidget.h"
#include "cconfiglist.h"


#define TAB_TITLE_LEN 30
#define RECENT_LIST_LEN 20


QT_BEGIN_NAMESPACE
class QAction;
class QComboBox;
class QFontComboBox;
class QTextEdit;
class QTextCharFormat;
class QMenu;
class QPrinter;
QT_END_NAMESPACE

//const int DefaultFontSize = 11;


class TextEdit : public QMainWindow
{
    Q_OBJECT

public:
    enum LoadMode { NewTab, CurrentTab };

    QString appDir;
    QString tempDir;
    QFont textFont;
    QFont defaultFont;
    //QString iniFileName;
    QMenuBar* mainMenu;
    QStatusBar* statBar;
    QString iniFileName;
    bool isUseNativeFileDlg;
    bool isNetworkEnabled;
    int lineWrapWidth;
    QString fileBrowserDir;
    TextEdit(QWidget *parent, QSettings& set);
    void setPublicParams(QSettings &set);
    QList<QString> getResFileList();
    void setResDir(QString dir);
    int createFilesDir(QString dir);
    void closeTab(int index = -1);
    void onHelp();
    bool isHttpLink(QString link);
    bool load(const QString &f, LoadMode mode = NewTab);
    bool close(QSettings& set);
    void setFileBrowserChecked(bool checked);
    void onFileMoved(QString file, QString path);
    int copyDir(QString fromName, QString toName);

public slots:
    void fileNew();
    void currentCharFormatChanged(const QTextCharFormat &format);
    void cursorPositionChanged();
    void onTextChanged(bool m);
    void onUndoAvailable(bool m);
    void onRedoAvailable(bool m);

signals:
    //void fileLoaded();
    void httpAllFinished();
    void sigFileBrowserShow(bool show);

protected:

private:
    const QSize DefFileDialogSize = QSize(760, 600);
    const QSize DefWinConfSize = QSize(700, 300);
    const int DefThumbHeigh = 100;
    const int MaxLineWrapWidth = 3840;
    const int MinLineWrapWidth = 100;
    const int DefTabTitleLen = 30;
    const QSize MenuIconSize = QSize(22, 22);
    const int DocumentMargin = 6;
    const int TabStopWidth = 4;

    QWidget *win;
    QTabWidget* wtTabs;
    QToolBar *tb;
    QToolBar *tbFont;
    QByteArray editorLayoutState;
    QAction *actionSave;
    QAction *actionTextBold;
    QAction *actionTextUnderline;
    QAction *actionTextItalic;
    QAction *actionTextLarger;
    QAction *actionTextSmaller;
    QAction *actionAlignLeft;
    QAction *actionAlignCenter;
    QAction *actionAlignRight;
    QAction *actionAlignJustify;
    QAction *actionIndentLess;
    QAction *actionIndentMore;
    QAction *actionToggleCheckState;
    QAction *actionUndo;
    QAction *actionRedo;
#ifndef QT_NO_CLIPBOARD
    QAction *actionCut;
    QAction *actionCopy;
    QAction *actionPaste;
#endif
    QAction *actDefFont;
    QAction *actUseNatDlg;
    QAction *actNetEnbl;
    QAction *actFind;
    QAction *actFileBrowser;
    QToolButton* btOpen;
    QToolButton* btAlign;
    QToolButton* btTable;
    QAction *actTableNoAct;
    CColorToolButton* btFgColor;
    CColorToolButton* btBgColor;
    QComboBox *comboStyle;
    QFontComboBox *comboFont;
    QComboBox *comboSize;
    QFont currFont;
    QShortcut* ctrlS;
    QShortcut* ctrlR;
    QShortcut* ctrlW;
    QShortcut* ctrlTab;
    QList<QColor> fgColors;
    QList<QColor> bgColors;
    QStringList slFgColors;
    QStringList slBgColors;
    QString currFgColorName;
    QString currBgColorName;
    QList<QAction*> editActs;
    QString lastSuffix;
    QString appPath;
    QString homePath;
    QString saveDir;
    QStringList recentFiles;
    QMenu* recentMenu;
    QMenu* tabMenu;
    QString textSS;
    QList<int> stFontSizes;
    QMenu *prefMenu;
    int tabTitleLen = TAB_TITLE_LEN;
    int thumbHeight;
    CFindTextPanel* findTextPanel;
    QShortcut* ctrlF;
    QShortcut* keyF3;
    QShortcut* keyShF3;
    QShortcut* keyEsc;
    bool isHttpRequest;
    int httpReqCount;
    int httpCountdown;
    QHash<QString, QString> resCache;
    QNetworkAccessManager *nm;
    QRect pdfMargins;
    bool isShrinkHtml = false;
    int imageQuality = -1;  // -1 - defaul quality
    QString backupDir;
    void addTab(QString filePath);
    CTextEdit* currText() const;
    //QVector<CConfigList::Param> params;
    CConfigList::ParamList params;
    void setupParams();
    void setupMenuActions();
    void setupTextActions();
    void setEditActsEnabled(bool enabled);
    bool maybeSave(int index = -1);
    void setCurrentFileName(const QString &fileName);
    void modifyIndentation(int amount);
    void mergeFormatOnWordOrSelection(const QTextCharFormat &format);
    void fontChanged(const QFont &f);
    void changeTextSize(int value);
    void setTabModified(bool m);
    QString genTempDirName();
    int saveToHtml(QString newFileName, QString format = "html");
    int saveToMaff(QString userFileName);
    int saveToEpub(QString newFileName);
    QString shrinkHtml(QString html);
    bool writeShrinkHtml(QString path, QString html);
    int unpackMaff(QString zipName);
    int copyToTempDir(QString name);
    int copyFile(QString fromName, QString toName);
    void setOtherTabEnabled(int index, bool enable);
    void adjustTableFormat(QTextTable* tab);
    void createFileList();
    void setFileListDir(QString path);
    void statusBarMessage(const QString& text);

private slots:
    void fileOpen();
    void onFileSave();
    bool fileSave(QString name, QString suffix = "");
    bool fileSaveAs();
    bool fileExportTo();
    void filePrint();
    void filePrintPreview();
    void filePrintPdf();
    void textBold();
    void textUnderline();
    void textItalic();
    void textFamily(const QString &f);
    void textSize(const QString &p);
    void textStyle(int styleIndex);
    void setChecked(bool checked);
    void indent();
    void unindent();
    void clipboardDataChanged();
    void about();
    void printPreview(QPrinter *);
    void onLinkClicked(QString link);
    void onCopy();
    void onPaste();
    void onCut();
    void onUndo();
    void onRedo();
    void onFgColor(QColor color);
    void onBgColor(QColor color);
    void onTextLarger();
    void onTextSmaller();
    void onSelectionChanged();
    void onTextDoubleClick();
    void onAlignMenuAct();
    void onTextAlign(bool checked);
    void fileAttach();
    void onTabChanged(int index);
    void onTabClose();
    void onTabCloseRequest(int index);
    //void onTedError(QString text);
    void onTedMessage(QTextFormat &msg);
    void onCursorOnLink(QString href);
    void onSaveResAsFile(QString name);
    void onTableMenuAct(bool checked);
    void onRecentFileMenuAct();
    void addRecentFile(QString fileName = "");
    void onDefaultFontSelect();
    void onAddResFile(QTextImageFormat &fmt);
    void onHttpFinished(QNetworkReply *resp);
    void onOptionsMenuAct();
    void onFindTextPanel();
    void onFindText();
    void onTabRefresh();
    void onNextTab();
    void onTabContextMenu(const QPoint &);
    void onOptions();
    void onFileBrowser(bool checked);
    void onMakeBackup();
};


class CWaitCursor : public QObject
{
    Q_OBJECT
public:
    enum CursorStatus { NotWait, Wait, Final};
    static int waitCursorStatus;
    CWaitCursor()
    {
        if (waitCursorStatus == NotWait)
        {
            QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
            qDebug() << "CWaitCursor:  setOverrideCursor";
        }
        waitCursorStatus = Wait;
    };
    ~CWaitCursor()
    {
        if (waitCursorStatus == Wait)
        {
            waitCursorStatus = Final;
            QCoreApplication::processEvents(QEventLoop::AllEvents, 1000);
            if (waitCursorStatus == Final)
            {
                QGuiApplication::restoreOverrideCursor();
                qDebug() << "CWaitCursor:  restoreOverrideCursor";
                waitCursorStatus = NotWait;
            }
        }
    };
};


class CWaitObj : public QObject
{
    Q_OBJECT
public:
    explicit CWaitObj(QObject *parent = nullptr): QObject(parent)
    {
        timer = new QTimer(this);
        timer->setSingleShot(true);
        timeout = 15000;
        errCode = 0;
    }
    ~CWaitObj()
    {
        disconnect(timer, &QTimer::timeout, this, &CWaitObj::onTimer);
        delete timer;
    }

    void onTimer()
    {
        loop.exit();
        errCode = 1;
        qDebug()<<"CWaitObj:  timeout: " << timeout/1000;
    }

    void wait()
    {
        connect(timer, &QTimer::timeout, this, &CWaitObj::onTimer);

        timer->start(timeout);
        loop.exec();
    }

    void exit()
    {
        loop.exit();
    }

    int errCode;
    int timeout;

private:
    QTimer* timer;
    QEventLoop loop;
};


extern TextEdit *winMain;
extern QString appName;
extern QString iconDir;
extern QSize fileDialogSize;
extern void delay(int msec);

#endif // TEXTEDIT_H

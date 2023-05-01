#ifndef CTEXTEDIT_H
#define CTEXTEDIT_H

#include <QObject>
#include <QApplication>
#include <QTextEdit>
#include <QMouseEvent>
#include <QMimeData>
#include <QTextBlock>
#include <QTextDocumentFragment>
#include <QScrollBar>



class ScrollBar : public QScrollBar
{
    Q_OBJECT

public:
    ScrollBar(QWidget *parent = 0);  // doc: Constructs a vertical scroll bar.

private:
    void mousePressEvent(QMouseEvent *ev) override;
    void wheelEvent(QWheelEvent *ev) override;
};



class CTextEdit : public QTextEdit
{
    Q_OBJECT

    enum PathType { FullPathType, DocPathType };

    class WaitCursor : public QObject
    {
        //Q_OBJECT
    public:
        WaitCursor()
        {
            QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        };
        ~WaitCursor()
        {
            QGuiApplication::restoreOverrideCursor();
        };
    };

public:
    static const int InitFontPointSize = 10;
    static const int LineWrapMargin = 30;
    const int maxAnchorLength = 64;
    enum MessageType { NoType, Question, Information, Warning, Critical, StatusBar };
    enum MessagePropType { NoTypeProp, TypeProp, TextProp, FlagsProp };

    //enum LoadFlag { Image, Thumb, Clip, Local, Inet };
    //Q_DECLARE_FLAGS(LoadFlags, LoadFlag)

    enum LoadFlags
    {
        None = 0,
        Image = 1 << 0, // 1
        Thumb = 1 << 1, // 2
        Clip = 1 << 2, // 4
        Local = 1 << 3, // 8
        Inet = 1 << 4 // 16
        // = 1 << 5, // 32
        // = 1 << 6, // 64
        // = 1 << 7  //128
    };

    QString zipFileName;
    QString tempPath;
    QString mime;
    CTextEdit(QWidget *parent = 0);
    void setFileName(QString name);
    QString getFileName();
    void setFilesDir(QString name);
    QString getFilesDir();
    QSize getImageSize(QString name);
    void insertFile(QString urlStr);
    void setThumbnailHeight(int h);
    void refresh();
    void setNetworkEnabled(bool y);
    QList<QString> getResourceFileList(QTextDocument* docum = nullptr);
    void setResourceDir(QString dir, QTextDocument* docum = nullptr);
    void setImageQuality(int quality);

signals:
    void linkClicked(QString href);
    void doubleClick();
    //void error(QString text);
    void message(QTextFormat &msg);
    void cursorOnLink(QString href);
    void saveAsFile(QString href);
    void addResFile(QTextImageFormat &format);

private slots:
    void onSaveAsFile();
    void onMenuAct();
    void onMenuAct2();

private:
    bool isHandCursor;
    bool isNetworkEnabled;
    QCursor editCursor;
    QString fileName;
    QString filesDir;
    QHash<QString, QSize> imageSizes;
    int thumbHeight;
    int imageQuality = -1;  // -1 - defaul quality
    QHash<QString, int> netReqTime;
    QHash<QString, QString> menuIcons;
    QTextFragment getCurrFrag(QTextCursor &cur);
    void setCurrFragFont(QTextCursor &cur, QFont &font);
    //QString downloadFile(QString urlStr, QString flags);
    QString downloadFile(QString urlStr, int flags = None);
    bool isHttpLink(QString link);
    QString pathToPath(QString path, CTextEdit::PathType type);
    void setHandCursor(const QString& link);
    void keyPressEvent(QKeyEvent *e) override;
    void keyReleaseEvent(QKeyEvent *ev) override;
    void mouseMoveEvent(QMouseEvent *ev) override;
    void mouseReleaseEvent(QMouseEvent *ev) override;
    void mouseDoubleClickEvent(QMouseEvent *ev) override;
    void contextMenuEvent(QContextMenuEvent *ev) override;
    bool canInsertFromMimeData(const QMimeData *source) const override;
    void insertFromMimeData(const QMimeData * source) override;
    QVariant loadResource(int type, const QUrl &name) override;
    void fitImage(QTextImageFormat& fmt);
    void emitMessage(MessageType type, QString text);
    bool secondPassed(QString addr);
};

//Q_DECLARE_OPERATORS_FOR_FLAGS(CTextEdit::LoadFlags)
// may be
//Q_DECLARE_METATYPE(CTextEdit::LoadFlag)
//Q_DECLARE_METATYPE(CTextEdit::LoadFlags)

#endif // CTEXTEDIT_H

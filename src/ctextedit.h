#ifndef CTEXTEDIT_H
#define CTEXTEDIT_H

#include <QObject>
#include <QApplication>
#include <QTextEdit>
#include <QMouseEvent>
#include <QMimeData>
#include <QTextBlock>
#include <QTextDocumentFragment>


class CTextEdit : public QTextEdit
{
    Q_OBJECT

    enum pathTypes { fullPathType, docPathType };

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
    QString zipFileName;   // You will do it right
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

signals:
    void linkClicked(QString href);
    void doubleClick();
    void error(QString text);
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
    QHash<QString, int> netReqTime;
    QHash<QString, QString> menuIcons;
    QTextFragment getCurrFrag(QTextCursor &cur);
    void setCurrFragFont(QTextCursor &cur, QFont &font);
    QString downloadFile(QString urlStr, QString flags);
    bool isHttpLink(QString link);
    QString pathToPath(QString path, CTextEdit::pathTypes type);
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
};


#endif // CTEXTEDIT_H

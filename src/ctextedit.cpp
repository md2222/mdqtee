#include <QDebug>
#include <QImageReader>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QTextTable>
#include <QMenu>
#include <QAction>
#include <QDateTime>
#include <QClipboard>
#include <QPainter>
#include <QPicture>
#include <QMimeDatabase>
#include <QMutex>
#include <QScrollBar>
#include <QRegularExpression>
#include <QColorSpace>
#include "ctextedit.h"


extern QString iconDir;
QMutex mtxLoad;


CTextEdit::CTextEdit(QWidget *parent)
{
    isHandCursor = false;
    isNetworkEnabled = true;
    viewport()->setMouseTracking(true);
    setFontPointSize(10);

    menuIcons["&Undo"] = iconDir + "editundo.png";
    menuIcons["&Redo"] = iconDir + "editredo.png";
    menuIcons["Cu&t"] = iconDir + "editcut.png";
    menuIcons["&Copy"] = iconDir + "editcopy.png";
    menuIcons["&Paste"] = iconDir + "editpaste.png";
    menuIcons["Delete"] = iconDir + "editdelete.png";
    menuIcons["Select All"] = iconDir + "editselectall.png";
}


void CTextEdit::keyPressEvent(QKeyEvent *ev)
{
    if (ev->key() == Qt::Key_Control)
    {
        QString href = anchorAt(mapFromGlobal(QCursor::pos()));
        qDebug() << "CTextEdit::keyPressEvent:    href=" << href;

        setHandCursor(href);
    }

    if(ev->modifiers() & Qt::ShiftModifier || ev->modifiers() & Qt::ControlModifier)
    {
        qDebug() << "keyPressEvent: Modifier + " << ev->key();

        if (ev->modifiers() & Qt::ControlModifier && ev->key() == Qt::Key_Y)
            redo();
        else if (ev->modifiers() & Qt::ControlModifier && ev->key() == Qt::Key_Down)
        {
            moveCursor(QTextCursor::EndOfBlock);

            QTextCursor cur = textCursor();
            QTextBlockFormat blockFormat;
            QTextCharFormat fmt;
            cur.insertBlock(blockFormat);
            cur.setCharFormat(fmt);
            setTextCursor(cur);
        }
        else if (ev->key() == Qt::Key_Space || ev->key() == Qt::Key_Return)
        {
            QTextCursor cur = textCursor();
            qDebug() << "keyPressEvent: Key_Space  " << cur.position() << cur.positionInBlock()
                     << cur.selectionStart() << cur.selectionEnd();

            if (cur.charFormat().isAnchor())
            {
                QTextFragment frag = getCurrFrag(cur);
                if (frag.isValid())
                    cur.setPosition(frag.position() + frag.length(), QTextCursor::MoveAnchor);
            }
            else
                moveCursor(QTextCursor::EndOfWord);

            QFont font = cur.charFormat().font();
            QTextCharFormat fmt;

            if (ev->modifiers() & Qt::ShiftModifier)
            {
                fmt.setFontFamily(font.family());
                fmt.setFontPointSize(font.pointSizeF());
            }

            cur.setCharFormat(fmt);
            cur.insertText(" ", fmt);

            if (ev->modifiers() & Qt::ControlModifier)
            {
                ev->setModifiers(Qt::NoModifier);
            }
        }
    }

    QTextEdit::keyPressEvent(ev);
}


void CTextEdit::keyReleaseEvent(QKeyEvent *ev)
{
    QTextEdit::keyReleaseEvent(ev);

    if (ev->key() == Qt::Key_Control)
    {
        setHandCursor("");
    }
}


void CTextEdit::mouseMoveEvent(QMouseEvent *ev)
{
    static int moveCount = 0;

    // do not need so often
    if (moveCount++ > 5)
    {
        moveCount = 0;

        if (ev->modifiers() & Qt::ControlModifier)
        {
            QString href = anchorAt(ev->pos());

            setHandCursor(href);
        }
        else
            setHandCursor("");
    }

    QTextEdit::mouseMoveEvent(ev);
}


void CTextEdit::setHandCursor(const QString& link)
{
    if (!link.isEmpty())
    {
        if (!isHandCursor)
        {
            editCursor = viewport()->cursor();
            viewport()->setCursor(Qt::PointingHandCursor);
            isHandCursor = true;
        }

        emit cursorOnLink(link);
    }
    else
    {
        if (isHandCursor)
        {
            viewport()->setCursor(editCursor);
            isHandCursor = false;
        }

        emit cursorOnLink("");
    }
}


void CTextEdit::mouseReleaseEvent(QMouseEvent *ev)
{
    QTextEdit::mousePressEvent(ev);

    if (ev->modifiers() & Qt::ControlModifier)
    {
        if (ev->button() == Qt::LeftButton)
        {
            QString href = anchorAt(ev->pos());
            if(!href.isEmpty())
            {
                if (href.left(1) == '#')
                {
                    qDebug() << "scrollToAnchor  " << href.mid(1);
                    scrollToAnchor(href.mid(1));
                }
                else
                    emit linkClicked(href);
            }
        }
    }
}


void CTextEdit::contextMenuEvent(QContextMenuEvent *ev)
{
    QMenu *menu = createStandardContextMenu();

    foreach (QAction* act, menu->actions())
    {
        QHashIterator<QString, QString> it(menuIcons);
        while (it.hasNext())
        {
            it.next();
            if (act->text().indexOf(it.key()) >= 0)
                act->setIcon(QIcon(it.value()));
        }
    }

    menu->addSeparator();
    menu->setToolTipsVisible(true);

    QTextCursor cur = cursorForPosition(ev->pos()); //++
    QString filePath;
    QString anchorHref;
    QString imgPath;
    bool isImageFormat = false;
    bool isAnchor = false;
    bool hasSelection = textCursor().hasSelection();
    QTextCharFormat charFmt;
    QTextFragment frag = getCurrFrag(cur);
    if (frag.isValid())
        charFmt = frag.charFormat();
    else
        charFmt = cur.charFormat();

    if (charFmt.isAnchor())
    {
        anchorHref = charFmt.anchorHref();
        if (!anchorHref.isEmpty() && anchorHref.left(1) != "#")
            isAnchor = true;
        qDebug() << "contextMenuEvent:  isAnchor: " << anchorHref;
    }

    if (charFmt.isImageFormat())
    {
        isImageFormat = true;
        QTextImageFormat fmt = charFmt.toImageFormat();
        imgPath = fmt.name();
        qDebug() << "contextMenuEvent:  isImageFormat: " << imgPath;
    }

    QAction* a = 0;

    /* Property for onReplaceWithLink:
    LS - Link save
    LI - Replace with image-link from clipboard
    LC - copy link to clipboard
    LR - Replace link with image

    IS - Image save
    IC - Copy image to clipboard
    IL - Replace with link - for images
    IT - Replace with thumbnail
    IF - Fit image to line wrap width
    IO - Open with external program
    */

    if (isAnchor)
    {
        if (!isHttpLink(anchorHref))
        {
            const QIcon saveAsIcon = QIcon(iconDir + "filesaveas.png");
            a = menu->addAction(saveAsIcon, "Save link as ...", this, &CTextEdit::onMenuAct);
            a->setData(ev->pos());
            a->setProperty("type", "LS");
        }

        QString fileMime = QMimeDatabase().mimeTypeForFile(anchorHref).name();
        if (fileMime.indexOf("image") >= 0)
        {
            const QIcon replImageIcon = QIcon(iconDir + "image.png");
            a = menu->addAction(replImageIcon, "Replace link with image", this, &CTextEdit::onMenuAct);
            a->setData(ev->pos());
            a->setProperty("type", "LR");
            a->setProperty("path", anchorHref);
        }

        if (mime != "md")
        {
            const QIcon insImageIcon = QIcon(iconDir + "insert-image.png");
            a = menu->addAction(insImageIcon, "Make image from clipboard as link", this, &CTextEdit::onMenuAct);
            a->setData(ev->pos());
            a->setProperty("type", "LI");
        }

        if (isHttpLink(anchorHref))
        {
            const QIcon copyLinkIcon = QIcon(iconDir + "editcopy.png");
            a = menu->addAction(copyLinkIcon, "Copy link to clipboard", this, &CTextEdit::onMenuAct);
            a->setData(ev->pos());
            a->setProperty("type", "LC");
        }
    }

    if (isImageFormat)
    {
        if (!isHttpLink(imgPath))
        {
            const QIcon saveAsIcon = QIcon(iconDir + "filesaveas.png");
            a = menu->addAction(saveAsIcon, "Save image as ...", this, &CTextEdit::onMenuAct);
            a->setData(ev->pos());
            a->setProperty("type", "IS");
        }

        const QIcon copyImageIcon = QIcon(iconDir + "editcopy.png");
        a = menu->addAction(copyImageIcon, "Copy image to clipboard", this, &CTextEdit::onMenuAct);
        a->setData(ev->pos());
        a->setProperty("type", "IC");

        if (mime != "md")
        {
            const QIcon fitImageIcon = QIcon(iconDir + "image-fit-win.png");
            a = menu->addAction(fitImageIcon, "Fit image to line wrap width", this, &CTextEdit::onMenuAct);
            a->setToolTip("To window width if no line wrap\nCtrl+   - For all images");
            a->setData(ev->pos());
            a->setProperty("type", "IF");

            if (!isAnchor)
            {
                const QIcon insLinkIcon = QIcon(iconDir + "insert-link.png");
                a = menu->addAction(insLinkIcon, "Replace image with link", this, &CTextEdit::onMenuAct);
                a->setData(ev->pos());
                a->setProperty("type", "IL");

                const QIcon insImageIcon = QIcon(iconDir + "insert-image.png");
                a = menu->addAction(insImageIcon, "Replace image with thumbnail", this, &CTextEdit::onMenuAct);
                a->setData(ev->pos());
                a->setProperty("type", "IT");
            }
        }

        a = menu->addAction("Open image with external program", this, &CTextEdit::onMenuAct);
        a->setData(ev->pos());
        a->setProperty("type", "IO");
        a->setProperty("path", imgPath);
    }

    if (isAnchor || isImageFormat)
        menu->addSeparator();

    QString imageLink;
    const QClipboard *clip = QApplication::clipboard();
    const QMimeData *clipMime = clip->mimeData();
    if (clipMime)
    {
        if (clipMime->hasText())
        {
            QString text = clip->text();
            if (isHttpLink(text))
            {
                QString fileMime = QMimeDatabase().mimeTypeForFile(text).name();
                if (fileMime.indexOf("image") >= 0)
                    imageLink = text;
            }
        }
    }

    /* Property for onInsertConst:
    CH - Copy HTML
    PP - Paste as plain text
    PI - Paste as image
    L - Insert line
    D - Insert current date
    */

    const QIcon copyIcon = QIcon(iconDir + "editcopy.png");
    a = menu->addAction(copyIcon, "Copy HTML", this, &CTextEdit::onMenuAct2);
    a->setData("CH");
    a->setEnabled(hasSelection);

    const QIcon pasteIcon = QIcon(iconDir + "editpaste.png");
    a = menu->addAction(pasteIcon, "Paste as plain text", this, &CTextEdit::onMenuAct2);
    a->setData("PP");

    if (!imageLink.isEmpty())
    {
        const QIcon pasteIcon = QIcon(iconDir + "editpaste.png");
        a = menu->addAction(pasteIcon, "Paste as image", this, &CTextEdit::onMenuAct2);
        a->setData("PI");
        a->setProperty("http", imageLink);
        a->setToolTip("Download image and paste");
    }

    if (mime != "md")
    {
        a = menu->addAction("Line", this, &CTextEdit::onMenuAct2);
        a->setData("L");
        a->setToolTip("Insert line");
    }

    a = menu->addAction("Current date", this, &CTextEdit::onMenuAct2);
    a->setData("D");
    a->setToolTip("Insert current date");

    a = menu->addAction("Merge blocks", this, &CTextEdit::onMenuAct2);
    a->setData("MB");
    a->setEnabled(hasSelection);

    menu->exec(ev->globalPos());
    delete menu;
}


void CTextEdit::mouseDoubleClickEvent(QMouseEvent *ev)
{
    QTextEdit::mouseDoubleClickEvent(ev);

    emit doubleClick();
}


bool CTextEdit::canInsertFromMimeData(const QMimeData *source) const
{
    if (source->hasImage())
        return true;
    else if (source->hasUrls())
        return true;
    else
        return QTextEdit::canInsertFromMimeData(source);
}


// when paste, drop, not when loading
void CTextEdit::insertFromMimeData(const QMimeData* source)
{
    if (source->hasImage())
    {
        WaitCursor wc;

        QImage image = qvariant_cast<QImage>(source->imageData());
        qDebug() << "insertFromMimeData:  image size:  " << image.width() << image.height();

        QString fileName = QString("image-%1.png").arg(time(NULL));
        qDebug() << "insertFromMimeData:  fileName:  " << fileName;

        if (!image.save(pathToPath(fileName, fullPathType), 0, -1))
        {
            qDebug() << "insertFromMimeData:  save image error ";
            emit error("Save image error.\n" + pathToPath(fileName, fullPathType));  // QueuedConnection
            return;
        }

        imageSizes[fileName] = image.size();

        QTextImageFormat tif;
        tif.setName(pathToPath(fileName, docPathType));
        int w = image.width();
        int ww = lineWrapColumnOrWidth();
        if (!ww)  ww = width() - 30;
        if (w > ww)  w = ww;
        tif.setWidth(w);

        textCursor().insertImage(tif);
    }
    else if (source->hasUrls())
    {
        foreach(QUrl url, source->urls())
        {
            QString urlStr = url.toString();
            qDebug() << "insertFromMimeData:  url:  " << urlStr;

            if (isHttpLink(urlStr))
            {
                bool ok = false;
                QString fileMime = QMimeDatabase().mimeTypeForFile(urlStr).name();
                // when drop image
                if (fileMime.indexOf("image") >= 0)
                {
                    QString fileName = downloadFile(urlStr, "H");
                    if (!fileName.isEmpty())
                    {
                        insertHtml("<img src=\"" + fileName.replace(" ", "%20") + "\">");
                        ok = true;
                    }
                }

                if (!ok)
                    insertHtml("<a href=\"" + urlStr + "\">" + urlStr + "</a>");
            }
            else
                insertFile(urlStr);
        }
    }
    else if (source->hasHtml())
    {
         qDebug() << "source->hasHtml: " << source->text();
         QString html;
         QString text = source->text();
         if (source->hasText() && Qt::mightBeRichText(text))  html = text;
         else  html = source->html();

         html.replace("<textarea", "<pre");
         html.replace("textarea>", "pre>");

         QTextCursor cur = textCursor();
         QFont font = textCursor().charFormat().font();
         cur.beginEditBlock();
         cur.insertHtml(html);
         setCurrFragFont(cur, font);
         cur.endEditBlock();
    }
    else if (source->hasText())
    {
        qDebug() << "source->hasText: " << source->text();
        QString text = source->text().trimmed();

        QTextCursor cur = textCursor();
        QFont font = cur.charFormat().font();

        // if html
        if (Qt::mightBeRichText(text))
        //if ( text.indexOf("<") >= 0 && (text.indexOf("</") > 0 || text.indexOf("><") > 0) )
        {
            qDebug() << "paste HTML text";
            cur.beginEditBlock();
            cur.insertHtml(text);
            setCurrFragFont(cur, font);
            cur.endEditBlock();
        }
        else
        {
            // make links clickable
            QRegularExpression exp = QRegularExpression("((http|https)://\\S+)");
            exp.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
            QRegularExpressionMatchIterator matchIter = exp.globalMatch(text);
            QList<QRegularExpressionMatch> matchList;

            while (matchIter.hasNext())
            {
                QRegularExpressionMatch match = matchIter.next();
                matchList.push_front(match);  // will replace from the end
            }

            foreach (QRegularExpressionMatch match, matchList)
                text.replace(match.capturedStart(), match.capturedLength(), "<a href=\"" + match.captured() + "\">" + match.captured() + "</a>");

            //cur.beginEditBlock();   // cursor jumping on redo
            if (matchList.count() > 0)
                cur.insertHtml(text);
            else
                cur.insertText(text);

            setCurrFragFont(cur, font);
            //cur.endEditBlock();
        }
    }
    else
    {
        QTextEdit::insertFromMimeData(source);
    }
}


void CTextEdit::insertFile(QString urlStr)
{
    QString fileName = downloadFile(urlStr, "F");
    if (fileName.isEmpty())  return;

    QTextCursor cur = textCursor();
    QFont font = textCursor().charFormat().font();

    cur.beginEditBlock();
    cur.insertHtml("<a href=\"" + fileName.replace(" ", "%20") + "\">" + QFileInfo(fileName).fileName() + "</a>");
    setCurrFragFont(cur, font);
    cur.endEditBlock();
}


void CTextEdit::setCurrFragFont(QTextCursor &cur, QFont &font)
{
    QTextCharFormat fmt = cur.charFormat();
    fmt.setFontFamily(font.family());
    fmt.setFontPointSize(font.pointSizeF());

    QTextFragment frag = getCurrFrag(cur);
    cur.setPosition(frag.position());
    cur.setPosition(frag.position() + frag.length(), QTextCursor::KeepAnchor);
    cur.setCharFormat(fmt);
}


QString CTextEdit::downloadFile(QString urlStr, QString flags)
{
    QTextImageFormat fmt;
    fmt.setProperty(1, flags);  // I - image, T - thumbnail, C - from clipboard, F - local file, H - internet
    fmt.setName(urlStr);
    emit addResFile(fmt);

    return fmt.name();
}


bool CTextEdit::isHttpLink(QString link)
{
    return link.indexOf("http") == 0;
}


QString CTextEdit::pathToPath(QString path, CTextEdit::pathTypes type)
{
    QString s;
    if (type == fullPathType)
        s = filesDir + "/" + QFileInfo(path).fileName();
    else if (type == docPathType)
        s = QFileInfo(filesDir).fileName() + "/" + QFileInfo(path).fileName();

    return s;
}


void CTextEdit::onSaveAsFile()
{
    QString fileName = qobject_cast<QAction*>(sender())->data().toString();
    emit saveAsFile(fileName);
}


void CTextEdit::fitImage(QTextImageFormat& fmt)
{
    QString fileName = fmt.name();
    if (!fileName.isEmpty())
    {
        fileName = QFileInfo(fileName).fileName().replace("%20", " ");
        QSize imageSize = getImageSize(fileName);
        qDebug() << "CTextEdit::fitImage:   imageSize=" << fmt.width() << fmt.height() << imageSize.width() << imageSize.height();
        qreal w = imageSize.width();
        qreal h = imageSize.height();
        qreal ratio = 0;
        if (h > 0)  ratio = w/h;

        //w = width() - 30;
        int ww = lineWrapColumnOrWidth();
        if (!ww)  ww = width() - 30;
        //if (w > ww)
        {
            w = ww;

            if (ratio > 0.001)
                h = qRound(w/ratio);
            qDebug() << "CTextEdit::onMenuAct:   IF:   ratio  " << w << h << ratio;

            if (fmt.width() > 0 || fmt.height() <= 0)  fmt.setWidth(w);
            if (fmt.height() > 0)  fmt.setHeight(h);
        }
    }
}


void CTextEdit::onMenuAct()
{
    QAction* a = qobject_cast<QAction*>(sender());
    QPoint pos = a->data().toPoint();

    QTextCursor cur = cursorForPosition(pos);
    qDebug() << "CTextEdit::onMenuAct:    positions  " << cur.position() << textCursor().position();

    QString type = a->property("type").toString();
    if (type.isEmpty())  return;

    QString html;

    /* Property for onReplaceWithLink:
    LI - Replace with image-link from clipboard
    LC - copy link to clipboard
    LR - Replace link with image
    IC - Copy image to clipboard
    IL - Replace with link - for images
    IT - Replace with thumbnail
    IF - Fit image to window width
    IO - Open with external program
    */
    QTextCharFormat fmt = cur.charFormat();

    if (type == "LS")
    {
        emit saveAsFile(fmt.anchorHref());
    }
    else if (type == "LR")
    {
        QString filePath = a->property("path").toString();

        if (isHttpLink(filePath))
            downloadFile(filePath, "IH");

        html = "<img src=\""+ pathToPath(filePath, docPathType) + "\">";
    }
    else if (type == "LI")
    {
        QTextImageFormat imgFmt = fmt.toImageFormat();
        imgFmt.setProperty(1, "ITC");  // I - image, T - thumbnail, С - from clipboard
        emit addResFile(imgFmt);
        if (imgFmt.name().isEmpty())  return;

        QString attr;
        if (imgFmt.height() > 0)
            attr = " height=" +  QString::number(imgFmt.height());
        html = "<a href=\"" + fmt.anchorHref() + "\"><img src=\""
                + imgFmt.name() + "\"" + attr + "></a> ";
    }
    else if (type == "LC")
    {
        QClipboard *clip = QGuiApplication::clipboard();
        clip->setText(fmt.anchorHref());
    }

    QTextImageFormat imgFmt = fmt.toImageFormat();

    if (type == "IS")
    {
        emit saveAsFile(imgFmt.name());
    }
    else if (type == "IL")
    {
        html = "<a href=\"" + imgFmt.name() + "\">" + QFileInfo(imgFmt.name()).fileName() + "</a> ";
    }
    else if (type == "IT")
        html = "<a href=\"" + imgFmt.name() + "\"><img src=\"" + imgFmt.name()
                + "\" height=" + QString::number(thumbHeight) + "></a> ";
    // Fit image to window width
    else if (type == "IF")
    {
        if (QApplication::keyboardModifiers() & Qt::CTRL)
        {
            QTextBlock bl = document()->begin();

            while (bl.isValid())
            {
                QTextBlock::iterator it;

                for (it = bl.begin(); !(it.atEnd()); ++it)
                {
                    QTextFragment currFrag = it.fragment();

                    if (currFrag.isValid())
                    {
                        QTextCharFormat fmt = currFrag.charFormat();

                        if (fmt.isImageFormat())
                        {
                            QTextImageFormat imgFmt = fmt.toImageFormat();

                            fitImage(imgFmt);

                            if (imgFmt.isValid())
                            {
                                cur.setPosition(currFrag.position());
                                cur.setPosition(currFrag.position() + currFrag.length(), QTextCursor::KeepAnchor);
                                cur.setCharFormat(imgFmt);
                            }
                        }
                    }
                }

                bl = bl.next();
            }
        }
        else
        {
            fitImage(imgFmt);

            if (imgFmt.isValid())
            {
                QTextFragment frag = getCurrFrag(cur);
                if (frag.isValid())
                {
                    cur.setPosition(frag.position());
                    cur.setPosition(frag.position() + frag.length(), QTextCursor::KeepAnchor);
                    cur.setCharFormat(imgFmt);
                }
            }
        }
    }
    else if (type == "IC")
    {
        QString path = pathToPath(imgFmt.name(), fullPathType);
        QImage image(path);
        qDebug() << "CTextEdit::onMenuAct:   IC:   image:  " << path << image.isNull();
        Q_ASSERT(!image.isNull());
        image.setColorSpace(QColorSpace());  //++ image recognized in insertFromMimeData
        QApplication::clipboard()->setImage(image, QClipboard::Clipboard);
    }
    else if (type == "IO")
    {
        QString filePath = a->property("path").toString();
        if (!filePath.isEmpty())
            emit linkClicked(filePath);
    }

    if (!html.isEmpty())
    {
        QTextFragment frag = getCurrFrag(cur);
        if (frag.isValid())
        {
            cur.setPosition(frag.position());  //++
            cur.setPosition(frag.position() + frag.length(), QTextCursor::KeepAnchor);
            cur.beginEditBlock();
            cur.removeSelectedText();
            cur.insertHtml(html);
            cur.endEditBlock();
        }
    }
}


void CTextEdit::onMenuAct2()
{
    QAction* act = qobject_cast<QAction*>(sender());
    QString s = act->data().toString();
    /* Property:
    CH - Copy HTML
    PP - Paste as plain text
    PI - Paste as image
    L - Insert line
    D - Insert current date
    */

    if (s == "CH")
    {
        QTextCursor cur = textCursor();
        if (cur.hasSelection())
        {
            QClipboard *clip = QApplication::clipboard();
            if (clip)
            {
                clip->setText(cur.selection().toHtml());
            }
        }
    }
    else if (s == "PP")
    {
        QClipboard *clip = QApplication::clipboard();
        const QMimeData *mimeData = clip->mimeData();
        if (mimeData->hasText())
        {
            QString text = clip->text();
            //insertPlainText(text);
            QTextCursor cur = textCursor();
            QFont font = textCursor().charFormat().font();
            QTextCharFormat fmt;
            cur.setCharFormat(fmt);
            cur.beginEditBlock();
            cur.insertText(text, fmt);
            setCurrFragFont(cur, font);
            cur.endEditBlock();
        }
    }
    else if (s == "PI")
    {
         QString imageLink = act->property("http").toString();

         downloadFile(imageLink, "IH");

         textCursor().insertHtml("<img src=\"" + pathToPath(imageLink, docPathType) + "\">");
    }
    else if (s == "L")
    {
        QTextDocumentFragment fr = QTextDocumentFragment::fromHtml("<hr><br>");
        textCursor().insertFragment(fr);
        ensureCursorVisible();
    }
    else if (s == "D")
    {
        QDateTime dt = QDateTime::currentDateTime();
        insertPlainText(dt.toString("d.MM.yyyy "));
    }
    else if (s == "MB")
    {

        QTextCursor cur = textCursor();
        if (!cur.hasSelection())  return;

        int begin = cur.selectionStart();
        int end = cur.selectionEnd();

        QTextBlock blBegin = this->document()->findBlock(begin);
        QTextBlock blEnd = this->document()->findBlock(end);

        if (!blBegin.isValid() || !blEnd.isValid())  return;
        if (blBegin == blEnd)  return;

        begin = blBegin.position();
        end = blEnd.position() + blEnd.length() - 1;
        cur.setPosition(begin);
        cur.setPosition(end, QTextCursor::KeepAnchor);
        qDebug() << "CTextEdit::onMenuAct2 - L:  pos: " << begin << end << blBegin.blockNumber() << blEnd.blockNumber();

        QTextCursor cur2 = textCursor();
        cur2.setPosition(begin);
        cur2.movePosition(QTextCursor::StartOfBlock);
        //cur2.insertBlock(blBegin.blockFormat(), blBegin.charFormat());
        cur2.beginEditBlock();

        for (QTextBlock b = blBegin; b.isValid(); b = b.next())
        {
            qDebug() << "CTextEdit::onMenuAct2 - L:  bl" << b.blockNumber();

            for (QTextBlock::Iterator it = b.begin(); !it.atEnd(); ++it)
            {
                QTextFragment fr(it.fragment());
                if (!fr.isValid()) continue;

                qDebug() << "CTextEdit::onMenuAct2 - L:  fragment pos, text:" << fr.position() << fr.text();
                cur2.insertText(fr.text(), fr.charFormat());
            }

            if (b == blEnd)  break;
        }

        cur.removeSelectedText();
        cur2.endEditBlock();
    }
}


QTextFragment CTextEdit::getCurrFrag(QTextCursor &cur)
{
    QTextFragment frag;
    const int curPos = cur.position();
    QTextBlock::iterator it;
    for (it = cur.block().begin(); !(it.atEnd()); ++it)
    {
        frag = it.fragment();
        if (frag.isValid())
        {
            const int fragPos = frag.position();
            qDebug() << "getCurrFrag: fragPos  frag.length()  curPos  " << fragPos << frag.length() << curPos;
            if (fragPos <= curPos && fragPos + frag.length() >= curPos)
                break;
        }
    }

    return frag;
}


// + runs on paste
// type - https://doc.qt.io/qt-5/qtextdocument.html#ResourceType-enum
QVariant CTextEdit::loadResource(int type, const QUrl &name)
{
    qDebug() << "loadResource: " << name;

    if (type == QTextDocument::ImageResource)
    {
        QImage image;
        QString fileName;
        QString filePath;
        QString addr = name.toString();

        if (addr.indexOf(":/") == 0)
            image = QImage(addr);
        else if (isHttpLink(addr))
        {
            if (!isNetworkEnabled)
                image = QImage(":/images/linux/no-network.png");
            else
            {
                // do not need so often
                int nextTime = 0;
                time_t t = time(NULL);
                if (netReqTime.contains(addr))
                    nextTime = netReqTime[addr];

                if (nextTime <= t)
                {
                    netReqTime[addr] = t + 1;

                    QString fn = downloadFile(name.toString(), "IH");

                    if (!fn.isEmpty())
                    {
                        qDebug() << "loadResource: loaded image=" << fn;
                        fileName = QFileInfo(fn).fileName();
                        if (!document()->isModified())
                            document()->setModified(true);
                    }
                }
            }
        }
        else
            fileName = QFileInfo(name.toString()).fileName();

        if (!fileName.isEmpty())
        {
            QByteArray ba;
            filePath = pathToPath(fileName, fullPathType);
            QFile file(filePath);
            if(file.open(QIODevice::ReadOnly))
                ba = file.readAll();
            QString format = QFileInfo(fileName).suffix().toUpper();
            qDebug() << "loadResource file.readAll " << filePath << fileName << format << ba.size();

            if (ba.size())
            {
                image = QImage::fromData(ba, format.toLatin1());
                qDebug() << "loadResource: image.size():  " << image.width() << image.height();

                imageSizes[fileName] = image.size();
            }
        }

        if (image.isNull())
        {
            image = QImage(":/images/linux/image-not-found.png"); //++
            qDebug() << "loadResource:  image.isNull  " << filePath << fileName;
        }

        return image;
    }
    else
        return QTextEdit::loadResource(type, name);
}


QSize CTextEdit::getImageSize(QString name)
{
    qDebug() << "getImageSize:  name=" << name;
    if (imageSizes.contains(name))
        return imageSizes[name];
    else
        return QSize(0, 0);
}


void CTextEdit::setFileName(QString name)
{
    fileName = name;
}


QString CTextEdit::getFileName()
{
    return fileName;
}


void CTextEdit::setFilesDir(QString name)
{
    filesDir = name;
}


QString CTextEdit::getFilesDir()
{
    return filesDir;
}


void CTextEdit::setThumbnailHeight(int h)
{
    thumbHeight = h;
}


void CTextEdit::refresh()
{
    int p = verticalScrollBar()->value();
    bool isModified = document()->isModified();
    QString html = toHtml();
    document()->clear();
    insertHtml(html);
    qDebug() << "refresh: insertHtml";
    verticalScrollBar()->setValue(p);
    document()->setModified(isModified);
}


void CTextEdit::setNetworkEnabled(bool y)
{
    isNetworkEnabled = y;
}


QList<QString> CTextEdit::getResourceFileList(QTextDocument* docum)
{
    qDebug() << "getResourceFileList";
    QTextDocument* doc = docum ? docum : document();
    QTextBlock bl = doc->begin();

    QList<QString> fileList;

    while (bl.isValid())
    {
        QTextBlock::iterator it;

        for (it = bl.begin(); !(it.atEnd()); ++it)
        {
            QTextFragment currFrag = it.fragment();

            if (currFrag.isValid())
            {
                QTextCharFormat fmt = currFrag.charFormat();

                if (fmt.isAnchor())
                {
                    QString href = fmt.anchorHref();
                    if (!href.isEmpty() && href.left(1) != "#" && !isHttpLink(href))
                        fileList.push_back(QFileInfo(href).fileName().replace("%20", " "));
                }

                if (fmt.isImageFormat())
                {
                    QTextImageFormat imgFmt = fmt.toImageFormat();
                    QString fileName = imgFmt.name();
                    if (!fileName.isEmpty())
                        fileList.push_back(QFileInfo(fileName).fileName().replace("%20", " "));
                }
            }
        }

        bl = bl.next();
    }

    //foreach(QString name, fileList)
        //qDebug() << name;

    return fileList;
}


void CTextEdit::setResourceDir(QString dir, QTextDocument* docum)
{
    QTextDocument* doc = docum ? docum : document();
    QTextCursor cur(doc);
    QTextBlock bl = doc->begin();

    while(bl.isValid())
    {
        QTextBlock::iterator it;

        for(it = bl.begin(); !(it.atEnd()); ++it)
        {
            QTextFragment currFrag = it.fragment();

            if(currFrag.isValid())
            {
                QString fileName = "";
                QTextCharFormat fmt = currFrag.charFormat();

                // anchor first otherwise it will restore image
                if(fmt.isAnchor())
                {
                     QString href = fmt.anchorHref().trimmed();

                     if (!href.isEmpty() && href.left(1) != "#" && !isHttpLink(href))
                     {
                         fileName = dir + "/" + QFileInfo(href).fileName();
                         qDebug() << "setResDir: new href: " << fileName;
                         fileName.replace(" ", "%20");
                         fmt.setAnchorHref(fileName);

                         cur.setPosition(currFrag.position());
                         cur.setPosition(currFrag.position() + currFrag.length(), QTextCursor::KeepAnchor);
                         cur.setCharFormat(fmt);
                     }
                }

                if(fmt.isImageFormat())
                {
                    QTextImageFormat imgFmt = fmt.toImageFormat();
                    fileName = dir + "/" + QFileInfo(imgFmt.name()).fileName();
                    qDebug() << "setResDir: new image name: " << fileName << currFrag.text() << currFrag.length();
                    fileName.replace(" ", "%20");
                    imgFmt.setName(fileName);

                    cur.setPosition(currFrag.position());
                    cur.setPosition(currFrag.position() + currFrag.length(), QTextCursor::KeepAnchor);
                    cur.setCharFormat(imgFmt);
                }
            }
        }

        bl = bl.next();
    }
}



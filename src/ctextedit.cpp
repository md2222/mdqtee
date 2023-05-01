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



ScrollBar::ScrollBar(QWidget *parent)
{
    setParent(parent);
}


void ScrollBar::mousePressEvent(QMouseEvent *ev)
{
    qDebug() << "ScrollBar::mousePressEvent:    " << ev->pos().y() << sliderPosition() << height() << maximum();

    if (ev->modifiers() & Qt::ControlModifier)
    {
        qreal unit  = maximum() / height();
        int mousePos = ev->pos().y() * unit;
        qDebug() << "ScrollBar::mousePressEvent:   unit  mousePos   " << unit << mousePos;

        if (mousePos > sliderPosition())
            triggerAction(SliderToMaximum);
        else
            triggerAction(SliderToMinimum);

        ev->setAccepted(true);
    }
    else
        QScrollBar::mousePressEvent(ev);
}


void ScrollBar::wheelEvent(QWheelEvent *ev)
{
    qDebug() << "ScrollBar::wheelEvent:    "  << ev->angleDelta();

    if (ev->modifiers() & Qt::ControlModifier)
    {
        if (ev->angleDelta().y() > 0)
            triggerAction(SliderPageStepSub);
        else
            triggerAction(SliderPageStepAdd);

        ev->setAccepted(true);
    }
    else
        QScrollBar::wheelEvent(ev);
}
//------------------------------------------------------------------------------------------------------


CTextEdit::CTextEdit(QWidget *parent)
{
    Q_UNUSED(parent)

    isHandCursor = false;
    isNetworkEnabled = true;
    viewport()->setMouseTracking(true);
    setFontPointSize(InitFontPointSize);

    menuIcons["&Undo"] = iconDir + "editundo.png";
    menuIcons["&Redo"] = iconDir + "editredo.png";
    menuIcons["Cu&t"] = iconDir + "editcut.png";
    menuIcons["&Copy"] = iconDir + "editcopy.png";
    menuIcons["&Paste"] = iconDir + "editpaste.png";
    menuIcons["Delete"] = iconDir + "editdelete.png";
    menuIcons["Select All"] = iconDir + "editselectall.png";

    ScrollBar* newScrollBar = new ScrollBar(this);

    setVerticalScrollBar(newScrollBar);
}


void CTextEdit::keyPressEvent(QKeyEvent *ev)
{
    bool isCtrl = ev->modifiers() & Qt::ControlModifier;
    bool isShift = ev->modifiers() & Qt::ShiftModifier;

    if (ev->key() == Qt::Key_Control)
    {
        QString href = anchorAt(mapFromGlobal(QCursor::pos()));
        qDebug() << "CTextEdit::keyPressEvent:    href=" << href;

        setHandCursor(href);
    }

    if(isCtrl || isShift)
    {
        qDebug() << "CTextEdit::keyPressEvent:    Modifier + " << ev->key() << document()->isUndoAvailable() << document()->isRedoAvailable();

        if (isCtrl && ev->key() == Qt::Key_Y)
        {
            qDebug() << "CTextEdit::keyPressEvent:    redo  " << document()->availableRedoSteps();
            redo();
            return;
        }
        else if (isCtrl && ev->key() == Qt::Key_Z)  // not need. low level func work always
        {
            qDebug() << "CTextEdit::keyPressEvent:    undo  " << document()->availableUndoSteps();
            undo();
            return;
        }
        else if (isCtrl && ev->key() == Qt::Key_Down)
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
            qDebug() << "CTextEdit::keyPressEvent:    Key_Space  " << cur.position() << cur.positionInBlock()
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

            if (isShift)
            {
                fmt.setFontFamily(font.family());
                fmt.setFontPointSize(font.pointSizeF());
            }

            cur.setCharFormat(fmt);
            cur.insertText(" ", fmt);

            if (isCtrl)
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
            {
                act->setIcon(QIcon(it.value()));
            }
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
    bool isLocalAnchor = false;

    QTextCharFormat charFmt;

    QTextFragment frag = getCurrFrag(cur);
    if (frag.isValid())
        charFmt = frag.charFormat();
    else
        charFmt = cur.charFormat();

    if (charFmt.isAnchor())
    {
        anchorHref = charFmt.anchorHref();

        if (!anchorHref.isEmpty())
        {
            isAnchor = true;
            if (anchorHref.left(1) == "#")
                isLocalAnchor = true;
        }
        qDebug() << "contextMenuEvent:  isAnchor: " << anchorHref;
    }

    if (charFmt.isImageFormat())
    {
        isImageFormat = true;
        QTextImageFormat fmt = charFmt.toImageFormat();
        imgPath = fmt.name();
        qDebug() << "contextMenuEvent:  isImageFormat: " << imgPath;
    }

    QClipboard *clip = QApplication::clipboard();
    const QMimeData *clipMime = clip->mimeData();

    QAction* a = 0;

    if (isAnchor)
    {
        const QIcon saveAsIcon = QIcon(iconDir + "filesaveas.png");
        a = menu->addAction(saveAsIcon, "Save link as ...", this, &CTextEdit::onMenuAct);
        a->setData(ev->pos());
        a->setProperty("type", "SaveLink");
        a->setEnabled(!isHttpLink(anchorHref) && !isLocalAnchor);

        QString fileMime = QMimeDatabase().mimeTypeForFile(anchorHref).name();

        const QIcon replImageIcon = QIcon(iconDir + "image.png");
        a = menu->addAction(replImageIcon, "Replace link with image", this, &CTextEdit::onMenuAct);
        a->setData(ev->pos());
        a->setProperty("type", "ReplaceLinkWithImage");
        a->setProperty("path", anchorHref);
        a->setEnabled(fileMime.indexOf("image") >= 0);

        // if not Markdown
        //if (mime != "md" && clipMime && clipMime->hasImage())
        if (mime != "md" && clipMime)
        {
            const QIcon insImageIcon = QIcon(iconDir + "insert-image.png");
            a = menu->addAction(insImageIcon, "Make image from clipboard as link", this, &CTextEdit::onMenuAct);
            a->setData(ev->pos());
            a->setProperty("type", "MakeImageAsLink");
            a->setEnabled(clipMime->hasImage());
        }

        const QIcon copyLinkIcon = QIcon(iconDir + "editcopy.png");
        a = menu->addAction(copyLinkIcon, "Copy link to clipboard", this, &CTextEdit::onMenuAct);
        a->setData(ev->pos());
        a->setProperty("type", "CopyLink");
    }

    if (isImageFormat)
    {
        const QIcon saveAsIcon = QIcon(iconDir + "filesaveas.png");
        a = menu->addAction(saveAsIcon, "Save image as ...", this, &CTextEdit::onMenuAct);
        a->setData(ev->pos());
        a->setProperty("type", "SaveImage");
        a->setEnabled(!isHttpLink(imgPath));

        const QIcon copyImageIcon = QIcon(iconDir + "editcopy.png");
        a = menu->addAction(copyImageIcon, "Copy image to clipboard", this, &CTextEdit::onMenuAct);
        a->setData(ev->pos());
        a->setProperty("type", "CopyImage");

        if (mime != "md")
        {
            const QIcon fitImageIcon = QIcon(iconDir + "image-fit-win.png");
            a = menu->addAction(fitImageIcon, "Fit image to line wrap width", this, &CTextEdit::onMenuAct);
            a->setToolTip("To window width if no line wrap\nCtrl+   - For all images");
            a->setData(ev->pos());
            a->setProperty("type", "FitImage");

            const QIcon insLinkIcon = QIcon(iconDir + "insert-link.png");
            a = menu->addAction(insLinkIcon, "Replace image with link", this, &CTextEdit::onMenuAct);
            a->setData(ev->pos());
            a->setProperty("type", "ReplaceImageWithLink");
            a->setEnabled(!isAnchor);

            const QIcon insImageIcon = QIcon(iconDir + "insert-image.png");
            a = menu->addAction(insImageIcon, "Replace image with thumbnail", this, &CTextEdit::onMenuAct);
            a->setData(ev->pos());
            a->setProperty("type", "ReplaceImageWithThumb");
            a->setEnabled(!isAnchor);
        }

        a = menu->addAction("Open image with external program", this, &CTextEdit::onMenuAct);
        a->setData(ev->pos());
        a->setProperty("type", "OpenImage");
        a->setProperty("path", imgPath);
    }

    if (isAnchor || isImageFormat)
        menu->addSeparator();

    QString imageLink;
    bool clipHasAnchor = false;

    if (clipMime && clipMime->hasText())
    {
        QString text = clip->text();

        if (isHttpLink(text))
        {
            QString fileMime = QMimeDatabase().mimeTypeForFile(text).name();
            if (fileMime.indexOf("image") >= 0)
                imageLink = text;
        }

        if (text.length() <= maxAnchorLength && text.left(1) == "#")
            clipHasAnchor = true;
    }

    bool hasSelection = textCursor().hasSelection();
    int selectionLength = textCursor().selectionEnd() - textCursor().selectionStart();

    const QIcon copyIcon = QIcon(iconDir + "editcopy.png");
    a = menu->addAction(copyIcon, "Copy HTML", this, &CTextEdit::onMenuAct2);
    a->setData("CopyHtml");
    a->setEnabled(hasSelection);

    const QIcon pasteIcon = QIcon(iconDir + "editpaste.png");
    a = menu->addAction(pasteIcon, "Paste as plain text", this, &CTextEdit::onMenuAct2);
    a->setData("PasteAsPlainText");

    const QIcon pasteIcon = QIcon(iconDir + "editpaste.png");
    a = menu->addAction(pasteIcon, "Paste as image", this, &CTextEdit::onMenuAct2);
    a->setData("PasteAsImage");
    a->setProperty("http", imageLink);
    a->setToolTip("Download image and paste");
    a->setEnabled(!imageLink.isEmpty());

    if (mime != "md")
    {
        a = menu->addAction("Line", this, &CTextEdit::onMenuAct2);
        a->setData("InsertLine");
        a->setToolTip("Insert line");
    }

    a = menu->addAction("Current date", this, &CTextEdit::onMenuAct2);
    a->setData("InsertCurrentDate");
    a->setToolTip("Insert current date");

    a = menu->addAction("Merge blocks", this, &CTextEdit::onMenuAct2);
    a->setData("MergeBlocks");
    a->setEnabled(hasSelection);

    a = menu->addAction("Create anchor link", this, &CTextEdit::onMenuAct2);
    a->setData("CreateAnchorLink");
    a->setToolTip("Then copy the link and insert anchor");
    a->setEnabled((selectionLength >= 3 && selectionLength <= maxAnchorLength));

    a = menu->addAction("Insert anchor", this, &CTextEdit::onMenuAct2);
    a->setData("InsertAnchor");
    a->setToolTip("Copy the link first");
    a->setEnabled(clipHasAnchor);

    a = menu->addAction("Delete anchor", this, &CTextEdit::onMenuAct2);
    a->setData("DeleteAnchor");
    a->setProperty("AnchorName", anchorHref.mid(1));
    a->setEnabled(isLocalAnchor);

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
        qDebug() << "insertFromMimeData:  image size:  " << image << image.isNull() << image.width() << image.height();

        if (image.isNull())
        {
            qDebug() << "insertFromMimeData:  Paste image error ";
            emitMessage(Warning, "Paste image error. Maybe format is not supported.\n");
            return;
        }

        QString fileName = QString("image-%1.jpg").arg(time(NULL));
        qDebug() << "insertFromMimeData:  fileName,  imageQuality  =  " << fileName << imageQuality;

        if ( !image.save(pathToPath(fileName, FullPathType), "JPG", imageQuality) )
        {
            qDebug() << "insertFromMimeData:  save image error ";
            emitMessage(Warning, "Save image error.\n" + pathToPath(fileName, FullPathType));
            return;
        }

        imageSizes[fileName] = image.size();

        QTextImageFormat tif;
        tif.setName(pathToPath(fileName, DocPathType));
        int w = image.width();
        int ww = lineWrapColumnOrWidth();
        if (!ww)  ww = width() - LineWrapMargin;
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
                    QString fileName = downloadFile(urlStr, Inet);

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
         QString html;
         QString text = source->text();
         if (source->hasText() && Qt::mightBeRichText(text))  html = text;
         else  html = source->html();
         qDebug() << "CTextEdit::insertFromMimeData:    source->hasHtml: " << html;

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
    QString fileName = downloadFile(urlStr, Local);
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


QString CTextEdit::downloadFile(QString urlStr, int flags)
{
    QTextImageFormat fmt;
    fmt.setProperty(FlagsProp, flags);
    fmt.setName(urlStr);
    emit addResFile(fmt);

    return fmt.name();
}


bool CTextEdit::isHttpLink(QString link)
{
    return link.indexOf("http") == 0;
}


QString CTextEdit::pathToPath(QString path, CTextEdit::PathType type)
{
    QString s;
    if (type == FullPathType)
        s = filesDir + "/" + QFileInfo(path).fileName();
    else if (type == DocPathType)
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

        int ww = lineWrapColumnOrWidth();
        if (!ww)  ww = width() - LineWrapMargin;
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

    QTextCharFormat fmt = cur.charFormat();

    if (type == "SaveLink")
    {
        emit saveAsFile(fmt.anchorHref());
    }
    else if (type == "ReplaceLinkWithImage")
    {
        QString filePath = a->property("path").toString();

        if (isHttpLink(filePath))
            downloadFile(filePath, Image | Inet);

        html = "<img src=\""+ pathToPath(filePath, DocPathType) + "\">";
    }
    else if (type == "MakeImageAsLink")
    {
        QTextImageFormat imgFmt = fmt.toImageFormat();
        imgFmt.setProperty(FlagsProp, Image | Thumb | Clip);
        emit addResFile(imgFmt);

        if (imgFmt.name().isEmpty())  return;

        QString attr;
        if (imgFmt.height() > 0)
            attr = " height=" +  QString::number(imgFmt.height());
        html = "<a href=\"" + fmt.anchorHref() + "\"><img src=\""
                + imgFmt.name() + "\"" + attr + "></a> ";
    }
    else if (type == "CopyLink")
    {
        QClipboard *clip = QGuiApplication::clipboard();
        clip->setText(fmt.anchorHref());
    }

    QTextImageFormat imgFmt = fmt.toImageFormat();

    if (type == "SaveImage")
    {
        emit saveAsFile(imgFmt.name());
    }
    else if (type == "ReplaceImageWithLink")
    {
        html = "<a href=\"" + imgFmt.name() + "\">" + QFileInfo(imgFmt.name()).fileName() + "</a> ";
    }
    else if (type == "ReplaceImageWithThumb")
        html = "<a href=\"" + imgFmt.name() + "\"><img src=\"" + imgFmt.name()
                + "\" height=" + QString::number(thumbHeight) + "></a> ";
    else if (type == "FitImage")
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
    else if (type == "CopyImage")
    {
        QString path = pathToPath(imgFmt.name(), FullPathType);
        QImage image(path);
        qDebug() << "CTextEdit::onMenuAct:   IC:   image:  " << path << image.isNull();
        Q_ASSERT(!image.isNull());
        image.setColorSpace(QColorSpace());  //++ image recognized in insertFromMimeData
        QApplication::clipboard()->setImage(image, QClipboard::Clipboard);
    }
    else if (type == "OpenImage")
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
            cur.setPosition(frag.position()); 
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

    if (s == "CopyHtml")
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
    else if (s == "PasteAsPlainText")
    {
        QClipboard *clip = QApplication::clipboard();
        const QMimeData *mimeData = clip->mimeData();
        if (mimeData->hasText())
        {
            QString text = clip->text();
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
    else if (s == "PasteAsImage")
    {
         QString imageLink = act->property("http").toString();

         downloadFile(imageLink, Image | Inet);

         textCursor().insertHtml("<img src=\"" + pathToPath(imageLink, DocPathType) + "\">");
    }
    else if (s == "InsertLine")
    {
        QTextDocumentFragment fr = QTextDocumentFragment::fromHtml("<hr><br>");
        textCursor().insertFragment(fr);
        ensureCursorVisible();
    }
    else if (s == "InsertCurrentDate")
    {
        QDateTime dt = QDateTime::currentDateTime();
        insertPlainText(dt.toString("d.MM.yyyy "));
    }
    else if (s == "MergeBlocks")
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
    else if (s == "CreateAnchorLink")
    {
        QTextCursor cur = textCursor();
        if (cur.hasSelection())
        {
            QString href = QString::number(time(0));
            cur.beginEditBlock();
            QString link = QString("<a href=\"#") + href + "\">" + cur.selection().toPlainText() + "</a>";
            cur.insertHtml(link);
            cur.endEditBlock();
        }
    }
    else if (s == "InsertAnchor")
    {
        QClipboard *clip = QApplication::clipboard();
        const QMimeData *mimeData = clip->mimeData();
        QString text = clip->text();
        QTextFormat msg;

        if (mimeData->hasText() && text.left(1) == "#")
        {
            QString anchorName = text.mid(1);
            bool isAnchorExists = false;

            QTextBlock bl = document()->begin();

            while (bl.isValid())
            {
                QTextBlock::iterator it;

                for (it = bl.begin(); !(it.atEnd()); ++it)
                {
                    QTextFragment currFrag = it.fragment();

                    if (!currFrag.isValid())
                        continue;

                    QTextCharFormat fmt = currFrag.charFormat();

                    if (!fmt.isAnchor() || !fmt.anchorHref().isEmpty())
                        continue;

                    if (fmt.anchorName() == anchorName)  // anchorNames().value(0) ?
                        isAnchorExists = true;
                }

                bl = bl.next();
            }

            if (isAnchorExists)
            {
                emitMessage(Warning, "Anchor already exists.");
            }
            else
            {
                // &#8203;  - is zero space
                QString s = QString("<a name=\"") + anchorName + "\">&#8203;</a>";  // &nbsp; <br> &#8203;
                qDebug() << "CTextEdit::onMenuAct2:    InsertAnchor:  " << s;

                QTextDocumentFragment frag = QTextDocumentFragment::fromHtml(s);

                QTextCursor cur = textCursor();
                cur.beginEditBlock();
                cur.insertFragment(frag);
                cur.endEditBlock();

                emitMessage(StatusBar, "Anchor inserted (" + anchorName + ").");
            }
        }
        else
        {
            emitMessage(Warning, "No anchor link in the clipboard.");
        }
    }
    else if (s == "DeleteAnchor")
    {
        QString anchorName = act->property("AnchorName").toString();
        qDebug() << "CTextEdit::onMenuAct2:    DeleteAnchor:  anchorName: " << anchorName;
        if (anchorName.isEmpty())  return;

        QTextBlock bl = document()->begin();

        while (bl.isValid())
        {
            QTextBlock::iterator it;

            for (it = bl.begin(); !(it.atEnd()); ++it)
            {
                QTextFragment currFrag = it.fragment();

                if (!currFrag.isValid())
                    continue;

                QTextCharFormat fmt = currFrag.charFormat();

                if (!fmt.isAnchor() || !fmt.anchorHref().isEmpty() || fmt.anchorName() != anchorName)
                    continue;

                qDebug() << "CTextEdit::onMenuAct2:    Deleting anchor:  " << fmt.anchorHref() << fmt.anchorName();

                QTextCursor cur = textCursor();
                cur.setPosition(currFrag.position());
                cur.setPosition(currFrag.position() + currFrag.length(), QTextCursor::KeepAnchor);
                cur.removeSelectedText();

                emitMessage(StatusBar, "Anchor deleted (" + anchorName + ").");

                return;
            }

            bl = bl.next();
        }
    }
    else
        qDebug() << "CTextEdit::onMenuAct2:    Error: unknown act: " << act;
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


bool CTextEdit::secondPassed(QString addr)
{
    time_t t = time(NULL);

    if (!netReqTime.contains(addr) || netReqTime[addr] <= t)
    {
        netReqTime[addr] = t + 1;
        return true;
    }

    return false;
}


// + runs on paste
// type - https://doc.qt.io/qt-5/qtextdocument.html#ResourceType-enum
QVariant CTextEdit::loadResource(int type, const QUrl &name)
{
    qDebug() << "CTextEdit::loadResource:    " << name;

    if (type == QTextDocument::ImageResource)
    {
        QImage image;
        QString fileName;
        QString filePath;
        QString addr = name.toString();
        qDebug() << "CTextEdit::loadResource:    addr  filesDir  " << addr << filesDir;

        if (addr.indexOf(":/") == 0)
            image = QImage(addr);
        else if (isHttpLink(addr))
        {
            if (!isNetworkEnabled)
                image = QImage(":/images/linux/no-network.png");
            else
            {
                if (secondPassed(addr))
                {
                    QString fn = downloadFile(name.toString(), Image | Inet);

                    if (!fn.isEmpty())
                    {
                        qDebug() << "loadResource:   loaded http file=" << fn;
                        fileName = QFileInfo(fn).fileName();
                    }
                }
            }
        }
        else
        {
            qDebug() << "CTextEdit::loadResource:    not isHttpLink";

            // if local addr and not filesDir = for copy from other tab
            if (addr.left(1) == "/" && addr.indexOf(filesDir) < 0)
            {
                if (secondPassed(addr))
                {
                    qDebug() << "CTextEdit::loadResource:    local not filesDir = " << filesDir;
                    QString fn = downloadFile(name.toString(), Image | Local);

                    if (!fn.isEmpty())
                    {
                        qDebug() << "loadResource: local file, image=" << fn;
                        fileName = QFileInfo(fn).fileName();
                    }
                }
            }
            else
            {
                qDebug() << "CTextEdit::loadResource:  local  filesDir";

                fileName = QFileInfo(addr).fileName();
            }
        }

        if (!fileName.isEmpty())
        {
            QByteArray ba;

            filePath = pathToPath(fileName, FullPathType);

            QFile file(filePath);

            if (file.open(QIODevice::ReadOnly))
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

#ifndef QT_NO_DEBUG_OUTPUT

    foreach(QString name, fileList)
        qDebug() << name;

#endif

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
                    QTextImageFormat* pfmt = (QTextImageFormat*)&fmt;
                    fileName = dir + "/" + QFileInfo(pfmt->name()).fileName();
                    fileName.replace(" ", "%20");
                    pfmt->setName(fileName);
                    qDebug() << "setResDir: new image name: " << fileName << currFrag.text() << currFrag.length();

                    cur.setPosition(currFrag.position());
                    cur.setPosition(currFrag.position() + currFrag.length(), QTextCursor::KeepAnchor);
                    cur.setCharFormat(fmt);
                }
            }
        }

        bl = bl.next();
    }
}


void CTextEdit::emitMessage(MessageType type, QString text)
{
    QTextFormat msg;

    msg.setProperty(TypeProp, type);
    msg.setProperty(TextProp, text);

    emit message(msg);
}


void CTextEdit::setImageQuality(int quality)
{
    int q = quality;
    if (quality < -1)  q = -1;
    else if (quality > 100)  q = 100;

    imageQuality = q;
}

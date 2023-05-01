#include "cfindtextpanel.h"
#include <QMessageBox>


CFindTextPanel::CFindTextPanel(QWidget *parent) : QWidget(parent)
{
    hb = new QHBoxLayout();
    hb->setAlignment(Qt::AlignLeft);
    setLayout(hb);  //crashed if later
    hb->setContentsMargins(10, 0, 10, 8);

    edFind = new QLineEdit(this);
    edFind->setPlaceholderText("Find");
    edFind->setFixedWidth(250);

    connect(edFind, SIGNAL(returnPressed()), this, SLOT(onReturnPressed()));

    btPrev = new QToolButton(this);
    btPrev->setFixedWidth(ButtonWidth);
    btPrev->setIconSize(IconSize);

    btNext = new QToolButton(this);
    btNext->setFixedWidth(ButtonWidth);
    btNext->setIconSize(IconSize);

    btClose = new QToolButton(this);
    btClose->setFixedWidth(ButtonWidth);
    btClose->setIconSize(IconSize);

    connect(btPrev, &QToolButton::clicked, this, &CFindTextPanel::findPrev);
    connect(btNext, &QToolButton::clicked, this, &CFindTextPanel::findNext);
    connect(btClose, &QToolButton::clicked, this, &CFindTextPanel::onClose);

    chWholeWords = new QCheckBox("Whole words    ", this);
    chWholeWords->setFixedWidth(120);
    chWholeWords->setChecked(false);
    connect(chWholeWords, SIGNAL(stateChanged(int)), this, SLOT(onWholeWords(int)));

    lbStatus = new QLabel(this);
    lbStatus->setFixedWidth(100);
    lbStatus->setAlignment(Qt::AlignCenter);
    lbStatus->setStyleSheet("QLabel { background-color: #404244; color: #fafafa; }");  // #3c63a7  #308cc6
    lbStatus->setText("End of text");
    lbStatus->setVisible(false);

    hb->addWidget(edFind);
    hb->addWidget(btPrev);
    hb->addWidget(btNext);
    hb->addWidget(chWholeWords);
    hb->addWidget(lbStatus);
    hb->addStretch(1);
    hb->addWidget(btClose, Qt::AlignRight);

    setFocusPolicy(Qt::TabFocus);
    setFocusProxy(edFind);

    teText = nullptr;
    isEndOfText = false;
    findStatus = 0;

}


void CFindTextPanel::setFont(const QFont &font)
{
    edFind->setFont(font);
}


void CFindTextPanel::setPrevIcon(const QIcon &icon)
{
    btPrev->setIcon(icon);
}


void CFindTextPanel::setNextIcon(const QIcon &icon)
{
    btNext->setIcon(icon);
}


void CFindTextPanel::setCloseIcon(const QIcon &icon)
{
    btClose->setIcon(icon);
}


void CFindTextPanel::setFlat(bool flat)
{
    btPrev->setAutoRaise(flat);
    btNext->setAutoRaise(flat);
    btClose->setAutoRaise(flat);

}


void CFindTextPanel::setTextEdit(QTextEdit *te)
{
    teText = te;

    findStatus = 0;
    lbStatus->setVisible(false);
}


void CFindTextPanel::setText(const QString &text)
{
    edFind->setText(text);
}


void CFindTextPanel::findNext()
{
    if (teText && !edFind->text().isEmpty())
    {
        findFlags &= ~QTextDocument::FindBackward;

        if (findStatus == AtEnd)
        {
            teText->moveCursor(QTextCursor::Start);
            findStatus = None;
        }

        if (!teText->find(edFind->text(), findFlags))
        {
            lbStatus->setVisible(true);
            findStatus = AtEnd;
        }
        else
            lbStatus->setVisible(false);
    }
}


void CFindTextPanel::findPrev()
{
    if (teText && !edFind->text().isEmpty())
    {
        findFlags |= QTextDocument::FindBackward;

        if (findStatus == AtBegin)
        {
            teText->moveCursor(QTextCursor::End);
            findStatus = None;
        }

        if (!teText->find(edFind->text(), findFlags))
        {
            lbStatus->setVisible(true);
            findStatus = AtBegin;
        }
        else
            lbStatus->setVisible(false);
    }
}


void CFindTextPanel::onWholeWords(int state)
{
    if (state == Qt::Checked)
        findFlags |= QTextDocument::FindWholeWords;
    else
        findFlags &= ~QTextDocument::FindWholeWords;
}


void CFindTextPanel::onReturnPressed()
{
    findNext();
}


void CFindTextPanel::onClose()
{
    setVisible(false);
    teText->setFocus();
}

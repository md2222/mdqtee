#ifndef CFINDTEXTPANEL_H
#define CFINDTEXTPANEL_H

#include <QWidget>
#include <QTextEdit>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QToolButton>
#include <QLabel>
#include <QCheckBox>


class CFindTextPanel : public QWidget
{
    Q_OBJECT
public:
    explicit CFindTextPanel(QWidget *parent = nullptr);
    void setTextEdit(QTextEdit *te);
    void setFont(const QFont &font);
    void setPrevIcon(const QIcon &icon);
    void setNextIcon(const QIcon &icon);
    void setCloseIcon(const QIcon &icon);
    void setFlat(bool flat);
    void setText(const QString &text);

signals:

public slots:
    void findNext();
    void findPrev();
    void onWholeWords(int state);
    void onReturnPressed();
    void onClose();

private:
    QTextEdit *teText;
    QHBoxLayout* hb;
    QLineEdit* edFind;
    QTextDocument::FindFlags findFlags;
    QToolButton* btPrev;
    QToolButton* btNext;
    QToolButton* btClose;
    QCheckBox* chWholeWords;
    QLabel* lbStatus;
    bool isEndOfText;
    int findStatus;
};

#endif // CFINDTEXTPANEL_H

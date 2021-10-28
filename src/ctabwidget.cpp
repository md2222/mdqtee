#include <QtDebug>
#include "ctabwidget.h"


CTabWidget::CTabWidget(QWidget* parent)
{
    bar = new CTabBar(this);
    bar->setBaseSize(600, 30);
    setTabBar(bar);
}


void CTabWidget::resizeEvent(QResizeEvent *ev)
{
    bar->setWidth(width());
    QTabWidget::resizeEvent(ev);
}



CTabBar::CTabBar(QWidget* parent)
{
    setWidth(size().width());
}


void CTabBar::setWidth(int w)
{
    _size = QSize(w, size().height());
    QTabBar::resize(_size);
}


bool CTabBar::event(QEvent *ev)
{
    if (ev->type() == QEvent::Polish)
        qDebug() << "event:  Polish";
        //cachedHeight = -1;
        //isPolish = true;
    return QTabBar::event(ev);
}


QSize CTabBar::tabSizeHint(int index) const
{
    QSize tabSize = QTabBar::tabSizeHint(index);
    qDebug() << "tabSizeHint:  " << index << tabSize << _size.width() << count();
    if (!_size.width())
        return tabSize;

    int tabCount = count();
    int w = 400;
    if (w * tabCount > _size.width())
        w = qMax(_size.width()/tabCount, 100);
    qDebug() << "tabSizeHint:  new w=" << w;

    return QSize(w, tabSize.height());
}


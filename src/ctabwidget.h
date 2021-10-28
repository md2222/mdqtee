#ifndef CTABWIDGET_H
#define CTABWIDGET_H

#include <QObject>
#include <QWidget>
#include <QTabWidget>
#include <QTabBar>
#include <QEvent>


class CTabBar : public QTabBar
{
    Q_OBJECT
public:
    int maxWidth;
    CTabBar(QWidget* parent);
    void setWidth(int w);

private:
    int cachedHeight;
    QSize _size;
    bool event(QEvent *ev);
    QSize tabSizeHint(int index) const;
};


class CTabWidget : public QTabWidget
{
    Q_OBJECT
public:
    CTabWidget(QWidget* parent);

private:
    CTabBar* bar;
    void resizeEvent(QResizeEvent *ev) override;
};

#endif // CTABWIDGET_H

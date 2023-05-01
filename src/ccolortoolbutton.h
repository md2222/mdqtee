#ifndef CCOLORTOOLBUTTON_H
#define CCOLORTOOLBUTTON_H

#include <QWidget>
#include <QToolButton>
#include <QMenu>
#include <QColor>


class CColorToolButton : public QToolButton
{
    Q_OBJECT
public:
    const int PixmapWidth = 18;
    const int PixmapHeight = 18;
    const QString CustomColorCode = "#000001";

    CColorToolButton(QWidget* parent);
    void setCustomColors(QList<QColor> colorList);
    void setCustomColors(QStringList colorNameList);
    QList<QColor> getCustomColors();
    QStringList getCustomColorNames();
    void setCurrentColor(QColor color);

signals:
    void selected(QColor);

private slots:
    void onClick();


private:
    QMenu* colorMenu;
    QList<QColor> customColors;
    QColor currentColor;
    void updateColorMenu();
    void onColorMenuAct();
    QColor getCustomColor();
};

#endif // CCOLORTOOLBUTTON_H

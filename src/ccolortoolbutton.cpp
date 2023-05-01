#include "ccolortoolbutton.h"
#include <QColorDialog>
#include <QtDebug>


CColorToolButton::CColorToolButton(QWidget* parent)
{
    setPopupMode(QToolButton::MenuButtonPopup);

    colorMenu = new QMenu(parent);
    setMenu(colorMenu);

    connect(this, &QToolButton::clicked, this, &CColorToolButton::onClick);
}


void CColorToolButton::setCustomColors(QList<QColor> colorList)
{
    customColors = colorList;
    updateColorMenu();
}


void CColorToolButton::setCustomColors(QStringList colorNameList)
{
    customColors.clear();
    foreach (QString colorName, colorNameList)
        customColors.append(QColor(colorName));
    updateColorMenu();
}


QList<QColor> CColorToolButton::getCustomColors()
{
    return customColors;
}


QStringList CColorToolButton::getCustomColorNames()
{
    QStringList sl;
    foreach (QColor color, customColors)
        sl << color.name();
    return sl;
}


void CColorToolButton::setCurrentColor(QColor color)
{
    QPixmap pix(PixmapWidth, PixmapHeight);
    pix.fill(color);
    setIcon(QIcon(pix));
    currentColor = color;
}


void CColorToolButton::updateColorMenu()
{
    QPixmap pix(PixmapWidth, PixmapHeight);
    QAction* a;

    colorMenu->clear();

    for (int i = 0; i < customColors.size(); i++)
    {
        pix.fill(customColors[i]);
        a = new QAction(QIcon(pix), QString::number(i + 1), this);
        a->setData((QColor)customColors[i]);
        connect(a, &QAction::triggered, this, &CColorToolButton::onColorMenuAct);
        colorMenu->addAction(a);
    }

    colorMenu->addSeparator();

    a = new QAction("Custom color", this);
    a->setData(QColor(CustomColorCode));
    connect(a, &QAction::triggered, this, &CColorToolButton::onColorMenuAct);
    colorMenu->addAction(a);

}


void CColorToolButton::onColorMenuAct()
{
    QAction* a = qobject_cast<QAction*>(sender());
    QColor col =  qvariant_cast<QColor>(a->data());

    if (col.name() == CustomColorCode)
    {
        col = getCustomColor();
        if (!col.isValid())  return;
    }

    setCurrentColor(col);

    emit selected(col);
}


QColor CColorToolButton::getCustomColor()
{
    QColorDialog cd(this);
    cd.setOptions(QColorDialog::DontUseNativeDialog);
    cd.setCurrentColor(currentColor);

    QColor col;

    int i = 0;
    for (; i < cd.customCount() && i < customColors.size(); i++)
        cd.setCustomColor(i, customColors[i]);
    int cc = i;

    if (cd.exec())
    {
        col = cd.selectedColor();

        if (col.isValid())
        {
            customColors.clear();
            for (int i = 0; i < cc; i++)
                customColors.append(cd.customColor(i));
            updateColorMenu();
        }
    }

    return col;
}


void CColorToolButton::onClick()
{
    emit selected(currentColor);
}

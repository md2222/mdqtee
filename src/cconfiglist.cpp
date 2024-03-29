#include <QDebug>
#include <QFontDialog>
#include <QFileDialog>
#include <QFontInfo>
#include <QCheckBox>
#include <QComboBox>
#include <QMessageBox>
#include "cconfiglist.h"

enum { NameCol, ValueCol, CommentCol };

CConfigList::CConfigList(QWidget *parent)
    : QTableWidget(parent)
{
    setColumnCount(3);
    setColumnWidth(NameCol, InitColumnWidth);
    setColumnWidth(ValueCol, InitColumnWidth);
    setColumnWidth(CommentCol, InitColumnWidth);

    setHorizontalHeaderLabels(QStringList() << "Name" << "Value" << "Comment");

    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);

    horizontalHeader()->setMinimumSectionSize(MinimumSectionSize);
    horizontalHeader()->setStretchLastSection(true);

    verticalHeader()->setDefaultSectionSize(verticalHeader()->defaultSectionSize() - 2);
    verticalHeader()->setVisible(false);

    setStyleSheet("QTableView::item {}");

    connect(itemDelegate(), SIGNAL(closeEditor(QWidget*, QAbstractItemDelegate::EndEditHint)), this, SLOT(onCloseEditor(QWidget*, QAbstractItemDelegate::EndEditHint)));
}


QHeaderView* CConfigList::header()
{
    return horizontalHeader();
}


const CConfigList::ParamList& CConfigList::params()
{
    return pars;
}


void CConfigList::clear()
{
    pars.clear();
    QTableWidget::clearContents();
    setRowCount(0);
}


void CConfigList::setParams(ParamList &p)
{
    clear();

    for (int i = 0; i < p.size(); i++)
        addParam(p[i]);
}


void CConfigList::addParam(Param &param)
{
    qDebug() << "addParam:  " << param.name << param.value;
    pars.append(param);

    int i = rowCount();
    insertRow(i);

    QTableWidgetItem *item = new QTableWidgetItem(param.label);
    setItem(i, NameCol, item);

    QWidget* widget = 0;

    if (param.type == TypeNumber)
    {
        item = new QTableWidgetItem(QString::number(param.value.toInt()));
    }
    else if (param.type == TypeFloat)
    {
        item = new QTableWidgetItem(QString::number(param.value.toFloat()));
    }
    else if (param.type == TypeBool)
    {
        item = new QTableWidgetItem("", TypeBool);

        QCheckBox *ch = new QCheckBox(this);
        ch->setStyleSheet("margin-left: 5px;");
        ch->setProperty("row", i);
        ch->setChecked(param.value.toBool());
        connect(ch, &QCheckBox::stateChanged, this, &CConfigList::onCheckBox);
        widget = ch;
    }
    else if (param.type == TypeList)
    {
        item = new QTableWidgetItem("", TypeList);

        if (param.list.count() > 0)
        {
            QComboBox* cb = new QComboBox(this);
            cb->setProperty("row", i);
            cb->setEditable(true);

            foreach (QString str, param.list)
                cb->addItem(str);
            cb->setEditText(param.value.toString());
            connect(cb, &QComboBox::currentTextChanged, this, &CConfigList::onComboBox);
            widget = cb;
        }
    }
    else if (param.type == TypeFont)
    {
        QFont font; font.fromString(param.value.toString());
        QString s;
        if (font.fromString(param.value.toString()))
            s = font.family() + ", " + QString::number(font.pointSizeF()) + ", " + font.styleName();
        else
            s = "[Invalid font]";
        qDebug() << "addParam:  " << s;
        item = new QTableWidgetItem(s, TypeFont);
    }
    else if (param.type == TypeFile)
    {
        item = new QTableWidgetItem(param.value.toString(), TypeFile);
    }
    else if (param.type == TypeDir)
    {
        item = new QTableWidgetItem(param.value.toString(), TypeDir);
    }
    else
    {
        item = new QTableWidgetItem(param.value.toString());
    }

    setItem(i, ValueCol, item);
    if (widget)
        setCellWidget(i, ValueCol, widget);

    item = new QTableWidgetItem(param.comment);
    setItem(i, CommentCol, item);
}


void CConfigList::editCurrentRow(int col)
{
    QTableWidgetItem* item = currentItem();

    if (item->column() == col)
    {
        int i = item->row();

        if (item->type() == TypeFont)
        {
            bool ok;
            QFont font = QFontDialog::getFont(&ok, pars[i].value.value<QFont>(), this, "Default text font", QFontDialog::DontUseNativeDialog);
            if (ok)
            {
                pars[i].value = font.toString();
                pars[i].isEdited = true;
                //QFontInfo fi(font); // lie
                QString s = font.family() + ", " + QString::number(font.pointSizeF()) + ", " + font.styleName();
                item->setText(s);
            }
        }
        else if (item->type() == TypeFile)
        {
            QFileDialog dlg(this, tr("Select file"));
            if (!isUseNatFileDlg)
                dlg.setOption(QFileDialog::DontUseNativeDialog);
            //dlg.setFilter(QDir::Hidden | QDir::NoDotAndDotDot);
            dlg.setNameFilters(QStringList()
                << "All files (*)"
            );
            dlg.setAcceptMode(QFileDialog::AcceptOpen);
            dlg.setFileMode(QFileDialog::ExistingFile);
            dlg.setViewMode(QFileDialog::Detail);
            dlg.resize(fileDialogSize);

            if (dlg.exec() != QDialog::Accepted)
                return;

            QString fileName = dlg.selectedFiles().first();
            fileDialogSize = dlg.size();

            pars[i].value = fileName;
            pars[i].isEdited = true;
            item->setText(fileName);
        }
        else if (item->type() == TypeDir)
        {
            QFileDialog dlg(this, tr("Select directory"));
            if (!isUseNatFileDlg)
                dlg.setOption(QFileDialog::DontUseNativeDialog);
            //dlg.setFilter(QDir::Hidden | QDir::NoDotAndDotDot);
            dlg.setFileMode(QFileDialog::DirectoryOnly);
            dlg.resize(fileDialogSize);

            if (dlg.exec() != QDialog::Accepted)
                return;

            QString dirPath = dlg.selectedFiles().first();
            fileDialogSize = dlg.size();

            if (dirPath.endsWith('/'))
                dirPath.chop(1);

            pars[i].value = dirPath;
            pars[i].isEdited = true;
            item->setText(dirPath);
        }
        else if (item->type() == TypeBool)
        {
            ;
        }
        else
        {
            item->setFlags(item->flags () | Qt::ItemIsEditable);
            editItem(item);
            item->setFlags(item->flags () & ~Qt::ItemIsEditable);
            if (pars[i].isValid)
                editStr = item->text();
        }
    }
}


void CConfigList::onCheckBox(int state)
{
    QCheckBox* ch = qobject_cast<QCheckBox*>(sender());
    int i = ch->property("row").toInt();
    qDebug() << "onCheckBox:  " << i << ch->isChecked();
    pars[i].value = ch->isChecked();
    pars[i].isEdited = true;
}


void CConfigList::onComboBox(QString text)
{
    QComboBox* cb = qobject_cast<QComboBox*>(sender());
    int i = cb->property("row").toInt();
    qDebug() << "onComboBox:  " << i << cb->currentText();
    pars[i].value = cb->currentText();
    pars[i].isEdited = true;
}


void CConfigList::onCloseEditor(QWidget *editor, QAbstractItemDelegate::EndEditHint hint)
{
    QString text = currentItem()->text().trimmed();
    int i = currentRow();
    qDebug() << "onCloseEditor:  " << i << text;
    QString err;

    if (hint == QAbstractItemDelegate::RevertModelCache)
    {
        //restore value on Esc
        currentItem()->setText(editStr);
        editStr = "";
        pars[i].isValid = true;
        return;
    }

    if (pars[i].type == TypeNumber)
    {
        bool ok;
        int n = text.toInt(&ok);
        if (!ok)
            err = "Value must be integer.        \n" + pars[i].label;
        else
            pars[i].value = text.toInt();
    }
    else if (pars[i].type == TypeFloat)
    {
        bool ok;
        float n = text.replace(',', '.').toFloat(&ok);
        if (!ok)
            err = "Value must be float.        \n" + pars[i].label;
        else
            pars[i].value = text.toFloat();
    }
    else if (pars[i].type == TypeDir && !text.isEmpty())
    {
        QDir dir(text);
        if (!dir.exists())
            err = "Directory not exists.        \n" + pars[i].label;
        else
            pars[i].value = text;
    }
    else
        pars[i].value = text;

    if (!err.isEmpty())
    {
        QMessageBox::warning(this, "Options", err, QMessageBox::Ok);
        pars[i].isValid = false;
        editCurrentRow(1);
        return;
    }

    pars[i].isValid = true;
    pars[i].isEdited = true;
}


void CConfigList::setUseNatFileDlg(bool native)
{
    isUseNatFileDlg = native;
}


void CConfigList::setFileDialogSize(QSize &size)
{
    fileDialogSize = size;
}


void CConfigList::loadSettings(ParamList &params, const QSettings &set)
{
    for (int i = 0; i < params.size(); i++)
    {
        QVariant value = set.value(params[i].name, params[i].defValue);
        if (value.isNull())  continue;

        if (params[i].type == CConfigList::TypeNumber)
            params[i].value = value.toInt();
        else if (params[i].type == CConfigList::TypeFloat)
            params[i].value = value.toFloat();
        else if (params[i].type == CConfigList::TypeBool)
            params[i].value = value.toBool();
        else
            params[i].value = value.toString();
    }
}


void CConfigList::unloadSettings(const ParamList &params, QSettings &set)
{
    for (int i = 0; i < params.count(); i++)
    {
        QVariant value = params[i].value;

        if (value.isNull())
            set.setValue(params[i].name, params[i].defValue);
        else if (params[i].isEdited)
            set.setValue(params[i].name, value);
    }
}

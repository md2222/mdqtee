#ifndef CCONFIGDIALOG_H
#define CCONFIGDIALOG_H

#include <QDialog>
#include <QSettings>
#include <QList>
#include <QStringList>
#include "cconfiglist.h"


namespace Ui {
class CConfigDialog;
}


class CConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CConfigDialog(QWidget *parent = 0);
    ~CConfigDialog();
    void setUseNativeFileDlg(bool native);
    void setParams(CConfigList::ParamList &p);
    CConfigList* list();
    QByteArray headerState();
    void setHeaderState(QByteArray state);
    const CConfigList::ParamList& params();

public slots:
    void onConfDoubleClick(const QModelIndex& index);

private:
    Ui::CConfigDialog *ui;
    CConfigList* conf;
};


extern CConfigDialog* winConf;

#endif // CCONFIGDIALOG_H

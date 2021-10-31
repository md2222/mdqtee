#include <QDebug>
#include "textedit.h"
#include "cconfigdialog.h"
#include "ui_cconfigdialog.h"


CConfigDialog* winConf;


CConfigDialog::CConfigDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CConfigDialog)
{
    ui->setupUi(this);

    setLayout(ui->vl);

    conf = new CConfigList(this);
    conf->fileDialogSize = fileDialogSize;

    ui->buttonBox->setStandardButtons(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    ui->vl->addWidget(conf);
    ui->vl->addWidget(ui->buttonBox);

    connect(conf, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(onConfDoubleClick(QModelIndex)));
}


CConfigDialog::~CConfigDialog()
{
    delete ui;
}


void CConfigDialog::onConfDoubleClick(const QModelIndex& index)
{
    conf->editCurrentRow(1);
}


void CConfigDialog::setUseNativeFileDlg(bool native)
{
     conf->isUseNatFileDlg = native;
}


void CConfigDialog::setParams(QVector<PARAM_STRUCT> &p)
{
    conf->clear();
    conf->addParams(p);
}


QVector<PARAM_STRUCT>& CConfigDialog::params()
{
    return conf->params();
}


QByteArray CConfigDialog::headerState()
{
    return conf->header()->saveState();
}


void CConfigDialog::setHeaderState(QByteArray state)
{
    if (!state.isEmpty())
        conf->header()->restoreState(state);
}


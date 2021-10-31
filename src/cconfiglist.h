#ifndef CCONFIGLIST_H
#define CCONFIGLIST_H

#include <QTableWidget>
#include <QHeaderView>


struct PARAM_STRUCT
{
    QString label;
    QString name;
    QVariant value;
    QVariant defValue;
    QString strValue;
    QStringList list;
    int type = 0;
    QString comment;
    bool isEdited = false;
    bool isValid = true;
};


class CConfigList : public QTableWidget
{
    Q_OBJECT

public:
    bool isEdited = false;
    bool isUseNatFileDlg = true;
    QSize fileDialogSize = QSize(700, 600);
    CConfigList(QWidget *parent);
    void addParam(PARAM_STRUCT &param);
    void addParams(QVector<PARAM_STRUCT> &p);
    void editCurrentRow(int col);
    void clear();
    QVector<PARAM_STRUCT>& params();
    QHeaderView* header();

    enum {
        TypeString = 1,
        TypeNumber = 3,
        TypeFloat = 4,
        TypeBool = 7,
        TypeList = 9,
        TypeFont = 10,
        TypeFile = 11,
        TypeDir = 12
    };

private slots:
    void onCheckBox(int state);
    void onComboBox(QString text);
    void onCloseEditor(QWidget *editor, QAbstractItemDelegate::EndEditHint hint);

private:
    QString editStr;
    QVector<PARAM_STRUCT> pars;
};


#endif // CCONFIGLIST_H

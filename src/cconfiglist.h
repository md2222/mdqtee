#ifndef CCONFIGLIST_H
#define CCONFIGLIST_H

#include <QTableWidget>
#include <QHeaderView>
#include <QSettings>



class CConfigList : public QTableWidget
{
    Q_OBJECT

public:
    struct Param
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

    typedef QVector<Param> ParamList;

    const int InitColumnWidth = 200;
    const int MinimumSectionSize = 50;

    //bool isEdited = false;
    CConfigList(QWidget *parent);
    void addParam(Param &param);
    void setParams(ParamList &p);
    void editCurrentRow(int col);
    void clear();
    const ParamList& params();
    QHeaderView* header();
    void setUseNatFileDlg(bool native = true);
    void setFileDialogSize(QSize &size);
    static void loadSettings(ParamList &params, const QSettings &set);
    static void unloadSettings(const ParamList &params, QSettings &set);

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
    enum { NameCol, ValueCol, CommentCol };

    bool isUseNatFileDlg = true;
    QSize fileDialogSize = QSize(700, 600);
    QString editStr;
    ParamList pars;
};


#endif // CCONFIGLIST_H

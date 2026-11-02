#ifndef FINDDIALOG_H
#define FINDDIALOG_H

#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui {
class FindDialog;
}
QT_END_NAMESPACE

class FindDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FindDialog(QWidget *parent = nullptr);
    ~FindDialog() override;

    void setFindText(const QString &text);

signals:
    void findRequested(const QString &text, bool caseSensitive,
                       bool backward, bool wrap);

private:
    Ui::FindDialog *ui;
};

#endif // FINDDIALOG_H

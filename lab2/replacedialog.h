#ifndef REPLACEDIALOG_H
#define REPLACEDIALOG_H

#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui {
class ReplaceDialog;
}
QT_END_NAMESPACE

class ReplaceDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ReplaceDialog(QWidget *parent = nullptr);
    ~ReplaceDialog() override;

    void setFindText(const QString &text);

signals:
    void findRequested(const QString &text, bool caseSensitive,
                       bool backward, bool wrap);
    void replaceRequested(const QString &findText, const QString &replaceText,
                          bool caseSensitive, bool backward, bool wrap);
    void replaceAllRequested(const QString &findText, const QString &replaceText,
                             bool caseSensitive, bool wrap);

private:
    Ui::ReplaceDialog *ui;
};

#endif // REPLACEDIALOG_H

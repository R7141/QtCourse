#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QString>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class QCloseEvent;
class QTimer;
class FindDialog;
class ReplaceDialog;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void newFile();
    void openFile();
    void updateWindowTitle();
    void updateActions();
    void updateStatusBar();
    void updateWordWrap(bool enabled);
    void chooseFont();
    void chooseFontColor();
    void chooseTextBackground();
    void chooseEditorBackground();
    void showFindDialog();
    void showReplaceDialog();
    void findText(const QString &text, bool caseSensitive,
                  bool backward, bool wrap);
    void replaceText(const QString &findText, const QString &replaceText,
                     bool caseSensitive, bool wrap);
    void replaceAllText(const QString &findText, const QString &replaceText,
                        bool caseSensitive, bool wrap);
    void showAboutDialog();
    void autoSave();

private:
    bool saveFile();
    bool saveFileAs();
    bool saveToFile(const QString &fileName, bool showError = true);
    bool maybeSave();
    void setCurrentFile(const QString &fileName);

    Ui::MainWindow *ui;

    QLabel statusCursorLabel;
    QLabel statusLabel;
    QString currentFilePath;
    FindDialog *findDialog = nullptr;
    ReplaceDialog *replaceDialog = nullptr;
    QTimer *autoSaveTimer = nullptr;
};
#endif // MAINWINDOW_H

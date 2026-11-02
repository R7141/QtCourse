#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "aboutdialog.h"
#include "codeeditor.h"
#include "finddialog.h"
#include "replacedialog.h"

#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QColorDialog>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDialog>
#include <QMessageBox>
#include <QPalette>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextStream>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->actionShowLineNumbers, &QAction::toggled,
            ui->plainTextEdit, &CodeEditor::setLineNumbersVisible);
    connect(ui->actionWordWrap, &QAction::toggled,
            this, &MainWindow::updateWordWrap);
    connect(ui->actionToolBar, &QAction::toggled, this, [this](bool visible) {
        ui->toolBar->setVisible(visible);
    });
    connect(ui->actionStatusBar, &QAction::toggled, this, [this](bool visible) {
        ui->statusbar->setVisible(visible);
    });

    connect(ui->actionFont, &QAction::triggered, this, &MainWindow::chooseFont);
    connect(ui->actionFontColor, &QAction::triggered,
            this, &MainWindow::chooseFontColor);
    connect(ui->actionTextBackground, &QAction::triggered,
            this, &MainWindow::chooseTextBackground);
    connect(ui->actionEditorBackgroundColor, &QAction::triggered,
            this, &MainWindow::chooseEditorBackground);
    connect(ui->action_Find, &QAction::triggered,
            this, &MainWindow::showFindDialog);
    connect(ui->actionReplace, &QAction::triggered,
            this, &MainWindow::showReplaceDialog);

    findDialog = new FindDialog(this);
    replaceDialog = new ReplaceDialog(this);
    connect(findDialog, &FindDialog::findRequested,
            this, &MainWindow::findText);
    connect(replaceDialog, &ReplaceDialog::findRequested,
            this, &MainWindow::findText);
    connect(replaceDialog, &ReplaceDialog::replaceRequested,
            this, &MainWindow::replaceText);
    connect(replaceDialog, &ReplaceDialog::replaceAllRequested,
            this, &MainWindow::replaceAllText);
    connect(ui->actionAbout, &QAction::triggered,
            this, &MainWindow::showAboutDialog);

    autoSaveTimer = new QTimer(this);
    autoSaveTimer->setInterval(60 * 1000);
    connect(autoSaveTimer, &QTimer::timeout, this, &MainWindow::autoSave);
    autoSaveTimer->start();

    connect(ui->actionNew, &QAction::triggered, this, &MainWindow::newFile);
    connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::openFile);
    connect(ui->actionSave, &QAction::triggered, this, &MainWindow::saveFile);
    connect(ui->actionSaveAs, &QAction::triggered, this, &MainWindow::saveFileAs);
    connect(ui->actionExit, &QAction::triggered, this, &MainWindow::close);
    connect(ui->plainTextEdit->document(), &QTextDocument::modificationChanged,
            this, &MainWindow::updateWindowTitle);

    connect(ui->actionUndo, &QAction::triggered,
            ui->plainTextEdit, &QPlainTextEdit::undo);
    connect(ui->actionRedo, &QAction::triggered,
            ui->plainTextEdit, &QPlainTextEdit::redo);
    connect(ui->actionCut, &QAction::triggered,
            ui->plainTextEdit, &QPlainTextEdit::cut);
    connect(ui->actionCopy, &QAction::triggered,
            ui->plainTextEdit, &QPlainTextEdit::copy);
    connect(ui->actionPaste, &QAction::triggered,
            ui->plainTextEdit, &QPlainTextEdit::paste);
    connect(ui->actionAll, &QAction::triggered,
            ui->plainTextEdit, &QPlainTextEdit::selectAll);

    connect(ui->plainTextEdit, &QPlainTextEdit::copyAvailable,
            this, &MainWindow::updateActions);
    connect(ui->plainTextEdit, &QPlainTextEdit::undoAvailable,
            this, &MainWindow::updateActions);
    connect(ui->plainTextEdit, &QPlainTextEdit::redoAvailable,
            this, &MainWindow::updateActions);
    connect(ui->plainTextEdit, &QPlainTextEdit::cursorPositionChanged,
            this, &MainWindow::updateActions);
    connect(QApplication::clipboard(), &QClipboard::dataChanged,
            this, &MainWindow::updateActions);
    connect(ui->plainTextEdit, &QPlainTextEdit::textChanged,
            this, &MainWindow::updateStatusBar);
    connect(ui->plainTextEdit, &QPlainTextEdit::cursorPositionChanged,
            this, &MainWindow::updateStatusBar);

    statusLabel.setMaximumWidth(150);
    statusLabel.setText("length:"+QString::number(0)+" lines:"+QString::number(1));
    ui->statusbar->addPermanentWidget(&statusLabel);

    statusCursorLabel.setMaximumWidth(150);
    statusCursorLabel.setText("Ln:"+QString::number(0)+" Col:"+QString::number(1));
    ui->statusbar->addPermanentWidget(&statusCursorLabel);

    QLabel *author=new QLabel(ui->statusbar);
    author->setText(tr("E"));
    ui->statusbar->addPermanentWidget(author);

    setCurrentFile(QString());
    updateActions();
    updateWordWrap(ui->actionWordWrap->isChecked());
    updateStatusBar();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (maybeSave())
        event->accept();
    else
        event->ignore();
}

void MainWindow::newFile()
{
    if (!maybeSave())
        return;

    ui->plainTextEdit->clear();
    ui->plainTextEdit->document()->setModified(false);
    setCurrentFile(QString());
    updateActions();
    updateStatusBar();
    ui->plainTextEdit->setFocus();
}

void MainWindow::openFile()
{
    const QString startPath = currentFilePath.isEmpty()
            ? QDir::homePath()
            : QFileInfo(currentFilePath).absolutePath();

    const QString fileName = QFileDialog::getOpenFileName(
                this, tr("打开文件"), startPath,
                tr("文本文件 (*.txt);;C++ 文件 (*.cpp *.h);;所有文件 (*.*)"));
    if (fileName.isEmpty())
        return;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("打开文件"),
                             tr("无法打开文件：\n%1\n\n%2")
                             .arg(fileName, file.errorString()));
        return;
    }

    QTextStream stream(&file);
    stream.setCodec("UTF-8");
    const QString content = stream.readAll();
    if (stream.status() != QTextStream::Ok) {
        QMessageBox::warning(this, tr("打开文件"),
                             tr("读取文件失败：\n%1").arg(fileName));
        return;
    }

    if (!maybeSave())
        return;

    ui->plainTextEdit->setPlainText(content);
    ui->plainTextEdit->document()->setModified(false);
    setCurrentFile(fileName);
    updateActions();
    updateStatusBar();
    ui->plainTextEdit->setFocus();
}

bool MainWindow::saveFile()
{
    if (currentFilePath.isEmpty())
        return saveFileAs();

    return saveToFile(currentFilePath);
}

bool MainWindow::saveFileAs()
{
    const QString suggestedName = currentFilePath.isEmpty()
            ? QDir::homePath() + QStringLiteral("/未命名.txt")
            : currentFilePath;

    const QString fileName = QFileDialog::getSaveFileName(
                this, tr("另存为"), suggestedName,
                tr("文本文件 (*.txt);;所有文件 (*.*)"));
    if (fileName.isEmpty())
        return false;

    if (!saveToFile(fileName))
        return false;

    setCurrentFile(fileName);
    return true;
}

bool MainWindow::saveToFile(const QString &fileName, bool showError)
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        if (showError) {
            QMessageBox::warning(this, tr("保存文件"),
                                 tr("无法保存文件：\n%1\n\n%2")
                                 .arg(fileName, file.errorString()));
        }
        return false;
    }

    QTextStream stream(&file);
    stream.setCodec("UTF-8");
    stream << ui->plainTextEdit->toPlainText();
    stream.flush();

    const bool writeOk = stream.status() == QTextStream::Ok
            && file.error() == QFile::NoError;
    const QString errorString = file.errorString();
    file.close();

    if (!writeOk) {
        if (showError) {
            QMessageBox::warning(this, tr("保存文件"),
                                 tr("写入文件失败：\n%1\n\n%2")
                                 .arg(fileName, errorString));
        }
        return false;
    }

    ui->plainTextEdit->document()->setModified(false);
    statusBar()->showMessage(tr("文件已保存：%1")
                             .arg(QFileInfo(fileName).fileName()), 2000);
    return true;
}

bool MainWindow::maybeSave()
{
    if (!ui->plainTextEdit->document()->isModified())
        return true;

    const QString displayName = currentFilePath.isEmpty()
            ? tr("未命名")
            : QFileInfo(currentFilePath).fileName();

    const QMessageBox::StandardButton ret = QMessageBox::warning(
                this, tr("简单文本编辑器"),
                tr("文档“%1”已修改，是否保存更改？").arg(displayName),
                QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
                QMessageBox::Save);

    if (ret == QMessageBox::Save)
        return saveFile();
    if (ret == QMessageBox::Cancel)
        return false;
    return true;
}

void MainWindow::setCurrentFile(const QString &fileName)
{
    currentFilePath = fileName;
    updateWindowTitle();
}

void MainWindow::updateWindowTitle()
{
    const QString displayName = currentFilePath.isEmpty()
            ? tr("未命名")
            : QFileInfo(currentFilePath).fileName();

    setWindowTitle(tr("%1[*] - 简单文本编辑器").arg(displayName));
    setWindowModified(ui->plainTextEdit->document()->isModified());
}

void MainWindow::updateActions()
{
    const bool hasSelection = ui->plainTextEdit->textCursor().hasSelection();

    ui->actionCut->setEnabled(hasSelection);
    ui->actionCopy->setEnabled(hasSelection);
    ui->actionPaste->setEnabled(ui->plainTextEdit->canPaste());
    ui->actionUndo->setEnabled(ui->plainTextEdit->document()->isUndoAvailable());
    ui->actionRedo->setEnabled(ui->plainTextEdit->document()->isRedoAvailable());
}

void MainWindow::updateStatusBar()
{
    const QTextDocument *document = ui->plainTextEdit->document();
    const QTextCursor cursor = ui->plainTextEdit->textCursor();

    statusLabel.setText(tr("length:%1 lines:%2")
                        .arg(document->characterCount() - 1)
                        .arg(document->blockCount()));
    statusCursorLabel.setText(tr("Ln:%1 Col:%2")
                              .arg(cursor.blockNumber() + 1)
                              .arg(cursor.positionInBlock() + 1));
}

void MainWindow::updateWordWrap(bool enabled)
{
    ui->plainTextEdit->setLineWrapMode(enabled
                                       ? QPlainTextEdit::WidgetWidth
                                       : QPlainTextEdit::NoWrap);
}

void MainWindow::chooseFont()
{
    bool ok = false;
    const QFont font = QFontDialog::getFont(
                &ok, ui->plainTextEdit->font(), this, tr("选择字体"));
    if (!ok)
        return;

    ui->plainTextEdit->document()->setDefaultFont(font);
    ui->plainTextEdit->setFont(font);
    ui->plainTextEdit->refreshLineNumberArea();
}

void MainWindow::chooseFontColor()
{
    const QColor color = QColorDialog::getColor(
                ui->plainTextEdit->palette().color(QPalette::Text),
                this, tr("字体颜色"));
    if (!color.isValid())
        return;

    QTextCharFormat format;
    format.setForeground(color);

    QTextCursor cursor = ui->plainTextEdit->textCursor();
    if (cursor.hasSelection())
        cursor.mergeCharFormat(format);
    else
        ui->plainTextEdit->mergeCurrentCharFormat(format);
}

void MainWindow::chooseTextBackground()
{
    const QColor color = QColorDialog::getColor(
                Qt::yellow, this, tr("文字背景色"));
    if (!color.isValid())
        return;

    QTextCharFormat format;
    format.setBackground(color);

    QTextCursor cursor = ui->plainTextEdit->textCursor();
    if (cursor.hasSelection())
        cursor.mergeCharFormat(format);
    else
        ui->plainTextEdit->mergeCurrentCharFormat(format);
}

void MainWindow::chooseEditorBackground()
{
    const QColor color = QColorDialog::getColor(
                ui->plainTextEdit->palette().color(QPalette::Base),
                this, tr("编辑器背景色"));
    if (!color.isValid())
        return;

    QPalette palette = ui->plainTextEdit->palette();
    palette.setColor(QPalette::Base, color);
    ui->plainTextEdit->setPalette(palette);
    ui->plainTextEdit->viewport()->setPalette(palette);
}

void MainWindow::showFindDialog()
{
    QString selected = ui->plainTextEdit->textCursor().selectedText();
    selected.replace(QChar::ParagraphSeparator, QLatin1Char('\n'));

    findDialog->setFindText(selected);
    findDialog->show();
    findDialog->raise();
    findDialog->activateWindow();
}

void MainWindow::showReplaceDialog()
{
    QString selected = ui->plainTextEdit->textCursor().selectedText();
    selected.replace(QChar::ParagraphSeparator, QLatin1Char('\n'));

    replaceDialog->setFindText(selected);
    replaceDialog->show();
    replaceDialog->raise();
    replaceDialog->activateWindow();
}

void MainWindow::findText(const QString &text, bool caseSensitive,
                          bool backward, bool wrap)
{
    if (text.isEmpty()) {
        QMessageBox::information(this, tr("查找"), tr("请输入要查找的内容。"));
        return;
    }

    QTextDocument::FindFlags flags;
    if (caseSensitive)
        flags |= QTextDocument::FindCaseSensitively;
    if (backward)
        flags |= QTextDocument::FindBackward;

    QTextCursor cursor = ui->plainTextEdit->textCursor();
    if (cursor.hasSelection()) {
        cursor.setPosition(backward ? cursor.selectionStart()
                                    : cursor.selectionEnd());
    }

    QTextCursor found = ui->plainTextEdit->document()->find(text, cursor, flags);
    if (found.isNull() && wrap) {
        QTextCursor start(ui->plainTextEdit->document());
        if (backward)
            start.movePosition(QTextCursor::End);

        found = ui->plainTextEdit->document()->find(text, start, flags);
    }

    if (found.isNull()) {
        QMessageBox::information(this, tr("查找"),
                                 tr("未找到“%1”。").arg(text));
        return;
    }

    ui->plainTextEdit->setTextCursor(found);
    ui->plainTextEdit->ensureCursorVisible();
    statusBar()->showMessage(tr("已找到“%1”").arg(text), 2000);
}

void MainWindow::replaceText(const QString &findText,
                             const QString &replaceText,
                             bool caseSensitive, bool wrap)
{
    if (findText.isEmpty()) {
        QMessageBox::information(this, tr("替换"), tr("请输入要查找的内容。"));
        return;
    }

    QTextCursor cursor = ui->plainTextEdit->textCursor();
    if (cursor.hasSelection()) {
        const Qt::CaseSensitivity sensitivity = caseSensitive
                ? Qt::CaseSensitive
                : Qt::CaseInsensitive;

        if (QString::compare(cursor.selectedText(), findText, sensitivity) == 0) {
            cursor.insertText(replaceText);
            ui->plainTextEdit->setTextCursor(cursor);
            statusBar()->showMessage(tr("已替换 1 处"), 2000);
        }
    }

    this->findText(findText, caseSensitive, false, wrap);
}

void MainWindow::replaceAllText(const QString &findText,
                                const QString &replaceText,
                                bool caseSensitive, bool wrap)
{
    Q_UNUSED(wrap)

    if (findText.isEmpty()) {
        QMessageBox::information(this, tr("替换"), tr("请输入要查找的内容。"));
        return;
    }

    QTextDocument::FindFlags flags;
    if (caseSensitive)
        flags |= QTextDocument::FindCaseSensitively;

    QTextDocument *document = ui->plainTextEdit->document();
    QTextCursor cursor(document);
    int count = 0;

    cursor.beginEditBlock();
    while (true) {
        QTextCursor found = document->find(findText, cursor, flags);
        if (found.isNull())
            break;

        const int start = found.selectionStart();
        found.insertText(replaceText);
        cursor.setPosition(start + replaceText.length());
        ++count;
    }
    cursor.endEditBlock();

    if (count == 0) {
        QMessageBox::information(this, tr("替换"),
                                 tr("未找到“%1”。").arg(findText));
        return;
    }

    statusBar()->showMessage(tr("已替换 %1 处").arg(count), 3000);
}

void MainWindow::showAboutDialog()
{
    AboutDialog dialog(this);
    dialog.exec();
}

void MainWindow::autoSave()
{
    if (currentFilePath.isEmpty()
            || !ui->plainTextEdit->document()->isModified()) {
        return;
    }

    if (saveToFile(currentFilePath, false))
        statusBar()->showMessage(tr("已自动保存"), 2000);
    else
        statusBar()->showMessage(tr("自动保存失败"), 3000);
}

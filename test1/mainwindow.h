#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QChar>
#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    // 所有按钮共用的槽：读取按钮上登记的 action 属性，转发给 handleAction
    void onButtonClicked();

private:
    // ===== 鼠标与键盘共用的唯一动作处理入口 =====
    // action 取值："0"~"9" "." "+" "-" "*" "/" "=" "C" "CE" "backspace"
    void handleAction(const QString &action);

    double calculate(double a, QChar op, double b, bool *ok) const;
    QString formatResult(double value) const;   // 结果格式化：去掉多余的尾随 0
    void setError(const QString &msg);          // 进入错误态，仅 C/CE 可退出
    void render();                              // 依据状态刷新显示区
    void prepareNewOperand();                   // 结果态下开始新一轮输入前的复位

    Ui::MainWindow *ui;

    // ===== 计算状态机 =====
    double  m_accumulator = 0;      // 第一操作数 / 最近一次计算结果
    QChar   m_pendingOp;            // 待执行的运算符，'\0' 表示当前没有
    QString m_currentInput;         // 正在输入的操作数文本（含小数点、正负号）
    QChar   m_lastOp;               // 上次按 = 时的运算符，用于连续按 =
    double  m_lastOperand = 0;      // 上次按 = 时的第二操作数
    bool    m_resultShown = false;  // 显示区当前是计算结果，下一次数字输入要重新开始
    bool    m_errorState = false;   // 错误态（如除数为 0），只响应 C/CE
    QString m_errorText;            // 错误提示文本
};

#endif // MAINWINDOW_H

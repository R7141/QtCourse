#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QHash>
#include <QKeyEvent>
#include <QPushButton>
#include <cmath>

namespace {
QString opSymbol(QChar op)
{
    if (op == QLatin1Char('*'))
        return QStringLiteral("×");
    if (op == QLatin1Char('/'))
        return QStringLiteral("÷");
    return QString(op);
}

const int kMaxInputLength = 16;

const QHash<QString, QString> kButtonActions = {
    {"but0", "0"},   {"but1", "1"},   {"but2", "2"},   {"but3", "3"},
    {"but4", "4"},   {"but5", "5"},   {"but6", "6"},   {"but7", "7"},
    {"but8", "8"},   {"but9", "9"},   {"butdot", "."},
    {"butadd", "+"}, {"butminus", "-"}, {"butmultiply", "*"}, {"butdivide", "/"},
    {"butequal", "="},
    {"butC", "C"},   {"butCE", "CE"}, {"butdelete", "backspace"},
    // 扩展功能键
    {"butplusminus", "neg"},     {"butpercent", "percent"},
    {"butsquareroot", "sqrt"},   {"butsquare", "sqr"}, {"butreciprocal", "recip"},
};
} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 统一连接
    const QList<QPushButton *> buttons = findChildren<QPushButton *>();
    for (QPushButton *btn : buttons) {
        const QString action = kButtonActions.value(btn->objectName());
        if (action.isEmpty())
            continue;
        btn->setProperty("action", action);
        connect(btn, &QPushButton::clicked, this, &MainWindow::onButtonClicked);
    }

    render();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onButtonClicked()
{
    const QString action = sender()->property("action").toString();
    if (!action.isEmpty())
        handleAction(action);
}

// 键盘事件：按键 -> 动作字符串 -> 与鼠标完全相同的 handleAction
void MainWindow::keyPressEvent(QKeyEvent *event)
{
    // 小键盘按键 = 主键 + KeypadModifier（如小键盘 + 返回 Key_Plus|KeypadModifier），
    // 去掉修饰键后可用同一套分支处理
    const int key = event->key();

    // 数字键（含小键盘：小键盘数字同样返回 Key_0~Key_9）
    if (key >= Qt::Key_0 && key <= Qt::Key_9) {
        handleAction(QString::number(key - Qt::Key_0));
        return;
    }

    switch (key) {
    case Qt::Key_Period:        // 主键盘 / 小键盘小数点
    case Qt::Key_Comma:         // 部分键盘布局用逗号
        handleAction(QStringLiteral("."));
        break;
    case Qt::Key_Plus:          // 主键盘与小键盘的 + - * /
        handleAction(QStringLiteral("+"));
        break;
    case Qt::Key_Minus:
        handleAction(QStringLiteral("-"));
        break;
    case Qt::Key_Asterisk:
        handleAction(QStringLiteral("*"));
        break;
    case Qt::Key_Slash:
        handleAction(QStringLiteral("/"));
        break;
    case Qt::Key_Enter:         // 回车（主键盘）
    case Qt::Key_Return:        // 小键盘回车
    case Qt::Key_Equal:         // 主键盘 =
        handleAction(QStringLiteral("="));
        break;
    case Qt::Key_Backspace:
        handleAction(QStringLiteral("backspace"));
        break;
    case Qt::Key_Escape:        // Esc 等价于 C：全部清除
        handleAction(QStringLiteral("C"));
        break;
    case Qt::Key_Delete:        // Delete 等价于 CE：只清当前输入
        handleAction(QStringLiteral("CE"));
        break;
    case Qt::Key_F9:            // F9 等价于 ±：取反
        handleAction(QStringLiteral("neg"));
        break;
    case Qt::Key_R:             // R 等价于 √：平方根
        handleAction(QStringLiteral("sqrt"));
        break;
    case Qt::Key_Q:             // Q 等价于 x²：平方
        handleAction(QStringLiteral("sqr"));
        break;
    case Qt::Key_I:             // I 等价于 1/x：倒数
        handleAction(QStringLiteral("recip"));
        break;
    case Qt::Key_Percent:       // Shift+5 等价于 %：百分比
        handleAction(QStringLiteral("percent"));
        break;
    default:
        QMainWindow::keyPressEvent(event);
        return;
    }
    event->accept();
}

// 统一动作处理：数字 / 小数点 / 运算符 / 等号 / 清除 / 退格
void MainWindow::handleAction(const QString &action)
{
    // 错误态（除数为 0 等）下除清除外一律忽略
    if (m_errorState && action != "C" && action != "CE")
        return;

    // 数字输入
    if (action.size() == 1 && action.at(0).isDigit()) {
        prepareNewOperand();
        if (m_inputIsResult) {
            m_currentInput.clear();
            m_inputIsResult = false;
        }
        if (m_currentInput.size() >= kMaxInputLength)
            return; // 超过长度上限：忽略，防止溢出 double 精度与显示区
        // 去掉多余的前导 0：单独的 "0" 再按数字时被新数字替换
        if (m_currentInput == "0" || m_currentInput == "-0")
            m_currentInput = (m_currentInput.startsWith('-') ? QStringLiteral("-") : QString())
                             + action;
        else
            m_currentInput += action;
        render();
        return;
    }

    // 小数点：同一操作数内只允许一个
    if (action == ".") {
        prepareNewOperand();
        if (m_inputIsResult) {
            m_currentInput.clear();
            m_inputIsResult = false;
        }
        if (m_currentInput.size() >= kMaxInputLength)
            return;
        if (m_currentInput.contains(QLatin1Char('.')))
            return; // 连续小数点：直接忽略，不改变状态
        if (m_currentInput.isEmpty())
            m_currentInput = QStringLiteral("0.");
        else
            m_currentInput += QLatin1Char('.');
        render();
        return;
    }

    // 二元运算符
    if (action == "+" || action == "-" || action == "*" || action == "/") {
        const QChar op = action.at(0);
        if (!m_currentInput.isEmpty()) {
            if (m_pendingOp.isNull()) {
                // 第一操作数输入完毕，记为累加器并等待第二操作数
                m_accumulator = m_currentInput.toDouble();
                m_currentInput.clear();
                m_inputIsResult = false;
            } else {
                // 连续运算：第二操作数已输入，先算出中间结果再挂起新的运算符
                bool ok = false;
                const double r =
                    calculate(m_accumulator, m_pendingOp, m_currentInput.toDouble(), &ok);
                if (!ok) {
                    setError(QStringLiteral("错误:除数不能为0"));
                    return;
                }
                if (!std::isfinite(r)) {
                    setError(QStringLiteral("错误:数值溢出"));
                    return;
                }
                m_accumulator = r;
                m_currentInput.clear();
                m_inputIsResult = false;
            }
        }
        // 输入为空时：已有运算符则只替换，否则以当前显示值为第一操作数
        m_pendingOp = op;
        m_resultShown = false;
        render();
        return;
    }

    // 等号：按 = 才出最终结果
    if (action == "=") {
        if (m_pendingOp.isNull()) {
            // 没有未完成的运算：有历史运算时重复上一次运算（连续按 =）
            if (m_lastOp.isNull() || !m_resultShown)
                return;
            bool ok = false;
            const double r = calculate(m_accumulator, m_lastOp, m_lastOperand, &ok);
            if (!ok) {
                setError(QStringLiteral("错误:除数不能为0"));
                return;
            }
            if (!std::isfinite(r)) {
                setError(QStringLiteral("错误:数值溢出"));
                return;
            }
            m_accumulator = r;
            m_resultShown = true;
            render();
            return;
        }

        // 第二操作数为空时（如 "5+" 后直接按 =）按第一操作数参与运算
        const double b = m_currentInput.isEmpty() ? m_accumulator
                                                  : m_currentInput.toDouble();
        bool ok = false;
        const double r = calculate(m_accumulator, m_pendingOp, b, &ok);
        if (!ok) {
            setError(QStringLiteral("错误:除数不能为0"));
            return;
        }
        if (!std::isfinite(r)) {
            setError(QStringLiteral("错误:数值溢出"));
            return;
        }
        m_lastOp = m_pendingOp;      // 记录本次运算，供连续按 = 重复执行
        m_lastOperand = b;
        m_accumulator = r;
        m_pendingOp = QChar();
        m_currentInput.clear();
        m_inputIsResult = false;
        m_resultShown = true;
        render();
        return;
    }

    // C：全部复位
    if (action == "C") {
        m_accumulator = 0;
        m_pendingOp = QChar();
        m_currentInput.clear();
        m_lastOp = QChar();
        m_lastOperand = 0;
        m_resultShown = false;
        m_inputIsResult = false;
        m_errorState = false;
        m_errorText.clear();
        render();
        return;
    }

    // CE：只清除当前正在输入的操作数
    if (action == "CE") {
        m_errorState = false;
        m_errorText.clear();
        m_inputIsResult = false;
        if (m_resultShown && m_currentInput.isEmpty()) {
            // 结果就是当前"操作数"：CE 应把显示清零，否则看起来无反应
            m_accumulator = 0;
            m_lastOp = QChar();
            m_lastOperand = 0;
            m_resultShown = false;
        }
        m_currentInput.clear();
        render();
        return;
    }

    // 扩展的一元运算：± % √ x² 1/x
    if (action == "neg" || action == "percent" || action == "sqrt" || action == "sqr"
        || action == "recip") {
        applyUnary(action);
        return;
    }

    // 退格：只在输入过程中生效
    if (action == "backspace") {
        if (m_resultShown || m_currentInput.isEmpty()) {
            if (!m_resultShown && m_currentInput.isEmpty() && !m_pendingOp.isNull()) {
                m_pendingOp = QChar(); // 撤销挂起的运算符后回到第一操作数
                render();
            }
            return; // 结果态/空输入直接忽略，避免越界删除
        }
        m_currentInput.chop(1);
        if (m_currentInput == "-")
            m_currentInput.clear();
        m_inputIsResult = false;
        render();
        return;
    }

    qWarning("未处理的动作: %s", qPrintable(action));
}

void MainWindow::applyUnary(const QString &kind)
{
    const bool hasInput = !m_currentInput.isEmpty();
    const double v = hasInput ? m_currentInput.toDouble() : m_accumulator;

    // 结果写到哪里：有输入→更新操作数文本；无输入→更新累加器
    const auto commit = [this, hasInput](double r, const QString &text) {
        if (!std::isfinite(r)) {
            setError(QStringLiteral("错误:数值溢出"));
            return;
        }
        if (hasInput) {
            m_currentInput = text;
            m_inputIsResult = true; // 计算得到的值，续输数字时重新开始
        } else {
            // 无输入时作用于当前显示值；有挂起运算符时仍在等第二操作数，
            // 不能置 resultShown，否则续输数字会清掉挂起的运算符
            m_accumulator = r;
            m_resultShown = m_pendingOp.isNull();
        }
        render();
    };

    if (kind == "neg") {
        if (hasInput) {
            // 保留输入文本形态（如 "0." → "-0."），避免丢失小数点
            if (m_currentInput.startsWith(QLatin1Char('-')))
                m_currentInput.remove(0, 1);
            else
                m_currentInput.prepend(QLatin1Char('-'));
            render();
        } else {
            commit(-v, QString());
        }
        return;
    }
    if (kind == "percent") {
        commit(v / 100.0, formatResult(v / 100.0));
        return;
    }
    if (kind == "sqrt") {
        if (v < 0) {
            setError(QStringLiteral("错误:负数不能开平方"));
            return;
        }
        commit(std::sqrt(v), formatResult(std::sqrt(v)));
        return;
    }
    if (kind == "sqr") {
        commit(v * v, formatResult(v * v));
        return;
    }
    if (kind == "recip") {
        if (qFuzzyIsNull(v)) {
            setError(QStringLiteral("错误:除数不能为0"));
            return;
        }
        commit(1.0 / v, formatResult(1.0 / v));
        return;
    }
    qWarning("未处理的一元运算: %s", qPrintable(kind));
}

// 四则运算；除数为 0 时返回 ok=false，由调用方进入错误态
double MainWindow::calculate(double a, QChar op, double b, bool *ok) const
{
    *ok = true;
    if (op == QLatin1Char('+'))
        return a + b;
    if (op == QLatin1Char('-'))
        return a - b;
    if (op == QLatin1Char('*'))
        return a * b;
    if (op == QLatin1Char('/')) {
        if (qFuzzyIsNull(b)) {
            *ok = false;
            return 0;
        }
        return a / b;
    }
    *ok = false;
    return 0;
}

// 结果格式化：12 位有效数字并去掉尾随 0；qFuzzyIsNull 消除 -0 的显示
QString MainWindow::formatResult(double value) const
{
    if (qFuzzyIsNull(value))
        value = 0;
    return QString::number(value, 'g', 12);
}

void MainWindow::setError(const QString &msg)
{
    m_errorState = true;
    m_errorText = msg;
    m_currentInput.clear();
    m_inputIsResult = false;
    ui->display->setText(msg);
}

// 结果态下输入数字/小数点前，丢弃上一轮运算链，开始全新输入
void MainWindow::prepareNewOperand()
{
    if (!m_resultShown)
        return;
    m_accumulator = 0;
    m_pendingOp = QChar();
    m_lastOp = QChar();
    m_lastOperand = 0;
    m_currentInput.clear();
    m_resultShown = false;
    m_inputIsResult = false;
}

// 显示规则：错误信息 > 运算中的表达式 > 正在输入的操作数 > 结果
void MainWindow::render()
{
    if (m_errorState) {
        ui->display->setText(m_errorText);
        return;
    }
    // 有待执行运算符时显示完整表达式，便于看清当前输入的数据与操作符
    if (!m_pendingOp.isNull()) {
        QString text = formatResult(m_accumulator)
                       + QLatin1Char(' ') + opSymbol(m_pendingOp);
        if (!m_currentInput.isEmpty())
            text += QLatin1Char(' ') + m_currentInput;
        ui->display->setText(text);
        return;
    }
    if (!m_currentInput.isEmpty())
        ui->display->setText(m_currentInput);
    else
        ui->display->setText(formatResult(m_accumulator));
}

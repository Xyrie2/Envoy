#include "Ui/ChannelEditDialog.h"

#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHostAddress>
#include <QIntValidator>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPushButton>
#include <QShowEvent>
#include <QVBoxLayout>

#include "Ui/Theme.h"
#include "Ui/TitleBar.h"

namespace
{
constexpr int LabelWidth = 68;
constexpr int FieldWidth = 168;
constexpr int ShadowMargin = 12;

QLineEdit* createIpEdit(QWidget* parent)
{
    QLineEdit* edit = new QLineEdit(parent);
    edit->setMinimumWidth(FieldWidth);
    edit->setPlaceholderText(QStringLiteral("0.0.0.0"));
    return edit;
}

QLineEdit* createPortEdit(QWidget* parent)
{
    QLineEdit* edit = new QLineEdit(parent);
    edit->setMinimumWidth(FieldWidth);
    edit->setValidator(new QIntValidator(1, 65535, edit));
    edit->setPlaceholderText(QStringLiteral("1 ~ 65535"));
    return edit;
}

QLabel* createFieldLabel(const QString& text, QWidget* parent)
{
    QLabel* label = new QLabel(text, parent);
    label->setObjectName(QStringLiteral("dlgFieldLabel"));
    label->setFixedWidth(LabelWidth);
    return label;
}

QLabel* createStaticValue(QWidget* parent)
{
    QLabel* value = new QLabel(parent);
    value->setObjectName(QStringLiteral("dlgStaticValue"));
    return value;
}
}

ChannelEditDialog::ChannelEditDialog(QWidget* parent) : QDialog(parent)
{
    setObjectName(QStringLiteral("channelEditDialog"));
    setWindowTitle(tr("编辑通道"));
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setModal(true);
    setSizeGripEnabled(false);

    QVBoxLayout* shell = new QVBoxLayout(this);
    shell->setContentsMargins(ShadowMargin, ShadowMargin, ShadowMargin, ShadowMargin);
    shell->setSpacing(0);

    QFrame* card = new QFrame(this);
    card->setObjectName(QStringLiteral("channelEditDialogCard"));
    card->setAttribute(Qt::WA_StyledBackground, true);
    QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect(card);
    shadow->setBlurRadius(24);
    shadow->setOffset(0, 6);
    shadow->setColor(QColor(0, 0, 0, 170));
    card->setGraphicsEffect(shadow);
    shell->addWidget(card);

    QVBoxLayout* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(0, 0, 0, 0);
    cardLayout->setSpacing(0);

    m_titleBar = new TitleBar(card);
    m_titleBar->setDialogMode(tr("编辑通道"));
    connect(m_titleBar, &TitleBar::closeClicked, this, &QDialog::reject);
    cardLayout->addWidget(m_titleBar);

    QWidget* body = new QWidget(card);
    QGridLayout* grid = new QGridLayout(body);
    grid->setContentsMargins(16, 14, 16, 10);
    grid->setHorizontalSpacing(12);
    grid->setVerticalSpacing(8);

    m_nameValue = createStaticValue(body);
    m_modeValue = createStaticValue(body);
    m_modeValue->setText(QStringLiteral("UDP"));
    m_srcIpEdit = createIpEdit(body);
    m_srcPortEdit = createPortEdit(body);
    m_dstIpEdit = createIpEdit(body);
    m_dstPortEdit = createPortEdit(body);

    grid->addWidget(createFieldLabel(tr("名称"), body), 0, 0);
    grid->addWidget(m_nameValue, 0, 1);
    grid->addWidget(createFieldLabel(tr("通信方式"), body), 1, 0);
    grid->addWidget(m_modeValue, 1, 1);
    grid->addWidget(createFieldLabel(tr("源IP"), body), 2, 0);
    grid->addWidget(m_srcIpEdit, 2, 1);
    grid->addWidget(createFieldLabel(tr("源端口"), body), 3, 0);
    grid->addWidget(m_srcPortEdit, 3, 1);
    grid->addWidget(createFieldLabel(tr("目的IP"), body), 4, 0);
    grid->addWidget(m_dstIpEdit, 4, 1);
    grid->addWidget(createFieldLabel(tr("目的端口"), body), 5, 0);
    grid->addWidget(m_dstPortEdit, 5, 1);
    grid->setColumnStretch(1, 1);

    m_errorLabel = new QLabel(body);
    m_errorLabel->setObjectName(QStringLiteral("dlgErrorText"));
    m_errorLabel->setWordWrap(true);
    m_errorLabel->setFixedHeight(m_errorLabel->fontMetrics().height());
    grid->addWidget(m_errorLabel, 6, 0, 1, 2);

    cardLayout->addWidget(body);

    QFrame* footer = new QFrame(card);
    footer->setObjectName(QStringLiteral("dlgFooter"));
    footer->setAttribute(Qt::WA_StyledBackground, true);
    QHBoxLayout* footerLayout = new QHBoxLayout(footer);
    footerLayout->setContentsMargins(16, 10, 16, 12);
    footerLayout->setSpacing(8);
    footerLayout->addStretch(1);

    QPushButton* cancelButton = new QPushButton(tr("取消"), footer);
    cancelButton->setObjectName(QStringLiteral("dlgSecondaryButton"));
    cancelButton->setAutoDefault(false);
    QPushButton* okButton = new QPushButton(tr("确定"), footer);
    okButton->setObjectName(QStringLiteral("dlgPrimaryButton"));
    okButton->setDefault(true);
    okButton->setAutoDefault(true);
    footerLayout->addWidget(cancelButton);
    footerLayout->addWidget(okButton);
    cardLayout->addWidget(footer);

    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(okButton, &QPushButton::clicked, this, &ChannelEditDialog::onAcceptClicked);

    for (QLineEdit* edit : {m_srcIpEdit, m_srcPortEdit, m_dstIpEdit, m_dstPortEdit})
    {
        connect(edit, &QLineEdit::returnPressed, this, &ChannelEditDialog::onAcceptClicked);
        connect(edit, &QLineEdit::textChanged, this,
                [this]()
                {
                    if (!m_errorLabel->text().isEmpty())
                    {
                        m_errorLabel->clear();
                    }
                });
    }

    layout()->setSizeConstraint(QLayout::SetFixedSize);
}

void ChannelEditDialog::setChannelName(const QString& name)
{
    m_nameValue->setText(name);
}

void ChannelEditDialog::setEndpoints(const QString& srcIp, quint16 srcPort, const QString& dstIp, quint16 dstPort)
{
    m_srcIpEdit->setText(srcIp);
    m_srcPortEdit->setText(srcPort > 0 ? QString::number(srcPort) : QString());
    m_dstIpEdit->setText(dstIp);
    m_dstPortEdit->setText(dstPort > 0 ? QString::number(dstPort) : QString());
}

QString ChannelEditDialog::srcIp() const
{
    return m_srcIpEdit->text().trimmed();
}

quint16 ChannelEditDialog::srcPort() const
{
    return static_cast<quint16>(m_srcPortEdit->text().toUShort());
}

QString ChannelEditDialog::dstIp() const
{
    return m_dstIpEdit->text().trimmed();
}

quint16 ChannelEditDialog::dstPort() const
{
    return static_cast<quint16>(m_dstPortEdit->text().toUShort());
}

void ChannelEditDialog::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);

    if (QWidget* host = parentWidget() ? parentWidget()->window() : nullptr)
    {
        const QRect hostRect = host->frameGeometry();
        if (hostRect.isValid() && !hostRect.isEmpty())
        {
            move(hostRect.center().x() - width() / 2, hostRect.center().y() - height() / 2);
        }
    }
    m_srcIpEdit->setFocus();
    m_srcIpEdit->selectAll();
}

void ChannelEditDialog::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && m_titleBar && m_titleBar->isCaptionAt(event->globalPos()))
    {
        m_dragging = true;
        m_dragOffset = event->globalPos() - frameGeometry().topLeft();
        event->accept();
        return;
    }
    QDialog::mousePressEvent(event);
}

void ChannelEditDialog::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton))
    {
        move(event->globalPos() - m_dragOffset);
        event->accept();
        return;
    }
    QDialog::mouseMoveEvent(event);
}

void ChannelEditDialog::mouseReleaseEvent(QMouseEvent* event)
{
    m_dragging = false;
    QDialog::mouseReleaseEvent(event);
}

void ChannelEditDialog::onAcceptClicked()
{
    QLineEdit* invalid = nullptr;
    QString message;
    if (!validate(&invalid, &message))
    {
        setError(message);
        if (invalid)
        {
            invalid->setFocus();
            invalid->selectAll();
        }
        return;
    }
    accept();
}

bool ChannelEditDialog::validate(QLineEdit** firstInvalid, QString* message)
{
    const QString srcIp = m_srcIpEdit->text().trimmed();
    const QString dstIp = m_dstIpEdit->text().trimmed();
    QHostAddress address;

    if (srcIp.isEmpty() || !address.setAddress(srcIp))
    {
        *firstInvalid = m_srcIpEdit;
        *message = tr("源IP 无效，请填写合法的 IP 地址（如 192.168.1.10）。");
        return false;
    }
    if (m_srcPortEdit->text().toUShort() == 0)
    {
        *firstInvalid = m_srcPortEdit;
        *message = tr("源端口无效，请在 1 ~ 65535 之间填写。");
        return false;
    }
    if (dstIp.isEmpty() || !address.setAddress(dstIp))
    {
        *firstInvalid = m_dstIpEdit;
        *message = tr("目的IP 无效，请填写合法的 IP 地址（如 192.168.1.20）。");
        return false;
    }
    if (m_dstPortEdit->text().toUShort() == 0)
    {
        *firstInvalid = m_dstPortEdit;
        *message = tr("目的端口无效，请在 1 ~ 65535 之间填写。");
        return false;
    }
    return true;
}

void ChannelEditDialog::setError(const QString& text)
{
    m_errorLabel->setText(text);
}

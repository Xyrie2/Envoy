#include "Ui/AppStatusBar.h"
#include "Ui/Theme.h"

#include <QHBoxLayout>
#include <QLabel>

namespace
{
const QColor ColorIdle = Theme::Pal::statusIdle();
const QColor ColorLinked = Theme::Pal::statusLinked();
const QColor ColorLost = Theme::Pal::statusLost();
}

const QString AppStatusBar::ClockFormat = QStringLiteral("yyyy-MM-dd HH:mm:ss");

AppStatusBar::AppStatusBar(QWidget* parent) : QFrame(parent)
{
    setObjectName(QStringLiteral("appStatusBar"));
    setFixedHeight(28);

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 0, 10, 0);
    layout->setSpacing(10);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setObjectName(QStringLiteral("statusLabel"));
    layout->addWidget(m_statusLabel);

    m_infoLabel = new QLabel(this);
    m_infoLabel->setObjectName(QStringLiteral("statusInfoLabel"));
    layout->addWidget(m_infoLabel, 1);

    m_rightLabel = new QLabel(this);
    m_rightLabel->setObjectName(QStringLiteral("statusTimeLabel"));
    layout->addWidget(m_rightLabel);

    setConnectionState(ConnState::Idle, 0);
    setClock(QStringLiteral("---- -- -- --:--:--"));
}

void AppStatusBar::setConnectionState(ConnState state, int secondsSinceLastRx)
{
    if (m_lastStyledState != state)
    {
        m_lastStyledState = state;
        QColor c;
        switch (state)
        {
        case ConnState::Linked:
            c = ColorLinked;
            break;
        case ConnState::Lost:
            c = ColorLost;
            break;
        case ConnState::Idle:
        default:
            c = ColorIdle;
            break;
        }
        m_statusLabel->setStyleSheet(QStringLiteral("color: %1; font-weight: 600;").arg(c.name()));
    }

    QString text;
    switch (state)
    {
    case ConnState::Linked:
        text = tr("● 已连接");
        break;
    case ConnState::Lost:
        text = tr("● 断链 %1s").arg(secondsSinceLastRx);
        break;
    case ConnState::Idle:
    default:
        text = tr("● 待收包");
        break;
    }
    m_statusLabel->setText(text);
}

void AppStatusBar::setTelemetry(const QString& summary, bool warning)
{
    m_infoLabel->setStyleSheet(QStringLiteral("color: %1;") .arg((warning ? ColorLost : ColorIdle).name()));
    m_infoLabel->setText(summary);
}

void AppStatusBar::setClock(const QString& timeText)
{
    m_rightLabel->setText(timeText);
}

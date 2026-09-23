#include "Ui/CollapsibleGroupBox.h"

#include "Ui/Theme.h"

#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QVBoxLayout>

namespace
{
/// <summary>
/// @brief 细线 chevron 箭头
/// </summary>
QPixmap chevronPixmap(bool expanded, const QColor& color)
{
    QPixmap pm(32, 32);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    QPen pen(color, 3.0);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    p.setPen(pen);
    if (expanded)
    {
        p.drawLine(10, 12, 16, 19);
        p.drawLine(16, 19, 22, 12);
    }
    else
    {
        p.drawLine(12, 10, 19, 16);
        p.drawLine(19, 16, 12, 22);
    }
    pm.setDevicePixelRatio(2.0);
    return pm;
}
} 

CollapsibleGroupBox::CollapsibleGroupBox(const QString& title, QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("collapsibleGroup"));
    setAttribute(Qt::WA_StyledBackground, true);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // 标题行：粗体标题 + 右侧箭头；整行可点击展开/折叠
    m_header = new QWidget(this);
    m_header->setObjectName(QStringLiteral("sectionHeader"));
    m_header->setCursor(Qt::PointingHandCursor);
    QHBoxLayout* headerLayout = new QHBoxLayout(m_header);
    headerLayout->setContentsMargins(12, 10, 14, 10);
    headerLayout->setSpacing(9);

    QLabel* titleLabel = new QLabel(title, m_header);
    titleLabel->setObjectName(QStringLiteral("sectionTitle"));
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch(1);

    m_arrow = new QLabel(m_header);
    m_arrow->setObjectName(QStringLiteral("sectionArrow"));
    m_arrow->setFixedSize(16, 16);
    m_arrow->setScaledContents(true);
    headerLayout->addWidget(m_arrow);

    layout->addWidget(m_header);

    // 分隔线：仅展开时显示于标题行与内容之间
    m_divider = new QFrame(this);
    m_divider->setObjectName(QStringLiteral("sectionDivider"));
    m_divider->setFrameShape(QFrame::NoFrame);
    m_divider->setFixedHeight(1);
    layout->addWidget(m_divider);

    m_content = new QWidget(this);
    m_content->setObjectName(QStringLiteral("sectionContent"));
    layout->addWidget(m_content);

    // 标题行及其全部子控件上的单击都触发切换
    m_header->installEventFilter(this);
    const QObjectList headerChildren = m_header->children();
    for (QObject* child : headerChildren)
    {
        if (qobject_cast<QWidget*>(child) != nullptr)
        {
            child->installEventFilter(this);
        }
    }

    setExpanded(false);
}

bool CollapsibleGroupBox::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::MouseButtonPress)
    {
        toggle();
        return true;
    }
    return QWidget::eventFilter(watched, event);
}

void CollapsibleGroupBox::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    const QRectF r = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    p.setPen(QPen(Theme::Pal::border(), 1));
    p.setBrush(Theme::Pal::surface());
    p.drawRoundedRect(r, 8, 8);
}

void CollapsibleGroupBox::setExpanded(bool expanded)
{
    const bool changed = m_expanded != expanded;
    m_expanded = expanded;
    if (m_content)
    {
        m_content->setVisible(expanded);
    }
    if (m_divider)
    {
        m_divider->setVisible(expanded);
    }
    updateToggleVisual();
    if (changed)
    {
        emit expandedChanged(expanded);
    }
}

void CollapsibleGroupBox::toggle()
{
    setExpanded(!isExpanded());
}

void CollapsibleGroupBox::updateToggleVisual()
{
    if (m_arrow)
    {
        m_arrow->setPixmap(chevronPixmap(m_expanded, Theme::Pal::textMuted()));
    }
}

#include "Ui/TitleBar.h"
#include "Ui/Theme.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QToolButton>

namespace
{
class WindowButton : public QToolButton
{
public:
    explicit WindowButton(QWidget* parent = nullptr) : QToolButton(parent) {}
    void setPixmaps(const QPixmap& normal, const QPixmap& hover)
    {
        m_normal = normal;
        m_hover = hover;
        setIcon(normal);
        setIconSize(normal.size() / int(normal.devicePixelRatio() < 1.0 ? 1.0 : normal.devicePixelRatio()));
    }
protected:
    void enterEvent(QEvent* event) override
    {
        setIcon(m_hover);
        QToolButton::enterEvent(event);
    }
    void leaveEvent(QEvent* event) override
    {
        setIcon(m_normal);
        QToolButton::leaveEvent(event);
    }

private:
    QPixmap m_normal;
    QPixmap m_hover;
};
}

QPixmap TitleBar::paintSymbol(Symbol symbol, int size, qreal dpr, const QColor& color)
{
    QPixmap pm(int(size * dpr), int(size * dpr));
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);

    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(QPen(color, 1.1));
    p.setBrush(Qt::NoBrush);

    const QRectF r = QRectF(0, 0, size, size).adjusted(4.5, 4.5, -4.5, -4.5);
    switch (symbol)
    {
    case Symbol::Min:
        p.drawLine(QPointF(r.left(), r.center().y() + r.height() * 0.32),
                   QPointF(r.right(), r.center().y() + r.height() * 0.32));
        break;
    case Symbol::Max:
        p.drawRect(r);
        break;
    case Symbol::Restore:
    {
        const QRectF back = QRectF(r.left() + 2.5, r.top(), r.width() - 2.5, r.height() - 2.5);
        const QRectF front = QRectF(r.left(), r.top() + 2.5, r.width() - 2.5, r.height() - 2.5);
        p.drawLine(back.topLeft(), back.topRight());
        p.drawLine(back.topRight(), back.bottomRight());
        p.drawRect(front);
        break;
    }
    case Symbol::Close:
    default:
        p.drawLine(r.topLeft(), r.bottomRight());
        p.drawLine(r.topRight(), r.bottomLeft());
        break;
    }
    return pm;
}

namespace
{
QPixmap paintAppIcon(int size, qreal dpr)
{
    QPixmap pm(int(size * dpr), int(size * dpr));
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);

    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    const QRectF r(0.5, 0.5, size - 1.5, size - 1.5);
    p.setPen(Qt::NoPen);
    p.setBrush(Theme::Pal::accent());
    p.drawEllipse(r);
    p.setPen(QPen(Qt::white, 1.3));
    p.drawLine(r.center(), QPointF(r.center().x(), r.top() + 2.0));
    p.setBrush(Qt::white);
    p.drawEllipse(r.center(), 1.1, 1.1);
    return pm;
}
}


TitleBar::TitleBar(QWidget* parent) : QWidget(parent)
{
    setObjectName(QStringLiteral("titleBar"));
    setFixedHeight(36);
    setAttribute(Qt::WA_StyledBackground, true);
    setFocusPolicy(Qt::NoFocus);

    QHBoxLayout* lay = new QHBoxLayout(this);
    lay->setContentsMargins(10, 0, 0, 0);
    lay->setSpacing(6);

    m_iconLabel = new QLabel(this);
    m_iconLabel->setPixmap(paintAppIcon(16, devicePixelRatioF()));
    m_iconLabel->setFixedSize(18, 18);
    m_iconLabel->setAlignment(Qt::AlignCenter);

    m_titleLabel = new QLabel(tr("Envoy"), this);
    m_titleLabel->setObjectName(QStringLiteral("tbTitle"));

    lay->addWidget(m_iconLabel);
    lay->addWidget(m_titleLabel);

    lay->addStretch(1);

    m_minBtn = makeWindowButton(Symbol::Min, tr("最小化"));
    m_maxBtn = makeWindowButton(Symbol::Max, tr("最大化"));
    m_closeBtn = makeWindowButton(Symbol::Close, tr("关闭"));
    lay->addWidget(m_minBtn);
    lay->addWidget(m_maxBtn);
    lay->addWidget(m_closeBtn);
    lay->addSpacing(2);

    connect(m_minBtn, &QToolButton::clicked, this, &TitleBar::minimizeClicked);
    connect(m_maxBtn, &QToolButton::clicked, this, &TitleBar::maximizeToggled);
    connect(m_closeBtn, &QToolButton::clicked, this, &TitleBar::closeClicked);

    setStyleSheet(QStringLiteral("TitleBar { background:%1; border-top-left-radius:%6px;"
                                 " border-top-right-radius:%6px; border-bottom:1px solid %2; }"
                                 "QLabel#tbTitle { color:%3; font-size:10pt; font-weight:600; }"
                                 "QToolButton#tbWinBtn { border:none; border-radius:6px; background:transparent; }"
                                 "QToolButton#tbWinBtn:hover { background:%4; }"
                                 "QToolButton#tbWinBtn:pressed { background:%7; }"
                                 "QToolButton#tbCloseBtn { border:none; border-radius:6px; background:transparent; }"
                                 "QToolButton#tbCloseBtn:hover { background:%5; }"
                                 "QToolButton#tbCloseBtn:pressed { background:%8; }")
                  .arg(Theme::hex(Theme::Pal::windowBg()), Theme::hex(Theme::Pal::border()),
                       Theme::hex(Theme::Pal::textPrimary()), Theme::hex(Theme::Pal::surfaceHover()),
                       Theme::hex(Theme::Pal::closeRed()))
                  .arg(Theme::WindowRadius)
                  .arg(Theme::hex(Theme::Pal::selBg()))
                  .arg(QStringLiteral("#a3121e")));
}

void TitleBar::setDialogMode(const QString& title)
{
    if (!title.isEmpty() && m_titleLabel)
    {
        m_titleLabel->setText(title);
    }
    if (m_minBtn)
    {
        m_minBtn->hide();
    }
    if (m_maxBtn)
    {
        m_maxBtn->hide();
    }
}

QToolButton* TitleBar::makeWindowButton(Symbol symbol, const QString& tip)
{
    WindowButton* b = new WindowButton(this);
    const QColor normal = Theme::Pal::textMuted();
    const QColor hover = (symbol == Symbol::Close) ? QColor("#ffffff") : Theme::Pal::textPrimary();
    b->setPixmaps(paintSymbol(symbol, 16, devicePixelRatioF(), normal),
                  paintSymbol(symbol, 16, devicePixelRatioF(), hover));
    b->setToolTip(tip);
    b->setObjectName(symbol == Symbol::Close ? QStringLiteral("tbCloseBtn")
                                             : QStringLiteral("tbWinBtn"));
    b->setFixedSize(44, 34);
    b->setAutoRaise(true);
    b->setFocusPolicy(Qt::NoFocus);
    return b;
}

void TitleBar::applyMaximizeSymbol(bool maximized)
{
    WindowButton* b = static_cast<WindowButton*>(m_maxBtn);
    const QColor normal = Theme::Pal::textMuted();
    const QColor hover = Theme::Pal::textPrimary();
    const Symbol sym = maximized ? Symbol::Restore : Symbol::Max;
    b->setPixmaps(paintSymbol(sym, 16, devicePixelRatioF(), normal), paintSymbol(sym, 16, devicePixelRatioF(), hover));
    m_maxBtn->setToolTip(maximized ? tr("还原") : tr("最大化"));
}

void TitleBar::setMaximizedState(bool maximized)
{
    if (m_maxBtn)
    {
        applyMaximizeSymbol(maximized);
    }
}

bool TitleBar::isCaptionAt(const QPoint& globalPos) const
{
    const QPoint local = mapFromGlobal(globalPos);
    if (!rect().contains(local))
    {
        return false;
    }
    QWidget* child = childAt(local);
    if (child == nullptr)
    {
        return true;
    }
    return qobject_cast<QLabel*>(child) != nullptr;
}

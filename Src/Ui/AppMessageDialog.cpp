#include "Ui/AppMessageDialog.h"

#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QScreen>
#include <QShowEvent>
#include <QVBoxLayout>

#include "Ui/TitleBar.h"

namespace
{
constexpr int ShadowMargin = 12;
constexpr int BodyMarginH = 16;
constexpr int FooterMarginH = 16;
constexpr int FooterSpacing = 8;

const char* defaultConfirmText()
{
    return "确认";
}

const char* defaultCancelText()
{
    return "取消";
}

const char* defaultOkText()
{
    return "确定";
}
}

AppMessageDialog::AppMessageDialog(Kind kind, const QString& title, const QString& text, QWidget* parent,
                                   const QString& confirmText, const QString& cancelText)
    : QDialog(parent)
{
    setObjectName(QStringLiteral("appMessageDialog"));
    setWindowTitle(title);
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setModal(true);
    setSizeGripEnabled(false);

    QVBoxLayout* shell = new QVBoxLayout(this);
    shell->setContentsMargins(ShadowMargin, ShadowMargin, ShadowMargin, ShadowMargin);
    shell->setSpacing(0);

    QFrame* card = new QFrame(this);
    card->setObjectName(QStringLiteral("appMessageDialogCard"));
    card->setProperty("dialogCard", true);
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
    m_titleBar->setDialogMode(title);
    connect(m_titleBar, &TitleBar::closeClicked, this, &QDialog::reject);
    cardLayout->addWidget(m_titleBar);

    m_footer = new QFrame(card);
    m_footer->setObjectName(QStringLiteral("dlgFooter"));
    m_footer->setAttribute(Qt::WA_StyledBackground, true);
    QHBoxLayout* footerLayout = new QHBoxLayout(m_footer);
    footerLayout->setContentsMargins(FooterMarginH, 10, FooterMarginH, 12);
    footerLayout->setSpacing(FooterSpacing);
    footerLayout->addStretch(1);

    QPushButton* primary = new QPushButton(
        kind == Kind::Question ? (confirmText.isEmpty() ? tr(defaultConfirmText()) : confirmText)
                               : (confirmText.isEmpty() ? tr(defaultOkText()) : confirmText),
        m_footer);
    primary->setObjectName(QStringLiteral("dlgPrimaryButton"));
    m_primaryButton = primary;

    if (kind == Kind::Question)
    {
        QPushButton* secondary = new QPushButton(cancelText.isEmpty() ? tr(defaultCancelText()) : cancelText, m_footer);
        secondary->setObjectName(QStringLiteral("dlgSecondaryButton"));
        secondary->setDefault(true);
        secondary->setAutoDefault(true);
        primary->setAutoDefault(true);
        footerLayout->addWidget(secondary);
        footerLayout->addWidget(primary);
        connect(secondary, &QPushButton::clicked, this, &QDialog::reject);
    }
    else
    {
        primary->setDefault(true);
        primary->setAutoDefault(true);
        footerLayout->addWidget(primary);
    }
    connect(primary, &QPushButton::clicked, this, &QDialog::accept);

    QWidget* body = new QWidget(card);
    QHBoxLayout* bodyLayout = new QHBoxLayout(body);
    bodyLayout->setContentsMargins(BodyMarginH, 14, BodyMarginH, 12);
    bodyLayout->setSpacing(0);

    m_textLabel = new QLabel(text, body);
    m_textLabel->setObjectName(QStringLiteral("msgDialogText"));
    m_textLabel->setWordWrap(true);
    m_textLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_textLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    bodyLayout->addWidget(m_textLabel);

    cardLayout->addWidget(body);
    cardLayout->addWidget(m_footer);

    layout()->setSizeConstraint(QLayout::SetFixedSize);
}

void AppMessageDialog::syncBodyWidthToFooter()
{
    if (m_widthSynced || !m_textLabel || !m_footer || !m_primaryButton)
    {
        return;
    }

    const int buttonWidth = m_primaryButton->sizeHint().width();
    const int twoButtonFooter = 2 * FooterMarginH + 2 * buttonWidth + FooterSpacing;
    const int cardWidth = qMax(m_footer->sizeHint().width(), twoButtonFooter);
    m_textLabel->setFixedWidth(cardWidth - 2 * BodyMarginH);
    m_widthSynced = true;
}

void AppMessageDialog::centerOnHost()
{
    if (QWidget* host = parentWidget() ? parentWidget()->window() : nullptr)
    {
        const QRect hostRect = host->frameGeometry();
        if (hostRect.isValid() && !hostRect.isEmpty())
        {
            move(hostRect.center().x() - width() / 2, hostRect.center().y() - height() / 2);
            return;
        }
    }
    if (QScreen* screen = QGuiApplication::primaryScreen())
    {
        const QRect avail = screen->availableGeometry();
        move(avail.center().x() - width() / 2, avail.center().y() - height() / 2);
    }
}

void AppMessageDialog::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);
    syncBodyWidthToFooter();
    centerOnHost();
}

void AppMessageDialog::mousePressEvent(QMouseEvent* event)
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

void AppMessageDialog::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton))
    {
        move(event->globalPos() - m_dragOffset);
        event->accept();
        return;
    }
    QDialog::mouseMoveEvent(event);
}

void AppMessageDialog::mouseReleaseEvent(QMouseEvent* event)
{
    m_dragging = false;
    QDialog::mouseReleaseEvent(event);
}

bool AppMessageDialog::Confirm(QWidget* parent, const QString& title, const QString& text,
                               const QString& confirmText, const QString& cancelText)
{
    AppMessageDialog dialog(Kind::Question, title, text, parent, confirmText, cancelText);
    return dialog.exec() == QDialog::Accepted;
}

void AppMessageDialog::Information(QWidget* parent, const QString& title, const QString& text, const QString& okText)
{
    AppMessageDialog dialog(Kind::Info, title, text, parent, okText);
    dialog.exec();
}

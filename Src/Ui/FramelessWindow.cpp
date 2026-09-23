#include "Ui/FramelessWindow.h"
#include "Ui/Theme.h"
#include "Ui/TitleBar.h"

#include <QCloseEvent>
#include <QEvent>
#include <QGuiApplication>
#include <QWindow>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QScreen>
#include <QShowEvent>
#include <QTimer>
#include <QVBoxLayout>
#include <QtGlobal>

#ifdef Q_OS_WIN
#include <windows.h>
#include <windowsx.h>

#ifndef SM_CXPADDEDBORDER
#define SM_CXPADDEDBORDER 92
#endif
#endif

namespace
{
constexpr int Border = 8;
constexpr int CornerRadius = Theme::WindowRadius;

#ifdef Q_OS_WIN
void ensureNativeResizeFrame(HWND hwnd)
{
    if (!hwnd)
    {
        return;
    }
    const LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
    const LONG_PTR required = WS_THICKFRAME | WS_CAPTION | WS_SYSMENU;
    if ((style & required) == required && (style & WS_POPUP) == 0)
        return;
    SetWindowLongPtrW(hwnd, GWL_STYLE, (style & ~static_cast<LONG_PTR>(WS_POPUP)) | required);
    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

typedef HRESULT(WINAPI * DwmSetWindowAttributeFn)(HWND, DWORD, LPCVOID, DWORD);
DwmSetWindowAttributeFn dwmSetWindowAttributeFn()
{
    static DwmSetWindowAttributeFn fn = []() -> DwmSetWindowAttributeFn
    {
        if (HMODULE dwm = LoadLibraryW(L"dwmapi.dll"))
        {
            return reinterpret_cast<DwmSetWindowAttributeFn>(GetProcAddress(dwm, "DwmSetWindowAttribute"));
        }
        return nullptr;
    }();
    return fn;
}

constexpr DWORD kDwmWindowCornerPreference = 33;
constexpr DWORD kDwmCornerRound = 2;
#endif
}


FramelessWindow::FramelessWindow(QWidget* parent) : QWidget(parent)
{
    setObjectName(QStringLiteral("framelessShell"));
    setWindowFlag(Qt::FramelessWindowHint, true);
    setAttribute(Qt::WA_TranslucentBackground);
    setMinimumSize(MinWidth, MinHeight);

    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);

#ifdef Q_OS_WIN
    ensureNativeResizeFrame(reinterpret_cast<HWND>(winId()));
#endif
    applyRoundedCorners();
}

void FramelessWindow::setContentWidget(QWidget* content)
{
    if (!content)
    {
        return;
    }
    if (m_content)
    {
        m_layout->removeWidget(m_content);
    }
    m_content = content;
    content->setParent(this, Qt::Widget);
    m_layout->addWidget(content, 1);
}

void FramelessWindow::setTitleBar(TitleBar* bar)
{
    if (!bar)
    {
        return;
    }
    m_titleBar = bar;
    m_layout->insertWidget(0, bar, 0);
#ifdef MANUAL_DRAG_FALLBACK
    bar->installEventFilter(this);
#endif
}

bool FramelessWindow::hitCaptionAt(const QPoint& globalPos) const
{
    return m_titleBar && m_titleBar->isCaptionAt(globalPos);
}

void FramelessWindow::toggleMaximize()
{
    if (m_pseudoMaximized)
    {
        m_pseudoMaximized = false;
        setGeometry(m_savedGeometry);
    }
    else
    {
        m_savedGeometry = geometry();
        m_pseudoMaximized = true;
        setGeometry(maximizedGeometry());
    }
    if (m_titleBar)
    {
        m_titleBar->setMaximizedState(m_pseudoMaximized);
    }
    applyRoundedCorners();
    if (!isVisible())
    {
        show();
    }
}

void FramelessWindow::showPseudoMaximized()
{
    if (!windowMaximized())
    {
        m_savedGeometry = geometry();
        m_pseudoMaximized = true;
        setGeometry(maximizedGeometry());
        if (m_titleBar)
        {
            m_titleBar->setMaximizedState(true);
        }
        applyRoundedCorners();
    }
    show();
}

void FramelessWindow::restoreGeometryForSave()
{
    if (m_pseudoMaximized)
    {
        m_pseudoMaximized = false;
        setGeometry(m_savedGeometry);
    }
}

bool FramelessWindow::windowMaximized() const
{
    return m_pseudoMaximized || isMaximized() || isFullScreen();
}

QRect FramelessWindow::maximizedGeometry() const
{
    QScreen* screen = windowHandle() ? windowHandle()->screen() : nullptr;
    if (!screen)
    {
        const QPoint center = geometry().center();
        screen = QGuiApplication::screenAt(center);
    }
    if (!screen)
    {
        screen = QGuiApplication::primaryScreen();
    }
    return screen ? screen->availableGeometry() : geometry();
}

bool FramelessWindow::nativeEvent(const QByteArray& eventType, void* message, long* result)
{
#ifndef Q_OS_WIN
    Q_UNUSED(result)
    return QWidget::nativeEvent(eventType, message, result);
#else
    if (eventType != QByteArrayLiteral("windows_generic_MSG"))
    {
        return QWidget::nativeEvent(eventType, message, result);
    }

    MSG* msg = static_cast<MSG*>(message);
    if (msg->message == WM_SYSCOMMAND)
    {
        const WPARAM cmd = msg->wParam & 0xFFF0;
        if (cmd == SC_MAXIMIZE)
        {
            toggleMaximize();
            *result = 0;
            return true;
        }
        if (cmd == SC_RESTORE && m_pseudoMaximized && !IsIconic(msg->hwnd))
        {
            toggleMaximize();
            *result = 0;
            return true;
        }
    }
    if (msg->message == WM_NCLBUTTONDBLCLK && msg->wParam == HTCAPTION)
    {
        toggleMaximize();
        *result = 0;
        return true;
    }

    if (msg->message == WM_NCCALCSIZE && msg->wParam == TRUE)
    {
        if (IsZoomed(msg->hwnd))
        {
            RECT* rc = reinterpret_cast<RECT*>(msg->lParam);
            const int f = GetSystemMetrics(SM_CXSIZEFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);
            rc->left += f;
            rc->top += f;
            rc->right -= f;
            rc->bottom -= f;
        }
        *result = 0;
        return true;
    }

    if (msg->message == WM_NCHITTEST)
    {
        const POINT pt = {GET_X_LPARAM(msg->lParam), GET_Y_LPARAM(msg->lParam)};
        RECT rc;
        if (!GetWindowRect(msg->hwnd, &rc))
        {
            return QWidget::nativeEvent(eventType, message, result);
        }

        const int b = qRound(Border * devicePixelRatioF());
        const bool L = pt.x - rc.left < b;
        const bool R = rc.right - pt.x < b;
        const bool T = pt.y - rc.top < b;
        const bool B = rc.bottom - pt.y < b;

        if (windowMaximized())
        {
            if (hitCaptionAt(QPoint(pt.x, pt.y)))
            {
                *result = HTCAPTION;
                return true;
            }
            return QWidget::nativeEvent(eventType, message, result);
        }

        if (L && T)
        {
            *result = HTTOPLEFT;
            return true;
        }
        if (R && T)
        {
            *result = HTTOPRIGHT;
            return true;
        }
        if (L && B)
        {
            *result = HTBOTTOMLEFT;
            return true;
        }
        if (R && B)
        {
            *result = HTBOTTOMRIGHT;
            return true;
        }
        if (L)
        {
            *result = HTLEFT;
            return true;
        }
        if (R)
        {
            *result = HTRIGHT;
            return true;
        }
        if (T)
        {
            *result = HTTOP;
            return true;
        }
        if (B)
        {
            *result = HTBOTTOM;
            return true;
        }

#ifndef MANUAL_DRAG_FALLBACK
        if (hitCaptionAt(QPoint(pt.x, pt.y)))
        {
            *result = HTCAPTION;
            return true;
        }
#endif
        return QWidget::nativeEvent(eventType, message, result);
    }

    if (msg->message == WM_GETMINMAXINFO)
    {
        MINMAXINFO* mmi = reinterpret_cast<MINMAXINFO*>(msg->lParam);
        HMONITOR hMon = MonitorFromWindow(msg->hwnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFO mi;
        mi.cbSize = sizeof(mi);
        if (GetMonitorInfoW(hMon, &mi))
        {
            mmi->ptMaxSize.x = mi.rcWork.right - mi.rcWork.left;
            mmi->ptMaxSize.y = mi.rcWork.bottom - mi.rcWork.top;
            mmi->ptMaxPosition.x = mi.rcWork.left;
            mmi->ptMaxPosition.y = mi.rcWork.top;
        }
        const qreal dpr = devicePixelRatioF();
        mmi->ptMinTrackSize.x = qRound(minimumSize().width() * dpr);
        mmi->ptMinTrackSize.y = qRound(minimumSize().height() * dpr);
        *result = 0;
        return true;
    }

    return QWidget::nativeEvent(eventType, message, result);
#endif
}

void FramelessWindow::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::WindowStateChange)
    {
        if (m_titleBar)
        {
            m_titleBar->setMaximizedState(windowMaximized());
        }
        scheduleNativeChromeFix();
    }
    QWidget::changeEvent(event);
}

void FramelessWindow::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    scheduleNativeChromeFix();
}

void FramelessWindow::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    applyRoundedCorners();
}


void FramelessWindow::scheduleNativeChromeFix()
{
    QTimer::singleShot(0, this, &FramelessWindow::ensureNativeChrome);
}

void FramelessWindow::ensureNativeChrome()
{
#ifdef Q_OS_WIN
    ensureNativeResizeFrame(reinterpret_cast<HWND>(winId()));
#endif
    applyRoundedCorners();
}

void FramelessWindow::applyRoundedCorners()
{
#ifdef Q_OS_WIN
    if (const DwmSetWindowAttributeFn fn = dwmSetWindowAttributeFn())
    {
        const HWND hwnd = reinterpret_cast<HWND>(winId());
        fn(hwnd, kDwmWindowCornerPreference, &kDwmCornerRound, sizeof(kDwmCornerRound));
    }

    if (windowMaximized())
    {
        clearMask();
        return;
    }

    const QRect rc = rect();
    const int mr = Theme::MaskRadius;
    const int d = mr * 2;
    QRegion mask(rc.adjusted(mr, 0, -mr, 0));
    mask += QRegion(rc.adjusted(0, mr, 0, -mr));
    mask += QRegion(rc.left(), rc.top(), d, d, QRegion::Ellipse);
    mask += QRegion(rc.right() - d + 1, rc.top(), d, d, QRegion::Ellipse);
    mask += QRegion(rc.left(), rc.bottom() - d + 1, d, d, QRegion::Ellipse);
    mask += QRegion(rc.right() - d + 1, rc.bottom() - d + 1, d, d, QRegion::Ellipse);
    setMask(mask);
#else
    Q_UNUSED(CornerRadius)
#endif
}

void FramelessWindow::closeEvent(QCloseEvent* event)
{
    event->accept();
}

bool FramelessWindow::eventFilter(QObject* watched, QEvent* event)
{
#ifdef MANUAL_DRAG_FALLBACK
    if (watched == m_titleBar)
    {
        QMouseEvent* me = static_cast<QMouseEvent*>(event);
        switch (event->type())
        {
        case QEvent::MouseButtonPress:
            if (me->button() == Qt::LeftButton && hitCaptionAt(me->globalPos()))
            {
                m_manualDrag = true;
                m_dragOffset = me->globalPos() - frameGeometry().topLeft();
                return true;
            }
            break;
        case QEvent::MouseMove:
            if (m_manualDrag)
            {
                move(me->globalPos() - m_dragOffset);
                return true;
            }
            break;
        case QEvent::MouseButtonRelease:
            m_manualDrag = false;
            break;
        default:
            break;
        }
    }
#else
    Q_UNUSED(watched)
    Q_UNUSED(event)
#endif
    return QWidget::eventFilter(watched, event);
}

#pragma once
#include <QPoint>
#include <QWidget>

#ifndef Q_OS_WIN
#define MANUAL_DRAG_FALLBACK
#endif

class QMouseEvent;
class QResizeEvent;
class QVBoxLayout;
class TitleBar;

class FramelessWindow : public QWidget
{
    Q_OBJECT
public:

    static constexpr int MinWidth = 1080;
    static constexpr int MinHeight = 700;
    FramelessWindow(QWidget* parent = nullptr);

    void setTitleBar(TitleBar* bar);
    void setContentWidget(QWidget* content);
    void toggleMaximize();
    void showPseudoMaximized();
    void restoreGeometryForSave();
    bool windowMaximized() const;

protected:
    void changeEvent(QEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void closeEvent(QCloseEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;
    bool nativeEvent(const QByteArray& eventType, void* message, long* result) override;
private:
    bool hitCaptionAt(const QPoint& globalPos) const;
    void applyRoundedCorners();
    void scheduleNativeChromeFix();
    void ensureNativeChrome();
    QRect maximizedGeometry() const;

    QVBoxLayout* m_layout = nullptr;
    TitleBar* m_titleBar = nullptr;
    QWidget* m_content = nullptr;

    bool m_pseudoMaximized = false;
    QRect m_savedGeometry;

#ifdef MANUAL_DRAG_FALLBACK
    bool m_manualDrag = false;
    QPoint m_dragOffset;
#endif
};

#pragma once
#include <QWidget>

class QLabel;
class QPixmap;
class QToolButton;

class TitleBar : public QWidget
{
    Q_OBJECT
public:
    TitleBar(QWidget* parent = nullptr);

    /// <summary>
    /// @brief 切换为弹窗标题栏
    /// </summary>
    void setDialogMode(const QString& title);

    bool isCaptionAt(const QPoint& globalPos) const;
public slots:
    void setMaximizedState(bool maximized);
signals:
    void minimizeClicked();
    void maximizeToggled();
    void closeClicked();
private:
    enum class Symbol
    {
        Min,
        Max,
        Restore,
        Close
    };
    QToolButton* makeWindowButton(Symbol symbol, const QString& tip);
    static QPixmap paintSymbol(Symbol symbol, int size, qreal dpr, const QColor& color);
    void applyMaximizeSymbol(bool maximized);

    QLabel* m_iconLabel = nullptr;
    QLabel* m_titleLabel = nullptr;
    QToolButton* m_minBtn = nullptr;
    QToolButton* m_maxBtn = nullptr;
    QToolButton* m_closeBtn = nullptr;
};

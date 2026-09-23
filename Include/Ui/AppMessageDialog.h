#pragma once
#include <QDialog>
#include <QPoint>
#include <QString>

class QFrame;
class QLabel;
class QPushButton;
class TitleBar;

/// <summary>
/// @brief 统一风格的提示 / 确认弹窗
/// </summary>
class AppMessageDialog : public QDialog
{
    Q_OBJECT

  public:
    enum class Kind
    {
        Info,
        Question
    };

    AppMessageDialog(Kind kind, const QString& title, const QString& text, QWidget* parent = nullptr,
                     const QString& confirmText = QString(), const QString& cancelText = QString());

    /// <summary>
    /// @brief 动作确认：返回是否点击「确认」（回车/Esc/关闭均视为取消）
    /// </summary>
    static bool Confirm(QWidget* parent, const QString& title, const QString& text,
                        const QString& confirmText = QString(), const QString& cancelText = QString());

    /// <summary>
    /// @brief 普通提示：仅一个「确定」按钮
    /// </summary>
    static void Information(QWidget* parent, const QString& title, const QString& text,
                            const QString& okText = QString());

  protected:
    void showEvent(QShowEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

  private:
    void centerOnHost();
    void syncBodyWidthToFooter();

    TitleBar* m_titleBar = nullptr;
    QLabel* m_textLabel = nullptr;
    QFrame* m_footer = nullptr;
    QPushButton* m_primaryButton = nullptr;
    bool m_widthSynced = false;
    bool m_dragging = false;
    QPoint m_dragOffset;
};

#pragma once
#include <QWidget>

class QFrame;
class QLabel;

/// <summary>
/// @brief 可折叠分组
/// </summary>
class CollapsibleGroupBox : public QWidget
{
    Q_OBJECT

public:
    explicit CollapsibleGroupBox(const QString& title, QWidget* parent = nullptr);

    /// <summary>
    /// @brief 内容容器：把分组内的控件/布局加到这个 widget 上
    /// </summary>
    QWidget* contentWidget() const { return m_content; }

    bool isExpanded() const { return m_expanded; }
public slots:
    void setExpanded(bool expanded);
    void toggle();
signals:
    void expandedChanged(bool expanded);
protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
private:
    void updateToggleVisual();

    QWidget* m_header = nullptr;
    QLabel* m_arrow = nullptr;
    QFrame* m_divider = nullptr;
    QWidget* m_content = nullptr;
    bool m_expanded = false;
};

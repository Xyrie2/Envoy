#pragma once
#include <QDialog>
#include <QPoint>
#include <QString>
#include <QtGlobal>

class QLabel;
class QLineEdit;
class QPushButton;
class TitleBar;

/// <summary>
/// @brief 通道编辑弹窗
/// </summary>
class ChannelEditDialog : public QDialog
{
    Q_OBJECT

  public:
    explicit ChannelEditDialog(QWidget* parent = nullptr);

    void setChannelName(const QString& name);
    void setEndpoints(const QString& srcIp, quint16 srcPort, const QString& dstIp, quint16 dstPort);

    QString srcIp() const;
    quint16 srcPort() const;
    QString dstIp() const;
    quint16 dstPort() const;

  protected:
    void showEvent(QShowEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

  private:
    void onAcceptClicked();
    bool validate(QLineEdit** firstInvalid, QString* message);
    void setError(const QString& text);

    TitleBar* m_titleBar = nullptr;
    QLabel* m_nameValue = nullptr;
    QLabel* m_modeValue = nullptr;
    QLineEdit* m_srcIpEdit = nullptr;
    QLineEdit* m_srcPortEdit = nullptr;
    QLineEdit* m_dstIpEdit = nullptr;
    QLineEdit* m_dstPortEdit = nullptr;
    QLabel* m_errorLabel = nullptr;

    bool m_dragging = false;
    QPoint m_dragOffset;
};

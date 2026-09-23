#pragma once
#include <QVector>
#include <QWidget>

class QLineEdit;
class QPushButton;
class QTableWidget;
class QTableWidgetItem;

/// <summary>
/// @brief 网络设置面板
/// </summary>
class NetworkSettingsPanel : public QWidget
{
    Q_OBJECT

public:
    struct ChannelRow
    {
        QString id;
        QString name;
        QString srcIp;
        quint16 srcPort = 0;
        QString dstIp;
        quint16 dstPort = 0;
    };

    NetworkSettingsPanel(QWidget* parent = nullptr);
    QVector<ChannelRow> channels() const { return m_channels; }
    void setChannels(const QVector<ChannelRow>& rows);
signals:
    void channelEdited(const QString& channelName, const QString& detail);
    void pendingApplyChanged(bool pending);
private:
    void refreshTable();
    void editChannel(const QString& id);
    bool editChannelDialog(ChannelRow& row);
    void setPendingApply(bool pending);
    static QTableWidgetItem* readOnlyItem(const QString& text);

    QVector<ChannelRow> m_channels;
    QTableWidget* m_channelTable = nullptr;
    QPushButton* applyPushButton = nullptr;
    bool m_pendingApply = false;
};

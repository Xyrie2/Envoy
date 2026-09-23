#pragma once
#include <QFrame>

class QLabel;

class AppStatusBar : public QFrame
{
    Q_OBJECT

public:
    enum class ConnState
    {
        Idle,  
        Linked,
        Lost   
    };
    AppStatusBar(QWidget* parent = nullptr);
    static const QString ClockFormat;
    void setConnectionState(ConnState state, int secondsSinceLastRx);
    void setTelemetry(const QString& summary, bool warning = false);
    void setClock(const QString& timeText);
private:
    QLabel* m_infoLabel = nullptr;
    QLabel* m_rightLabel = nullptr;
    QLabel* m_statusLabel = nullptr;
    ConnState m_lastStyledState = ConnState::Idle;
};

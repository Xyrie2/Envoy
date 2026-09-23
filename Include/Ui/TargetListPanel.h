#pragma once
#include <QWidget>

class QGroupBox;
class QPushButton;
class QLabel;
class QTableWidget;
class QToolButton;

class TargetListPanel : public QWidget
{
    Q_OBJECT
public:
    TargetListPanel(QWidget* parent = nullptr);

    QGroupBox* TrackListGroupBox = nullptr;
    QPushButton* modeListButton = nullptr;
    QPushButton* modeFocusButton = nullptr;
    QLabel* statsLabel = nullptr;
    QToolButton* columnSettingsButton = nullptr;
    QTableWidget* trackTableWidget = nullptr;
};

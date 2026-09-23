#include "Ui/TargetListPanel.h"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpacerItem>
#include <QTableWidget>
#include <QToolButton>
#include <QVBoxLayout>

TargetListPanel::TargetListPanel(QWidget* parent) : QWidget(parent)
{
    setObjectName(QStringLiteral("targetListPanel"));

    QVBoxLayout* panelLayout = new QVBoxLayout(this);
    panelLayout->setContentsMargins(0, 0, 0, 0);

    TrackListGroupBox = new QGroupBox(this);
    TrackListGroupBox->setObjectName(QStringLiteral("TrackListGroupBox"));
    TrackListGroupBox->setTitle(tr("航迹监视"));

    QVBoxLayout* trackListLayout = new QVBoxLayout(TrackListGroupBox);
    trackListLayout->setContentsMargins(10, 10, 10, 10);
    trackListLayout->setSpacing(4);

    QHBoxLayout* listHeaderLayout = new QHBoxLayout();
    listHeaderLayout->setSpacing(6);

    QHBoxLayout* segmentLayout = new QHBoxLayout();
    segmentLayout->setSpacing(0);

    modeListButton = new QPushButton(TrackListGroupBox);
    modeListButton->setObjectName(QStringLiteral("modeListButton"));
    modeListButton->setText(tr("目标列表"));
    modeListButton->setCheckable(true);
    modeListButton->setChecked(true);

    modeFocusButton = new QPushButton(TrackListGroupBox);
    modeFocusButton->setObjectName(QStringLiteral("modeFocusButton"));
    modeFocusButton->setText(tr("焦点历史"));
    modeFocusButton->setCheckable(true);

    segmentLayout->addWidget(modeListButton);
    segmentLayout->addWidget(modeFocusButton);
    listHeaderLayout->addLayout(segmentLayout);

    listHeaderLayout->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Fixed));

    statsLabel = new QLabel(TrackListGroupBox);
    statsLabel->setObjectName(QStringLiteral("statsLabel"));
    statsLabel->setText(tr("累计: 0　　存活: 0　　消批: 0"));
    statsLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    listHeaderLayout->addWidget(statsLabel);

    columnSettingsButton = new QToolButton(TrackListGroupBox);
    columnSettingsButton->setObjectName(QStringLiteral("columnSettingsButton"));
    columnSettingsButton->setText(tr("列设置 ▾"));
    columnSettingsButton->setToolTip(tr("选择要显示的表头列"));
    columnSettingsButton->setPopupMode(QToolButton::InstantPopup);
    columnSettingsButton->setCursor(Qt::PointingHandCursor);
    columnSettingsButton->setFocusPolicy(Qt::NoFocus);
    listHeaderLayout->addWidget(columnSettingsButton);

    trackListLayout->setAlignment(listHeaderLayout, Qt::AlignTop);
    trackListLayout->addLayout(listHeaderLayout, 0);
    trackTableWidget = new QTableWidget(TrackListGroupBox);
    trackTableWidget->setObjectName(QStringLiteral("trackTableWidget"));
    trackTableWidget->setMinimumHeight(200);
    trackListLayout->addWidget(trackTableWidget, 1);

    panelLayout->addWidget(TrackListGroupBox);
}

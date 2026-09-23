#include "Ui/ControlPanel.h"

#include "Ui/CollapsibleGroupBox.h"

#include <QComboBox>
#include <QDoubleValidator>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QPushButton>
#include <QScrollArea>
#include <QSpacerItem>
#include <QVBoxLayout>

QLineEdit* addLineEdit(QGridLayout* grid, int row, int col, const char* name, const QString& text)
{
    QLabel* label = new QLabel(text);
    label->setObjectName(QString::fromLatin1(name) + QStringLiteral("Label"));
    grid->addWidget(label, row, col);

    QLineEdit* edit = new QLineEdit;
    edit->setObjectName(QString::fromLatin1(name));
    edit->setMinimumWidth(30);

    QDoubleValidator* validator = new QDoubleValidator(edit);
    validator->setLocale(QLocale::c());
    validator->setRange(-1.0e9, 1.0e9, 8);
    edit->setValidator(validator);
    grid->addWidget(edit, row, col + 1);
    return edit;
}

/// <summary>
/// @brief 可折叠分组的内容布局：标题行下方留白充分，字段行距均匀
/// </summary>
QVBoxLayout* makeSectionLayout(QWidget* content)
{
    QVBoxLayout* layout = new QVBoxLayout(content);
    layout->setContentsMargins(14, 12, 14, 12);
    layout->setSpacing(10);
    return layout;
}

ControlPanel::ControlPanel(QWidget* parent) : QWidget(parent)
{
    setObjectName(QStringLiteral("controlPage"));
    QVBoxLayout* pageLayout = new QVBoxLayout(this);
    pageLayout->setContentsMargins(2, 2, 2, 2);
    pageLayout->setSpacing(2);

    // 滚动区
    controlScrollArea = new QScrollArea(this);
    controlScrollArea->setObjectName(QStringLiteral("controlScrollArea"));
    controlScrollArea->setFrameShape(QFrame::NoFrame);
    controlScrollArea->setWidgetResizable(true);

    QWidget* content = new QWidget;
    content->setObjectName(QStringLiteral("controlPanel"));

    QVBoxLayout* contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(4, 4, 4, 4);
    contentLayout->setSpacing(6);

    // ——— 基础控制：保留开关机与参数查询 ———
    QGroupBox* basicControlGroupBox = new QGroupBox(tr("基础控制"), content);
    basicControlGroupBox->setObjectName(QStringLiteral("BasicControlGroupBox"));
    QGridLayout* basicControlGrid = new QGridLayout(basicControlGroupBox);
    basicControlGrid->setContentsMargins(10, 10, 10, 10);
    basicControlGrid->setSpacing(6);

    const QSize basicBtnMinSize(132, 32);
    const auto styleBasicButton = [basicBtnMinSize](QPushButton* btn) {
        btn->setMinimumSize(basicBtnMinSize);
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    };

    powerOnPushButton = new QPushButton(tr("开机"), basicControlGroupBox);
    powerOnPushButton->setObjectName(QStringLiteral("powerOnPushButton"));
    styleBasicButton(powerOnPushButton);
    basicControlGrid->addWidget(powerOnPushButton, 0, 0);

    powerOffPushButton = new QPushButton(tr("关机"), basicControlGroupBox);
    powerOffPushButton->setObjectName(QStringLiteral("powerOffPushButton"));
    styleBasicButton(powerOffPushButton);
    basicControlGrid->addWidget(powerOffPushButton, 0, 1);

    spParamQueryPushButton = new QPushButton(tr("信处参数查询"), basicControlGroupBox);
    spParamQueryPushButton->setObjectName(QStringLiteral("spParamQueryPushButton"));
    styleBasicButton(spParamQueryPushButton);
    basicControlGrid->addWidget(spParamQueryPushButton, 1, 0);

    dpParamQueryPushButton = new QPushButton(tr("数处参数查询"), basicControlGroupBox);
    dpParamQueryPushButton->setObjectName(QStringLiteral("dpParamQueryPushButton"));
    styleBasicButton(dpParamQueryPushButton);
    basicControlGrid->addWidget(dpParamQueryPushButton, 1, 1);
    basicControlGrid->setColumnStretch(0, 1);
    basicControlGrid->setColumnStretch(1, 1);

    // ——— 雷达工作模式
    QLabel* modeLabel = new QLabel(tr("雷达模式"), basicControlGroupBox);
    modeLabel->setObjectName(QStringLiteral("radarModeLabel"));
    basicControlGrid->addWidget(modeLabel, 2, 0, Qt::AlignLeft | Qt::AlignVCenter);

    radarModeComboBox = new QComboBox(basicControlGroupBox);
    radarModeComboBox->setObjectName(QStringLiteral("radarModeComboBox"));
    radarModeComboBox->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    radarModeComboBox->setMinimumContentsLength(12);
    radarModeComboBox->setToolTip(tr("选择要应用的雷达模式"));
    basicControlGrid->addWidget(radarModeComboBox, 2, 1);

    // 模式应用：整行铺满，与基础控制其他按钮同风格
    modeApplyPushButton = new QPushButton(tr("模式应用"), basicControlGroupBox);
    modeApplyPushButton->setObjectName(QStringLiteral("modeApplyPushButton"));
    modeApplyPushButton->setToolTip(tr("把选中的雷达模式下发到雷达"));
    modeApplyPushButton->setMinimumHeight(basicBtnMinSize.height());
    modeApplyPushButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    basicControlGrid->addWidget(modeApplyPushButton, 3, 0, 1, 2);

    connect(modeApplyPushButton, &QPushButton::clicked, this, &ControlPanel::onModeApplyClicked);
    connect(radarModeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
        [this](int) { onModeSelectionChanged();
    });

    for (int i = 0; i < ModeCount; ++i)
    {
        radarModeComboBox->addItem(tr("模式%1").arg(i));
    }
    radarModeComboBox->setEnabled(true);
    radarModeComboBox->setCurrentIndex(0);
    modeApplyPushButton->setEnabled(true);

    contentLayout->addWidget(basicControlGroupBox);

    // ——— 阵地信息设置（可折叠，默认折叠；自带下发按钮）———
    locationGroupBox = new CollapsibleGroupBox(tr("阵地信息设置"), content);
    locationGroupBox->setObjectName(QStringLiteral("LocationGroupBox"));
    QVBoxLayout* locationLayout = makeSectionLayout(locationGroupBox->contentWidget());

    QGridLayout* locationGrid = new QGridLayout;
    NorthCorrectionLineEdit = addLineEdit(locationGrid, 0, 0, "NorthCorrectionLineEdit", tr("偏北修正(°)"));
    ElevationCorrectionLineEdit = addLineEdit(locationGrid, 0, 2, "ElevationCorrectionLineEdit", tr("俯仰修正(°)"));
    RangeCorrectionLineEdit = addLineEdit(locationGrid, 1, 0, "RangeCorrectionLineEdit", tr("斜距修正(m)"));
    RadarHeightLineEdit = addLineEdit(locationGrid, 1, 2, "RadarHeightLineEdit", tr("雷达高度(m)"));
    RadarLongitudeLineEdit = addLineEdit(locationGrid, 2, 0, "RadarLongitudeLineEdit", tr("雷达经度(°)"));
    RadarLatitudeLineEdit = addLineEdit(locationGrid, 2, 2, "RadarLatitudeLineEdit", tr("雷达纬度(°)"));
    locationGrid->setColumnStretch(1, 1);
    locationGrid->setColumnStretch(3, 1);
    locationLayout->addLayout(locationGrid);

    locationApplyPushButton = new QPushButton(tr("应用阵地信息"), locationGroupBox->contentWidget());
    locationApplyPushButton->setObjectName(QStringLiteral("locationApplyPushButton"));
    locationApplyPushButton->setToolTip(tr("只下发本分组（阵地信息）的参数"));
    locationLayout->addWidget(locationApplyPushButton);
    connect(locationApplyPushButton, &QPushButton::clicked, this,
            [this]() { emit parameterGroupApplyRequested(static_cast<int>(ParamGroup::Location)); });

    contentLayout->addWidget(locationGroupBox);

    // ——— 扇区设置（可折叠，默认折叠；自带下发按钮）———
    scanAreaGroupBox = new CollapsibleGroupBox(tr("扇区设置"), content);
    scanAreaGroupBox->setObjectName(QStringLiteral("ScanAreaGroupBox"));
    QVBoxLayout* scanAreaLayout = makeSectionLayout(scanAreaGroupBox->contentWidget());

    QGridLayout* scanGrid = new QGridLayout;
    StartScan0LineEdit = addLineEdit(scanGrid, 0, 0, "StartScan0LineEdit", tr("起始扇区0(°)"));
    EndScan0LineEdit = addLineEdit(scanGrid, 0, 2, "EndScan0LineEdit", tr("结束扇区0(°)"));
    StartScan1LineEdit = addLineEdit(scanGrid, 1, 0, "StartScan1LineEdit", tr("起始扇区1(°)"));
    EndScan1LineEdit = addLineEdit(scanGrid, 1, 2, "EndScan1LineEdit", tr("结束扇区1(°)"));
    StartScan2LineEdit = addLineEdit(scanGrid, 2, 0, "StartScan2LineEdit", tr("起始扇区2(°)"));
    EndScan2LineEdit = addLineEdit(scanGrid, 2, 2, "EndScan2LineEdit", tr("结束扇区2(°)"));
    StartScan3LineEdit = addLineEdit(scanGrid, 3, 0, "StartScan3LineEdit", tr("起始扇区3(°)"));
    EndScan3LineEdit = addLineEdit(scanGrid, 3, 2, "EndScan3LineEdit", tr("结束扇区3(°)"));
    scanGrid->setColumnStretch(1, 1);
    scanGrid->setColumnStretch(3, 1);
    scanAreaLayout->addLayout(scanGrid);

    scanAreaApplyPushButton = new QPushButton(tr("应用扇区设置"), scanAreaGroupBox->contentWidget());
    scanAreaApplyPushButton->setObjectName(QStringLiteral("scanAreaApplyPushButton"));
    scanAreaApplyPushButton->setToolTip(tr("只下发本分组（扇区设置）的参数"));
    scanAreaLayout->addWidget(scanAreaApplyPushButton);
    connect(scanAreaApplyPushButton, &QPushButton::clicked, this,
            [this]() { emit parameterGroupApplyRequested(static_cast<int>(ParamGroup::ScanArea)); });

    contentLayout->addWidget(scanAreaGroupBox);

    // ——— 目标起批限制（可折叠，默认折叠；自带下发按钮）———
    trackLimitGroupBox = new CollapsibleGroupBox(tr("目标起批限制"), content);
    trackLimitGroupBox->setObjectName(QStringLiteral("TrackLimitGroupBox"));
    QVBoxLayout* trackLimitLayout = makeSectionLayout(trackLimitGroupBox->contentWidget());

    QGridLayout* limitGrid = new QGridLayout;
    SpeedMinLineEdit = addLineEdit(limitGrid, 0, 0, "SpeedMinLineEdit", tr("速度下限(m/s)"));
    SpeedMaxLineEdit = addLineEdit(limitGrid, 0, 2, "SpeedMaxLineEdit", tr("速度上限(m/s)"));
    HeightMinLineEdit = addLineEdit(limitGrid, 1, 0, "HeightMinLineEdit", tr("高度下限(m)"));
    HeightMaxLineEdit = addLineEdit(limitGrid, 1, 2, "HeightMaxLineEdit", tr("高度上限(m)"));
    DistanceMinLineEdit = addLineEdit(limitGrid, 2, 0, "DistanceMinLineEdit", tr("距离下限(m)"));
    DistanceMaxLineEdit = addLineEdit(limitGrid, 2, 2, "DistanceMaxLineEdit", tr("距离上限(m)"));
    limitGrid->setColumnStretch(1, 1);
    limitGrid->setColumnStretch(3, 1);
    trackLimitLayout->addLayout(limitGrid);

    trackLimitApplyPushButton = new QPushButton(tr("应用起批限制"), trackLimitGroupBox->contentWidget());
    trackLimitApplyPushButton->setObjectName(QStringLiteral("trackLimitApplyPushButton"));
    trackLimitApplyPushButton->setToolTip(tr("只下发本分组（目标起批限制）的参数"));
    trackLimitLayout->addWidget(trackLimitApplyPushButton);
    connect(trackLimitApplyPushButton, &QPushButton::clicked, this,
            [this]() { emit parameterGroupApplyRequested(static_cast<int>(ParamGroup::TrackLimit)); });

    contentLayout->addWidget(trackLimitGroupBox);

    contentLayout->addItem(new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding));
    controlScrollArea->setWidget(content);
    pageLayout->addWidget(controlScrollArea);
}

/// <summary>
/// @brief 模式应用：把当前选中的模式值交给主窗口，按开关机流程确认并下发
/// </summary>
void ControlPanel::onModeApplyClicked()
{
    const int modeIndex = radarModeComboBox ? radarModeComboBox->currentIndex() : -1;
    if (modeIndex < 0 || modeIndex >= ModeCount)
    {
        return;
    }
    emit radarModeApplyRequested(modeIndex);
}

void ControlPanel::onModeSelectionChanged()
{
    setModeControlsEnabled(radarModeComboBox && radarModeComboBox->currentIndex() >= 0);
}

void ControlPanel::setModeControlsEnabled(bool selectionValid)
{
    if (modeApplyPushButton)
    {
        modeApplyPushButton->setEnabled(selectionValid);
    }
}

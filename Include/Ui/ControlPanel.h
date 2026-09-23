#pragma once
#include <QString>
#include <QWidget>

class CollapsibleGroupBox;
class QComboBox;
class QPushButton;
class QLineEdit;
class QScrollArea;

class ControlPanel : public QWidget
{
    Q_OBJECT
public:
    /// 模式选择项数量
    static constexpr int ModeCount = 8;

    ControlPanel(QWidget* parent = nullptr);

    /// <summary>
    /// @brief 基础控制
    /// </summary>
    QPushButton* powerOnPushButton = nullptr;
    QPushButton* powerOffPushButton = nullptr;

    /// <summary>
    /// @brief 参数查询按钮
    /// </summary>
    QPushButton* spParamQueryPushButton = nullptr;
    QPushButton* dpParamQueryPushButton = nullptr;

    /// <summary>
    /// @brief 分组级参数下发按钮
    /// </summary>
    enum class ParamGroup
    {
        Location = 0,  /// 阵地信息设置
        ScanArea = 1,  /// 扇区设置
        TrackLimit = 2 /// 目标起批限制
    };

    /// <summary>
    /// @brief 雷达工作模式设置
    /// </summary>
    QComboBox* radarModeComboBox = nullptr;
    QPushButton* modeApplyPushButton = nullptr;

    /// <summary>
    /// @brief 可折叠分组的容器与各自的参数下发按钮
    /// </summary>
    CollapsibleGroupBox* locationGroupBox = nullptr;
    CollapsibleGroupBox* scanAreaGroupBox = nullptr;
    CollapsibleGroupBox* trackLimitGroupBox = nullptr;
    QPushButton* locationApplyPushButton = nullptr;
    QPushButton* scanAreaApplyPushButton = nullptr;
    QPushButton* trackLimitApplyPushButton = nullptr;

    /// <summary>
    /// @brief 阵地信息设置
    /// </summary>
    QLineEdit* NorthCorrectionLineEdit = nullptr;
    QLineEdit* ElevationCorrectionLineEdit = nullptr;
    QLineEdit* RangeCorrectionLineEdit = nullptr;
    QLineEdit* RadarLongitudeLineEdit = nullptr;
    QLineEdit* RadarLatitudeLineEdit = nullptr;
    QLineEdit* RadarHeightLineEdit = nullptr;

    /// <summary>
    /// @brief 扇区设置
    /// </summary>
    QLineEdit* StartScan0LineEdit = nullptr;
    QLineEdit* EndScan0LineEdit = nullptr;
    QLineEdit* StartScan1LineEdit = nullptr;
    QLineEdit* EndScan1LineEdit = nullptr;
    QLineEdit* StartScan2LineEdit = nullptr;
    QLineEdit* EndScan2LineEdit = nullptr;
    QLineEdit* StartScan3LineEdit = nullptr;
    QLineEdit* EndScan3LineEdit = nullptr;

    /// <summary>
    /// @brief 目标起批限制
    /// </summary>
    QLineEdit* SpeedMinLineEdit = nullptr;
    QLineEdit* SpeedMaxLineEdit = nullptr;
    QLineEdit* HeightMinLineEdit = nullptr;
    QLineEdit* HeightMaxLineEdit = nullptr;
    QLineEdit* DistanceMinLineEdit = nullptr;
    QLineEdit* DistanceMaxLineEdit = nullptr;
signals:
    /// <summary>
    /// @brief 请求应用选中模式
    /// </summary>
    void radarModeApplyRequested(int modeIndex);

    /// <summary>
    /// @brief 请求下发某个分组的参数
    /// </summary>
    void parameterGroupApplyRequested(int group);
private:
    QScrollArea* controlScrollArea = nullptr;
    void onModeApplyClicked();
    void onModeSelectionChanged();
    void setModeControlsEnabled(bool selectionValid);
};

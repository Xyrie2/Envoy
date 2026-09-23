#pragma once
#include <QColor>
#include <QPalette>
#include <QString>

namespace Theme
{

namespace Pal
{

inline QColor windowBg()
{
    return QColor(0x0b, 0x12, 0x20);
}

inline QColor surface()
{
    return QColor(0x11, 0x1a, 0x2c);
}

inline QColor surfaceAlt()
{
    return QColor(0x16, 0x20, 0x36);
}

inline QColor surfaceHover()
{
    return QColor(0x1d, 0x2a, 0x47);
}

inline QColor border()
{
    return QColor(0x26, 0x33, 0x50);
}

inline QColor borderStrong()
{
    return QColor(0x36, 0x47, 0x70);
}

inline QColor textPrimary()
{
    return QColor(0xdb, 0xe4, 0xf3);
}

inline QColor textMuted()
{
    return QColor(0x82, 0x96, 0xb5);
}

inline QColor accent()
{
    return QColor(0x3b, 0x82, 0xf6);
}

inline QColor accentHover()
{
    return QColor(0x60, 0xa5, 0xfa);
}

inline QColor selBg()
{
    return QColor(0x1d, 0x3a, 0x67);
}

inline QColor selText()
{
    return QColor(0xea, 0xf2, 0xff);
}

inline QColor statusIdle()
{
    return QColor(0x82, 0x96, 0xb5);
}

inline QColor statusLinked()
{
    return QColor(0x34, 0xd3, 0x99);
}

inline QColor statusLost()
{
    return QColor(0xf8, 0x71, 0x71);
}

inline QColor closeRed()
{
    return QColor(0xe8, 0x11, 0x23);
}

inline QColor mapBg()
{
    return QColor(0x0a, 0x10, 0x1d);
}

inline QColor mapRing()
{
    return QColor(0x7f, 0xdc, 0xe8);
}

inline QColor mapSpoke()
{
    return QColor(0x3f, 0x59, 0x86);
}

inline QColor mapTick()
{
    return QColor(0x25, 0x34, 0x53);
}

inline QColor mapLabel()
{
    return QColor(0x82, 0x96, 0xb5);
}

inline QColor mapRadar()
{
    return QColor(0x3b, 0x82, 0xf6);
}

inline QColor mapSector()
{
    return QColor(59, 130, 246, 30);
}

inline QColor mapFg()
{
    return QColor(0xdb, 0xe4, 0xf3);
}

inline QColor mapPoint()
{
    return QColor(0xdc, 0xec, 0xff);
}

inline QColor floatBg()
{
    return QColor(0x16, 0x20, 0x36, 242);
}

inline QColor floatBorder()
{
    return QColor(0x36, 0x47, 0x70);
}

}

inline QString hex(const QColor& c)
{
    return c.name();
}

constexpr int WindowRadius = 8;
constexpr int MaskRadius = WindowRadius + 2;

inline QString appQss()
{
    return QStringLiteral(
                "QWidget { color:%TextPrimary%; }"
                "QWidget#centralwidget { background:%WindowBg%; }"

                "QGroupBox { border:1px solid %Border%; border-radius:8px; margin-top:14px;"
                "  padding:0px; background:%Surface%; font-weight:600; }"
                "QGroupBox::title { subcontrol-origin:margin; left:10px; padding:0 6px; color:%TextMuted%; }"

                "QTabWidget::pane { border:1px solid %Border%; border-radius:6px; top:-1px; background:%Surface%; }"
                "QTabBar { font-weight:600; }"
                "QTabBar::tab { padding:8px 22px; background:%SurfaceAlt%; border:1px solid %Border%;"
                "  border-bottom:none; border-top-left-radius:5px; border-top-right-radius:5px;"
                "  margin-right:2px; color:%TextMuted%; }"
                "QTabBar::tab:selected { background:%Surface%; color:%Accent%;"
                "  border-color:%BorderStrong%; }"
                "QTabBar::tab:hover:!selected { background:%SurfaceHover%; }"

                "QTableWidget { gridline-color:%BorderStrong%;"
                "  border-top:1px solid %BorderStrong%; border-left:1px solid %BorderStrong%;"
                "  border-right:1px solid %BorderStrong%; border-bottom:none;"
                "  background:%Surface%; alternate-background-color:%SurfaceAlt%;"
                "  selection-background-color:%SelBg%; selection-color:%SelText%; }"
                "QTableWidget#trackTableWidget { border-bottom:1px solid %BorderStrong%; }"
                "QTableWidget#channelTable { border-bottom:1px solid %BorderStrong%; }"
                "QHeaderView::section { background:%SurfaceAlt%; color:%TextMuted%; padding:4px 6px;"
                "  border:none; border-right:1px solid %Border%; border-bottom:1px solid %Border%;"
                "  font-weight:600; }"

                "QTextEdit, QPlainTextEdit { background:%WindowBg%; color:%TextMuted%;"
                "  font-family:\"Consolas\",\"Courier New\",\"Microsoft YaHei\",monospace; font-size:9pt;"
                "  border:1px solid %Border%; }"

                "QLineEdit, QDoubleSpinBox, QSpinBox, QComboBox { padding:2px 4px;"
                "  background:%SurfaceAlt%; border:1px solid %Border%; border-radius:4px;"
                "  color:%TextPrimary%; selection-background-color:%SelBg%; selection-color:%SelText%; }"
                "QLineEdit:focus, QDoubleSpinBox:focus, QSpinBox:focus, QComboBox:focus"
                "  { border:1px solid %Accent%; }"
                "QComboBox QAbstractItemView { background:%SurfaceAlt%; color:%TextPrimary%;"
                "  border:1px solid %BorderStrong%; selection-background-color:%SelBg%;"
                "  selection-color:%SelText%; }"

                "QSpinBox::up-button, QSpinBox::down-button { background:%SurfaceHover%;"
                "  border:1px solid %BorderStrong%; width:16px; margin:1px; }"
                "QSpinBox::up-button { subcontrol-origin: border; subcontrol-position: top right;"
                "  border-top-right-radius:4px; }"
                "QSpinBox::down-button { subcontrol-origin: border; subcontrol-position: bottom right;"
                "  border-bottom-right-radius:4px; }"
                "QSpinBox::up-button:hover, QSpinBox::down-button:hover { background:%Accent%;"
                "  border-color:%AccentHover%; }"
                "QSpinBox::up-button:pressed, QSpinBox::down-button:pressed { background:%SelBg%; }"
                "QSpinBox::up-arrow { border-left:3px solid transparent; border-right:3px solid transparent;"
                "  border-bottom:4px solid %TextPrimary%; width:0; height:0; }"
                "QSpinBox::down-arrow { border-left:3px solid transparent; border-right:3px solid transparent;"
                "  border-top:4px solid %TextPrimary%; width:0; height:0; }"
                "QSpinBox::up-arrow:hover { border-bottom-color:#ffffff; }"
                "QSpinBox::down-arrow:hover { border-top-color:#ffffff; }"

                "QSlider { background:transparent; }"
                "QCheckBox::indicator { width:14px; height:14px; border:1px solid %BorderStrong%;"
                "  border-radius:3px; background:%SurfaceAlt%; }"
                "QCheckBox::indicator:hover { border:1px solid %AccentHover%; }"
                "QCheckBox::indicator:checked { background:%Accent%; border:1px solid %AccentHover%; }"
                "QSlider::groove:horizontal { height:4px; background:%SurfaceAlt%;"
                "  border:1px solid %Border%; border-radius:2px; }"
                "QSlider::sub-page:horizontal { background:%Accent%; border:1px solid %AccentHover%;"
                "  border-radius:2px; }"
                "QSlider::handle:horizontal { width:11px; margin:-5px 0; border-radius:5px;"
                "  background:%TextPrimary%; border:none; }"
                "QSlider::handle:horizontal:hover { background:#ffffff; }"
                "QLabel#trailAlphaValueLabel { color:%TextMuted%; }"

                "QPushButton { padding:6px 14px; border-radius:5px; background:%SurfaceAlt%;"
                "  border:1px solid %BorderStrong%; color:%TextPrimary%; }"
                "QPushButton:hover { background:%SurfaceHover%; }"
                "QPushButton:pressed { background:%WindowBg%; }"
                "QPushButton:disabled { color:%TextMuted%; border-color:%Border%; }"
                "QPushButton[cellAction=\"true\"] { padding:3px 8px; }"
                "QPushButton#applyNetworkPushButton, QPushButton#modeApplyPushButton,"
                "QPushButton#locationApplyPushButton, QPushButton#scanAreaApplyPushButton, QPushButton#trackLimitApplyPushButton"
                " { background:%Accent%; color:#ffffff; font-weight:600; border:none; }"
                "QPushButton#applyNetworkPushButton:hover, QPushButton#modeApplyPushButton:hover,"
                "QPushButton#locationApplyPushButton:hover, QPushButton#scanAreaApplyPushButton:hover, QPushButton#trackLimitApplyPushButton:hover"
                " { background:%AccentHover%; }"
                "QPushButton#modeListButton, QPushButton#modeFocusButton { padding:5px 16px; border-radius:0;"
                "  background:%SurfaceAlt%; font-weight:600; }"
                "QPushButton#modeListButton { border-top-left-radius:5px; border-bottom-left-radius:5px; }"
                "QPushButton#modeFocusButton { border-top-right-radius:5px; border-bottom-right-radius:5px;"
                "  border-left:none; }"
                "QPushButton#modeListButton:checked, QPushButton#modeFocusButton:checked { background:%Accent%;"
                "  color:#ffffff; border-color:%Accent%; }"
                "QPushButton#modeListButton:hover:!checked, QPushButton#modeFocusButton:hover:!checked {"
                "  background:%SurfaceHover%; }"
                "QToolButton { background:transparent; border:none; color:%TextMuted%; padding:3px 8px;"
                "  border-radius:4px; }"
                "QToolButton:hover { background:%SurfaceHover%; color:%TextPrimary%; }"

                "QLabel#statsLabel { background:%SelBg%; border:1px solid %BorderStrong%;"
                "  border-radius:10px; padding:3px 12px; color:%AccentHover%; font-weight:600; }"

                "QSplitter::handle { background:%Border%; width:5px; }"
                "QSplitter::handle:hover { background:%Accent%; }"

                "QScrollArea { border:none; background:transparent; }"
                "QScrollBar:vertical { background:transparent; width:9px; margin:0; }"
                "QScrollBar::handle:vertical { background:%SurfaceAlt%; border-radius:4px; min-height:28px; }"
                "QScrollBar::handle:vertical:hover { background:%BorderStrong%; }"
                "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height:0; }"
                "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background:transparent; }"
                "QScrollBar:horizontal { background:transparent; height:9px; margin:0; }"
                "QScrollBar::handle:horizontal { background:%SurfaceAlt%; border-radius:4px; min-width:28px; }"
                "QScrollBar::handle:horizontal:hover { background:%BorderStrong%; }"
                "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width:0; }"
                "QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal { background:transparent; }"

                "QToolTip { background:%SurfaceAlt%; color:%TextPrimary%; border:1px solid %BorderStrong%;"
                "  padding:3px 6px; }"

                "QFrame#appStatusBar { background:#0e1626; border-top:1px solid %Border%;"
                "  border-bottom-left-radius:%Radius%; border-bottom-right-radius:%Radius%; }"
                "QFrame#appStatusBar QLabel { color:%TextMuted%; font-weight:600; background:transparent; }"
                "QToolButton#panelToggleBtn { color:%TextMuted%; border:1px solid %Border%;"
                "  border-radius:4px; padding:2px 10px; background:%SurfaceAlt%; font-weight:600; }"
                "QToolButton#panelToggleBtn:hover { background:%SurfaceHover%; color:%TextPrimary%;"
                "  border-color:%BorderStrong%; }"
                "QToolButton#trackToggleBtn { color:%TextMuted%; border:1px solid %Border%;"
                "  border-radius:4px; padding:2px 10px; background:%SurfaceAlt%; font-weight:600; }"
                "QToolButton#trackToggleBtn:hover { background:%SurfaceHover%; color:%TextPrimary%;"
                "  border-color:%BorderStrong%; }"
                "QToolButton#columnSettingsButton { color:%TextMuted%; border:1px solid %Border%;"
                "  border-radius:4px; padding:2px 10px; background:%SurfaceAlt%; font-weight:600; }"
                "QToolButton#columnSettingsButton:hover { background:%SurfaceHover%; color:%TextPrimary%;"
                "  border-color:%BorderStrong%; }"
                "QToolButton#columnSettingsButton::menu-indicator { image:none; width:0px; }"

                "QDialog#channelEditDialog, QDialog#appMessageDialog { background:transparent; }"
                "QFrame#channelEditDialogCard, QFrame[dialogCard=\"true\"] { background:%Surface%;"
                "  border:1px solid %BorderStrong%; border-radius:%Radius%px; }"
                "QWidget#sectionHeader { background:transparent; }"
                "QLabel#sectionTitle { color:%TextPrimary%; font-weight:700; font-size:13px; }"
                "QFrame#sectionDivider { background:%Border%; border:none; max-height:1px; }"

                "QLabel#dlgFieldLabel { color:%TextMuted%; font-weight:600; }"
                "QLabel#dlgStaticValue { color:%TextPrimary%; }"
                "QLabel#msgDialogText { color:%TextPrimary%; }"
                "QLabel#dlgErrorText { color:%StatusLost%; }"
                "QFrame#dlgFooter { background:transparent; border:none; border-top:1px solid %Border%; }"
                "QPushButton#dlgPrimaryButton { background:%Accent%; color:#ffffff; font-weight:600;"
                "  border:1px solid %Accent%; padding:6px 20px; min-width:84px; max-width:84px; }"
                "QPushButton#dlgPrimaryButton:hover { background:%AccentHover%; border-color:%AccentHover%; }"
                "QPushButton#dlgPrimaryButton:pressed { background:%SelBg%; border-color:%SelBg%; }"
                "QPushButton#dlgSecondaryButton { padding:6px 20px; min-width:84px; max-width:84px; }"

                "QMenu { background:%SurfaceAlt%; color:%TextPrimary%; border:1px solid %BorderStrong%;"
                "  padding:4px; }"
                "QMenu::item { color:%TextPrimary%; padding:4px 20px 4px 26px; border-radius:4px; }"
                "QMenu::item:selected { background:%SelBg%; color:%SelText%; }"
                "QMenu::item:disabled { color:%TextMuted%; }"
                "QMenu::separator { height:1px; background:%Border%; margin:3px 6px; }")
            .replace("%WindowBg%", hex(Pal::windowBg()))
            .replace("%Surface%", hex(Pal::surface()))
            .replace("%SurfaceAlt%", hex(Pal::surfaceAlt()))
            .replace("%SurfaceHover%", hex(Pal::surfaceHover()))
            .replace("%Border%", hex(Pal::border()))
            .replace("%BorderStrong%", hex(Pal::borderStrong()))
            .replace("%TextPrimary%", hex(Pal::textPrimary()))
            .replace("%TextMuted%", hex(Pal::textMuted()))
            .replace("%AccentHover%", hex(Pal::accentHover()))
            .replace("%Accent%", hex(Pal::accent()))
            .replace("%SelBg%", hex(Pal::selBg()))
            .replace("%SelText%", hex(Pal::selText()))
            .replace("%StatusLost%", hex(Pal::statusLost()))
            .replace("%Radius%", QString::number(WindowRadius));
}

inline QPalette darkPalette()
{
    QPalette pal;
    pal.setColor(QPalette::Window, Pal::windowBg());
    pal.setColor(QPalette::WindowText, Pal::textPrimary());
    pal.setColor(QPalette::Base, Pal::surface());
    pal.setColor(QPalette::AlternateBase, Pal::surfaceAlt());
    pal.setColor(QPalette::Text, Pal::textPrimary());
    pal.setColor(QPalette::Button, Pal::surfaceAlt());
    pal.setColor(QPalette::ButtonText, Pal::textPrimary());
    pal.setColor(QPalette::ToolTipBase, Pal::surfaceAlt());
    pal.setColor(QPalette::ToolTipText, Pal::textPrimary());
    pal.setColor(QPalette::Highlight, Pal::selBg());
    pal.setColor(QPalette::HighlightedText, Pal::selText());
    pal.setColor(QPalette::PlaceholderText, Pal::textMuted());
    pal.setColor(QPalette::Disabled, QPalette::Text, Pal::textMuted());
    pal.setColor(QPalette::Disabled, QPalette::ButtonText, Pal::textMuted());
    pal.setColor(QPalette::Disabled, QPalette::WindowText, Pal::textMuted());
    return pal;
}
}

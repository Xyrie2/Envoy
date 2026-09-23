#include "App/MainWindow.h"
#include "Ui/AppMessageDialog.h"
#include "Ui/FramelessWindow.h"
#include "Ui/Theme.h"
#include "Ui/TitleBar.h"

#include <QApplication>
#include <QDir>
#include <QFont>
#include <QIcon>
#include <QLocalServer>
#include <QLocalSocket>
#include <QLockFile>
#include <QScreen>
#include <QSettings>
#include <QStyle>
#include <QStyleFactory>

namespace {

constexpr int kDefaultWidth = 1280;
constexpr int kDefaultHeight = 820;

const QString kSingleInstanceServer = QStringLiteral("RadarDemoSingleInstanceGuard");
const QString kSingleInstanceLock = QStringLiteral("RadarDemoSingleInstance.lock");
const QString kWinGeometryKey = QStringLiteral("win/geometry");
const QString kWinMaximizedKey = QStringLiteral("win/maximized");

void applyTheme(QApplication& app)
{
    if (auto* fusion = QStyleFactory::create(QStringLiteral("Fusion"))) {
        app.setStyle(fusion);
    }

    app.setPalette(Theme::darkPalette());
    app.setStyleSheet(Theme::appQss());

    QFont f = app.font();
    f.setPointSize(9);
    app.setFont(f);
}

void centerOnPrimaryScreen(QWidget& widget, int width, int height)
{
    widget.resize(width, height);

    if (QScreen* screen = QGuiApplication::primaryScreen()) {
        const QRect available = screen->availableGeometry();
        widget.move(available.center() - QPoint(width / 2, height / 2));
    }
}

bool isVisibleOnAnyScreen(const QWidget& widget)
{
    const QRect frame = widget.frameGeometry();
    if (!frame.isValid()) {
        return false;
    }

    const auto screens = QGuiApplication::screens();
    for (QScreen* screen : screens) {
        if (screen->availableGeometry().intersects(frame)) {
            return true;
        }
    }
    return false;
}

bool notifyExistingInstance()
{
    QLocalSocket socket;
    socket.connectToServer(kSingleInstanceServer);

    if (!socket.waitForConnected(300)) {
        return false;
    }

    socket.write("show");
    socket.flush();
    socket.waitForBytesWritten(200);
    return true;
}

void setupSingleInstanceServer(QApplication& app, QLocalServer* server, FramelessWindow& shell)
{
    QObject::connect(server, &QLocalServer::newConnection, &app, [server, &shell]() {
        while (QLocalSocket* socket = server->nextPendingConnection()) {
            QObject::connect(socket, &QLocalSocket::readyRead, socket, [socket, &shell]() {
                const QByteArray data = socket->readAll();
                if (!data.contains("show")) {
                    return;
                }

                if (shell.isMinimized()) {
                    shell.showNormal();
                }

                if (shell.windowMaximized()) {
                    shell.showPseudoMaximized();
                } else {
                    shell.showNormal();
                }

                shell.raise();
                shell.activateWindow();
            });

            QObject::connect(socket, &QLocalSocket::disconnected, socket, &QObject::deleteLater);
        }
    });
}

void restoreWindowState(FramelessWindow& shell, QSettings& settings)
{
    bool restored = false;

    const QByteArray geometry = settings.value(kWinGeometryKey).toByteArray();
    if (!geometry.isEmpty()) {
        restored = shell.restoreGeometry(geometry);
    }

    const bool visible = isVisibleOnAnyScreen(shell);
    const bool tooSmall = shell.width() < FramelessWindow::MinWidth || shell.height() < FramelessWindow::MinHeight;

    if (!restored || !visible || (!shell.windowMaximized() && tooSmall)) {
        centerOnPrimaryScreen(shell, kDefaultWidth, kDefaultHeight);
    }

    const bool maximized = settings.value(kWinMaximizedKey, false).toBool();
    if (maximized) {
        shell.showPseudoMaximized();
    } else {
        shell.showNormal();
    }
}

void saveWindowState(FramelessWindow& shell, QSettings& settings)
{
    const bool wasMaximized = shell.windowMaximized();

    shell.restoreGeometryForSave();
    settings.setValue(kWinGeometryKey, shell.saveGeometry());
    settings.setValue(kWinMaximizedKey, wasMaximized);
    settings.sync();
}

}

int main(int argc, char* argv[])
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0) && QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

    QApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("RadarDemo"));
    app.setApplicationName(QStringLiteral("RadarDemo"));

    const QIcon appIcon(QStringLiteral(":/Resources/app.ico"));
    if (!appIcon.isNull()) {
        app.setWindowIcon(appIcon);
    }

    applyTheme(app);

    if (notifyExistingInstance()) {
        AppMessageDialog::Information(
                    nullptr,
                    QObject::tr("已在运行"),
                    QObject::tr("程序已在运行，请勿重复启动。"));
        return 0;
    }

    QLockFile singleInstanceLock(QDir(QDir::tempPath()).filePath(kSingleInstanceLock));
    if (!singleInstanceLock.tryLock(100)) {
        AppMessageDialog::Information(
                    nullptr,
                    QObject::tr("已在运行"),
                    QObject::tr("程序已在运行，请勿重复启动。"));
        return 0;
    }

    QLocalServer::removeServer(kSingleInstanceServer);
    QLocalServer* guardServer = new QLocalServer(&app);
    if (!guardServer->listen(kSingleInstanceServer)) {
        qWarning() << "single-instance guard listen failed:" << guardServer->errorString();
    }

    FramelessWindow shell;
    shell.setWindowTitle(QObject::tr("Envoy"));

    MainWindow* mainPanel = new MainWindow(&shell);
    shell.setContentWidget(mainPanel);

    TitleBar* titleBar = new TitleBar(&shell);
    shell.setTitleBar(titleBar);

    QObject::connect(titleBar, &TitleBar::minimizeClicked, &shell, &QWidget::showMinimized);
    QObject::connect(titleBar, &TitleBar::maximizeToggled, &shell, [&shell]() {
        shell.toggleMaximize();
    });
    QObject::connect(titleBar, &TitleBar::closeClicked, &shell, &QWidget::close);

    if (guardServer->isListening()) {
        setupSingleInstanceServer(app, guardServer, shell);
    }

    QSettings settings;
    restoreWindowState(shell, settings);

    QObject::connect(&app, &QCoreApplication::aboutToQuit, [&shell, &settings]() {
        saveWindowState(shell, settings);
    });

    return app.exec();
}

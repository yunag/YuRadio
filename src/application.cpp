#include <QLoggingCategory>
Q_LOGGING_CATEGORY(applicationLog, "YuRadio.Application")

#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>

#ifdef Q_OS_ANDROID
  #include "android/nativemediacontroller.h"
  #include "android/notificationclient.h"
  #include "android/virtualkeyboardlistener.h"
#endif /* Q_OS_ANDROID */

#ifdef QT_DEBUG
  #include "hotreloaderclient.h"
#endif /* QT_DEBUG */

#include <QSslSocket>
#include <QThread>

#include "network/networkmanagerfactory.h"

#ifdef UIOHOOK_SUPPORTED
  #include "globalkeylistener.h"
  #include "radioplayer.h"
#endif /* !UIOHOOK_SUPPORTED */

using namespace Qt::StringLiterals;

#include "application.h"

Application::Application(int argc, char **argv) : QGuiApplication(argc, argv) {
  /* Format debug messages */
  qSetMessagePattern(
    u"[%{time yyyy/MM/dd h:mm:ss.zzz} "
    "%{if-debug}D%{endif}%{if-info}I%{endif}%{if-warning}W%{endif}%{if-"
    "critical}C%{endif}%{if-fatal}F%{endif}] %{file}:%{line} - %{message}"_s);

  QLoggingCategory::setFilterRules(
    u"YuRadio.*.debug=true\nYuRadio.CircularBuffer.debug="
    "false\nHotreloader.*.info=false\nYuRadio.IcecastReader.info=false"_s);
  QThread::currentThread()->setObjectName("Main Thread"_L1);

  QCoreApplication::setOrganizationName(u"YuRadio"_s);
  QCoreApplication::setApplicationName(u"YuRadio"_s);
  QCoreApplication::setApplicationVersion(u"1.0"_s);

  qCInfo(applicationLog) << "Version:"
                         << QCoreApplication::applicationVersion();
  qCInfo(applicationLog) << "Device supports OpenSSL:"
                         << QSslSocket::supportsSsl();

  m_engine = std::make_unique<QQmlApplicationEngine>();

  QQuickStyle::setStyle(u"Material"_s);

  auto *networkManagerFactory = new NetworkManagerFactory(m_engine.get());
  m_engine->setNetworkAccessManagerFactory(networkManagerFactory);

#ifdef UIOHOOK_SUPPORTED
  m_engine->rootContext()->setContextProperty("UIOHOOK_SUPPORTED", true);
  auto *player = m_engine->singletonInstance<RadioPlayer *>("YuRadioContents",
                                                            "MainRadioPlayer");
  GlobalKeyListener *listener = GlobalKeyListener::instance();
  QObject::connect(listener, &GlobalKeyListener::keyPressed, player,
                   [player](Qt::Key key) {
    if (key == Qt::Key_MediaPlay || key == Qt::Key_MediaStop ||
        key == Qt::Key_MediaPause) {
      player->toggle();
    }
  });
#else
  m_engine->rootContext()->setContextProperty("UIOHOOK_SUPPORTED", false);
#endif /* UIOHOOK_SUPPORTED */

#ifdef Q_OS_ANDROID
  /* Rename android UI thread*/
  QNativeInterface::QAndroidApplication::runOnAndroidMainThread([]() {
    QThread::currentThread()->setObjectName("Android Main Thread"_L1);
  });

  auto *notificationClient = new NotificationClient(this);
  m_engine->rootContext()->setContextProperty("NotificationClient",
                                              notificationClient);

  NativeMediaController::registerNativeMethods();
  VirtualKeyboardListener::registerNativeMethods();
#endif /* Q_OS_ANDROID */

#ifdef QT_DEBUG
  auto *client = new HotReloaderClient(
    m_engine.get(), u"192.168.1.37"_s, u"/src/Main/Main.qml"_s,
    u"/src/Main/ErrorPage.qml"_s, {u"Main"_s, u"YuRadioContents"_s}, this);
#else
  m_engine->loadFromModule(u"Main"_s, u"Main"_s);
#endif /* QT_DEBUG */
}

Application::~Application() {
#ifdef UIOHOOK_SUPPORTED
  GlobalKeyListener::instance()->cleanup();
#endif
}

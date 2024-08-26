#include <QLoggingCategory>
Q_LOGGING_CATEGORY(globalKeyListenerLog, "YuRadio.GlobalKeyListener")

#include <QThread>

#include <uiohook.h>

#include "globalkeylistener.h"

static inline Qt::Key UioHookKeyToQt(uint16_t keycode) {
  switch (keycode) {
    /* TODO: Add supported keys */
    case VC_MEDIA_PLAY:
      return Qt::Key_MediaPlay;
    case VC_MEDIA_STOP:
      return Qt::Key_MediaStop;
    case VC_MEDIA_NEXT:
      return Qt::Key_MediaNext;
    case VC_MEDIA_PREVIOUS:
      return Qt::Key_MediaPrevious;
    default:
      return Qt::Key_unknown;
  }
}

static void uiohookEventCallback(uiohook_event *const event) {
  GlobalKeyListener *listener = GlobalKeyListener::instance();

  keyboard_event_data &keyboard = event->data.keyboard;
  switch (event->type) {
    case EVENT_KEY_PRESSED:
      emit listener->keyPressed(UioHookKeyToQt(keyboard.keycode));
      qCInfo(globalKeyListenerLog)
        << QString("GlobalKeyPress keycode=%1, rawcode=%2")
             .arg(keyboard.keycode)
             .arg(keyboard.rawcode);
      break;

    case EVENT_KEY_RELEASED:
      emit listener->keyReleased(UioHookKeyToQt(keyboard.keycode));
      qCInfo(globalKeyListenerLog)
        << QString("GlobalKeyRelease keycode=%1, rawcode=%2")
             .arg(keyboard.keycode)
             .arg(keyboard.rawcode);
      break;

    case EVENT_KEY_TYPED:
      emit listener->keyTyped(UioHookKeyToQt(keyboard.keycode));
      qCInfo(globalKeyListenerLog)
        << QString("GlobalKeyType keycode=%1, rawcode=%2")
             .arg(keyboard.keycode)
             .arg(keyboard.rawcode);
      break;
    default:
      break;
  }
}

GlobalKeyListener::GlobalKeyListener(QObject *parent) : QObject(parent) {
  m_thread = std::make_unique<QThread>();
  moveToThread(m_thread.get());

  hook_set_dispatch_proc(&uiohookEventCallback);

  m_thread->start();
  connect(m_thread.get(), &QThread::started, this, &GlobalKeyListener::run);
}

GlobalKeyListener::~GlobalKeyListener() {
  QMetaObject::invokeMethod(this, &GlobalKeyListener::cleanupImpl);
  m_thread->wait();
}

GlobalKeyListener *GlobalKeyListener::instance() {
  static GlobalKeyListener instance;
  return &instance;
}

void GlobalKeyListener::cleanup() {
  QMetaObject::invokeMethod(this, &GlobalKeyListener::cleanupImpl);
}

void GlobalKeyListener::cleanupImpl() {
  int status = hook_stop();
  switch (status) {
    case UIOHOOK_SUCCESS:
      qCInfo(globalKeyListenerLog) << "Exited successfully";
      break;
    case UIOHOOK_ERROR_OUT_OF_MEMORY:
      qCWarning(globalKeyListenerLog) << "Failed to allocate memory";
      break;
    case UIOHOOK_ERROR_X_RECORD_GET_CONTEXT:
      qCWarning(globalKeyListenerLog) << "Failed to get XRecord context";
      break;
    case UIOHOOK_FAILURE:
    default:
      qCWarning(globalKeyListenerLog) << "An unknown hook error occurred";
      break;
  }
  m_thread->quit();
}

void GlobalKeyListener::run() {
  int status = hook_run();
  switch (status) {
    case UIOHOOK_SUCCESS:
      break;

    // System level errors.
    case UIOHOOK_ERROR_OUT_OF_MEMORY:
      qWarning(globalKeyListenerLog) << "Failed to allocate memory";
      break;

    // X11 specific errors.
    case UIOHOOK_ERROR_X_OPEN_DISPLAY:
      qWarning(globalKeyListenerLog) << "Failed to open X11 display";
      break;

    case UIOHOOK_ERROR_X_RECORD_NOT_FOUND:
      qWarning(globalKeyListenerLog) << "Unable to locate XRecord extension";
      break;

    case UIOHOOK_ERROR_X_RECORD_ALLOC_RANGE:
      qWarning(globalKeyListenerLog) << "Unable to allocate XRecord range";
      break;

    case UIOHOOK_ERROR_X_RECORD_CREATE_CONTEXT:
      qWarning(globalKeyListenerLog) << "Unable to allocate XRecord context";
      break;

    case UIOHOOK_ERROR_X_RECORD_ENABLE_CONTEXT:
      qWarning(globalKeyListenerLog) << "Failed to enable XRecord context";
      break;

    // Windows specific errors.
    case UIOHOOK_ERROR_SET_WINDOWS_HOOK_EX:
      qWarning(globalKeyListenerLog) << "Failed to register low level windows "
                                        "hook";
      break;

    // Darwin specific errors.
    case UIOHOOK_ERROR_AXAPI_DISABLED:
      qWarning(globalKeyListenerLog)
        << "Failed to enable access for assistive devices";
      break;

    case UIOHOOK_ERROR_CREATE_EVENT_PORT:
      qWarning(globalKeyListenerLog) << "Failed to create apple event port";
      break;

    case UIOHOOK_ERROR_CREATE_RUN_LOOP_SOURCE:
      qWarning(globalKeyListenerLog)
        << "Failed to create apple run loop source";
      break;

    case UIOHOOK_ERROR_GET_RUNLOOP:
      qWarning(globalKeyListenerLog) << "Failed to acquire apple run loop";
      break;

    case UIOHOOK_ERROR_CREATE_OBSERVER:
      qWarning(globalKeyListenerLog)
        << "Failed to create apple run loop observer";
      break;

    case UIOHOOK_FAILURE:
    default:
      qWarning(globalKeyListenerLog) << "An unknown hook error occurred";
      break;
  }
}
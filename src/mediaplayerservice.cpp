#include "mediaplayerservice.h"

#ifdef Q_OS_ANDROID
#include "android/androidmediaplayerservice.h"
#elif defined(Q_OS_LINUX)
#include "linux/linuxmediaplayerservice.h"
#endif

#include "globalkeylistener.h"

using namespace Qt::StringLiterals;

#ifdef UIOHOOK_SUPPORTED
class BasicMediaPlayerService : public MediaPlayerService {
public:
  explicit BasicMediaPlayerService(RadioPlayer *player)
      : MediaPlayerService(player), toggleShortcut(new GlobalShortcut(this)),
        stopShortcut(new GlobalShortcut(this)) {
    toggleShortcut->setSequence("Media Play");
    stopShortcut->setSequence("Media Stop");

    connect(toggleShortcut, &GlobalShortcut::activated, this, [this]() {
      /* Handle as toggle */
      if (m_player->isPlaying()) {
        m_player->pause();
      } else {
        m_player->play();
      }
    });

    connect(stopShortcut, &GlobalShortcut::activated, this, [this]() {
      m_player->stop();
    });
  }

  ~BasicMediaPlayerService() override = default;

private:
  GlobalShortcut *toggleShortcut;
  GlobalShortcut *stopShortcut;
};
#endif /* DEBUG */

namespace {

QPointer<MediaPlayerService> registeredMediaPlayerService = nullptr;

MediaPlayerService *createMediaPlayerService(RadioPlayer *player) {
#ifdef Q_OS_ANDROID
  return new AndroidMediaPlayerService(player);
#elif defined(Q_OS_LINUX)
  /* Will register org.mpris.MediaPlayer2.yuradio service */
  return new LinuxMediaPlayerService(player, u"yuradio"_s, u"YuRadio"_s);
#elif defined(UIOHOOK_SUPPORTED)
  return new BasicMediaPlayerService(player);
#endif
  /* TODO: Implement SMTC for Windows. 
   * This is not trivial task since 
   * I need to build it as dll with MSVC in Visual Studio...
   * see https://github.com/spmn/vlc-win10smtc */

  return nullptr;
}

}  // namespace

MediaPlayerService *MediaPlayerService::registerService(RadioPlayer *player) {
  if (registeredMediaPlayerService) {
    registeredMediaPlayerService->deleteLater();
    registeredMediaPlayerService = nullptr;
  }

  return createMediaPlayerService(player);
}

RadioPlayer *MediaPlayerService::player() {
  return m_player;
}

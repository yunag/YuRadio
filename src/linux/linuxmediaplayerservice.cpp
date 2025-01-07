#include <MprisPlayer>
#include <QObject>

#include "linuxmediaplayerservice.h"

using namespace Qt::StringLiterals;

static Mpris::PlaybackStatus
getMprisPlaybackStatus(RadioPlayer::PlaybackState state) {
  switch (state) {
    case RadioPlayer::StoppedState:
      return Mpris::Stopped;
    case RadioPlayer::PlayingState:
      return Mpris::Playing;
    case RadioPlayer::PausedState:
      return Mpris::Paused;
  }
}

LinuxMediaPlayerService::LinuxMediaPlayerService(RadioPlayer *player,
                                                 QString serviceName,
                                                 QString identity)
    : MediaPlayerService(player), m_mprisPlayer(new MprisPlayer(this)) {
  /* Will register org.mpris.MediaPlayer2.<serviceName> service */
  m_mprisPlayer->setServiceName(std::move(serviceName));
  m_mprisPlayer->setIdentity(std::move(identity));
  m_mprisPlayer->setCanPlay(true);
  m_mprisPlayer->setCanPause(true);
  m_mprisPlayer->setCanControl(true);

  connect(m_mprisPlayer, &MprisPlayer::playRequested, m_player,
          &RadioPlayer::play);
  connect(m_mprisPlayer, &MprisPlayer::stopRequested, m_player,
          &RadioPlayer::stop);
  connect(m_mprisPlayer, &MprisPlayer::pauseRequested, m_player,
          &RadioPlayer::pause);

  connect(m_mprisPlayer, &MprisPlayer::playPauseRequested, m_player, [this]() {
    if (m_player->isPlaying()) {
      m_player->pause();
    } else {
      m_player->play();
    }
  });

  connect(m_mprisPlayer, &MprisPlayer::volumeRequested, m_player,
          [this](double volume) {
    m_player->setVolume(volume);
  });

  connect(m_player, &RadioPlayer::volumeChanged, m_mprisPlayer, [this]() {
    m_mprisPlayer->setVolume(m_player->volume());
  });

  connect(m_player, &RadioPlayer::playbackStateChanged, m_mprisPlayer,
          [this]() {
    m_mprisPlayer->setPlaybackStatus(
      getMprisPlaybackStatus(m_player->playbackState()));
  });

  connect(m_player, &RadioPlayer::streamTitleChanged, this, [this]() {
    QVariantMap metadata = m_mprisPlayer->metadata();
    metadata[u"xesam:title"_s] = m_player->streamTitle();
    m_mprisPlayer->setMetadata(metadata);
  });

  connect(m_player, &RadioPlayer::mediaStatusChanged, this, [this]() {
    if (m_player->mediaStatus() == RadioPlayer::NoMedia) {
      m_mprisPlayer->setCanControl(false);
    } else if (m_player->mediaStatus() == RadioPlayer::LoadingMedia) {
      m_mprisPlayer->setCanControl(true);
    }
  });

  connect(m_player, &RadioPlayer::mediaItemChanged, this, [this]() {
    QVariantMap mprisMetadata;
    const MediaItem &mediaItem = m_player->mediaItem();

    mprisMetadata[u"mpris:length"_s] = -1;
    mprisMetadata[u"mpris:artUrl"_s] = mediaItem.artworkUri;
    mprisMetadata[u"xesam:artist"_s] = mediaItem.author;
    mprisMetadata[u"xesam:title"_s] = u"YuRadio"_s;

    m_mprisPlayer->setMetadata(std::move(mprisMetadata));
  });
}

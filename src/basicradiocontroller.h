#ifndef BASICRADIOCONTROLLER_H
#define BASICRADIOCONTROLLER_H

#include <QMediaPlayer>

#include "platformradiocontroller.h"

class QMediaDevices;
class RadioInfoReaderProxyServer;

class BasicRadioController : public PlatformRadioController {
  Q_OBJECT

public:
  explicit BasicRadioController(QObject *parent = nullptr);
  ~BasicRadioController() override;

  void play() override;
  void stop() override;
  void pause() override;

  void setVolume(float volume) override;
  void setMediaItem(const MediaItem &mediaItem) override;

private slots:
  void mediaStatusChanged(QMediaPlayer::MediaStatus status);
  void onAudioStreamRecorderChanged();

private:
  void processMediaItem(const MediaItem &mediaItem);
  void reconnectMediaPlayer();

protected:
  RadioInfoReaderProxyServer *m_proxyServer;
  QThread m_proxyServerThread;

  QString m_lastStreamTitle;
  QMediaPlayer *m_mediaPlayer;
  QMediaDevices *m_mediaDevices;
};

#endif /* !BASICRADIOCONTROLLER_H */

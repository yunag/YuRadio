#ifndef QTMEDIARADIOCONTROLLER_H
#define QTMEDIARADIOCONTROLLER_H

#include <QMediaPlayer>

#include "platformradiocontroller.h"

class QMediaDevices;
class RadioInfoReaderProxyServer;

class QtMediaRadioController : public PlatformRadioController {
  Q_OBJECT

public:
  explicit QtMediaRadioController(QObject *parent = nullptr);
  ~QtMediaRadioController() override;

  void play() override;
  void stop() override;
  void pause() override;

  void setVolume(qreal volume) override;
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

#endif /* !QTMEDIARADIOCONTROLLER_H */

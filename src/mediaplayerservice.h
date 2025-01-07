#ifndef MEDIAPLAYERSERVICE_H
#define MEDIAPLAYERSERVICE_H

#include "radioplayer.h"

class MediaPlayerService : public QObject {
public:
  ~MediaPlayerService() override = default;
  static MediaPlayerService *registerService(RadioPlayer *player);

  RadioPlayer *player();

protected:
  MediaPlayerService(RadioPlayer *player) : QObject(player), m_player(player) {}

protected:
  RadioPlayer *m_player = nullptr;
};

#endif /* !MEDIAPLAYERSERVICE_H */

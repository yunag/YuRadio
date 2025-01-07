#ifndef ANDROIDMEDIAPLAYERSERVICE_H
#define ANDROIDMEDIAPLAYERSERVICE_H

#include "mediaplayerservice.h"

class AndroidMediaSessionImageProvider;

class AndroidMediaPlayerService : public MediaPlayerService {
public:
  AndroidMediaPlayerService(RadioPlayer *player);
  ~AndroidMediaPlayerService() override;

public:
  QJniObject radioPlayer;
  AndroidMediaSessionImageProvider *imageProvider;

private:
  static void registerNativeMethods();
};

#endif /* !ANDROIDMEDIAPLAYERSERVICE_H */

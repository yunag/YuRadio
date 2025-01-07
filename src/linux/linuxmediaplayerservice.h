#ifndef LINUXMEDIAPLAYERSERVICE_H
#define LINUXMEDIAPLAYERSERVICE_H

#include "mediaplayerservice.h"
#include "radioplayer.h"

class MprisPlayer;

class LinuxMediaPlayerService : public MediaPlayerService {
public:
  LinuxMediaPlayerService(RadioPlayer *player, QString serviceName,
                          QString identity);

private:
  MprisPlayer *m_mprisPlayer;
};

#endif /* !LINUXMEDIAPLAYERSERVICE_H */

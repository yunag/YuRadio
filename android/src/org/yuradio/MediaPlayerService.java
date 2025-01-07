package org.yuradio;

import android.content.Intent;
import android.util.Log;

import androidx.annotation.Nullable;
import androidx.media3.common.Player;
import androidx.media3.common.util.UnstableApi;
import androidx.media3.session.MediaSession;
import androidx.media3.session.MediaSessionService;
import androidx.media3.common.MediaMetadata;

public class MediaPlayerService extends MediaSessionService {
    private static final String TAG = MediaPlayerService.class.getSimpleName();

    private MediaSession mediaSession = null;

    @Nullable
    @Override
    public MediaSession onGetSession(MediaSession.ControllerInfo controllerInfo) {
        Log.i(TAG, "onGetSession");
        return mediaSession;
    }

    @Override
    @UnstableApi
    public void onCreate() {
        Log.i(TAG, "onCreate");

        super.onCreate();

        RadioPlayer player = new RadioPlayer(this);
        mediaSession = new MediaSession.Builder(this, player).build();
    }

    @Override
    public void onTaskRemoved(Intent rootIntent) {
        Log.i(TAG, "onTaskRemoved");

        Player player = mediaSession.getPlayer();
        if (!player.getPlayWhenReady() || player.getMediaItemCount() == 0 ||
                player.getPlaybackState() == Player.STATE_ENDED) {
            stopSelf();
        }
    }

    @Override
    public void onDestroy() {
        Log.i(TAG, "onDestroy");

        mediaSession.getPlayer().release();
        mediaSession.release();
        mediaSession = null;

        super.onDestroy();
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        Log.i(TAG, "onStartCommand");
        return super.onStartCommand(intent, flags, startId);
    }
}

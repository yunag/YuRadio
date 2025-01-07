package org.yuradio;

import android.content.Context;
import android.net.Uri;
import android.os.Handler;
import android.util.Log;

import androidx.annotation.NonNull;
import androidx.media3.common.MediaItem;
import androidx.media3.common.MediaMetadata;
import androidx.media3.common.Player;
import androidx.media3.common.SimpleBasePlayer;
import androidx.media3.common.util.UnstableApi;

import com.google.common.util.concurrent.Futures;
import com.google.common.util.concurrent.ListenableFuture;

import java.util.ArrayList;
import java.util.List;

@UnstableApi
public class RadioPlayer extends SimpleBasePlayer {
    private static final String TAG = RadioPlayer.class.getSimpleName();

    Context context;
    private int mediaState = Player.STATE_IDLE;
    private String mediaTitle;
    private String mediaAuthor;
    private Uri mediaSource;
    private Uri mediaArtworkUri;
    private boolean playWhenReady = false;
    private MediaItem mediaItem = null;

    native public void setVolumeNative(float volume);

    native public void playNative();

    native public void pauseNative();

    native public void stopNative();

    native public void radioPlayerHandlerNative(RadioPlayer player);

    private static class NativeMediaState {
        final static int NoMedia = 0;
        final static int LoadingMedia = 1;
        final static int LoadedMedia = 2;
        final static int InvalidMedia = 3;
        final static int EndOfFile = 4;
    }

    private static class NativePlaybackState {
        final static int PlayingState = 0;
        final static int PausedState = 1;
        final static int StoppedState = 2;
    }

    /* TODO: Maybe use Looper directly */
    protected RadioPlayer(Context context) {
        super(context.getMainLooper());

        this.context = context;
        radioPlayerHandlerNative(this);
    }

    private void buildMediaItem() {
        mediaItem = new MediaItem.Builder()
                .setUri(mediaSource)
                .setMediaId(String.valueOf(mediaSource))
                .setMediaMetadata(new MediaMetadata.Builder()
                        .setArtworkUri(mediaArtworkUri)
                        .setArtist(mediaAuthor)
                        .setTitle(mediaTitle)
                        .build())
                .build();
    }

    private void tryInvalidateState() {
        try {
            invalidateState();
        } catch (Exception e) {
            Log.i(TAG, "Failed to invalidate player state: " + e.getMessage());
        }
    }
    
    public void setMediaItemData(String source, String title, String artworkUri) {
        new Handler(context.getMainLooper()).post(() -> {
            mediaTitle = title;
            mediaSource = Uri.parse(source);
            mediaArtworkUri = Uri.parse(artworkUri);

            buildMediaItem();

            tryInvalidateState();
        });
    }

    public void setAuthorMetadata(String author) {
        new Handler(context.getMainLooper()).post(() -> {
            mediaAuthor = author;

            buildMediaItem();

            tryInvalidateState();
        });
    }

    /* Called by native player when media state is changed */
    public void mediaStateChanged(int nativeMediaState) {
        new Handler(context.getMainLooper()).post(() -> {
            if (nativeMediaState == NativeMediaState.InvalidMedia || nativeMediaState == NativeMediaState.NoMedia) {
                mediaState = Player.STATE_IDLE;
            } else if (nativeMediaState == NativeMediaState.LoadingMedia) {
                mediaState = Player.STATE_BUFFERING;
            } else if (nativeMediaState == NativeMediaState.LoadedMedia) {
                mediaState = Player.STATE_READY;
            } else if (nativeMediaState == NativeMediaState.EndOfFile) {
                mediaState = Player.STATE_ENDED;
            } else {
                Log.i(TAG, "Invalid native media state: " + nativeMediaState);
                return;
            }

            tryInvalidateState();
        });
    }

    /* Called by native player when playback state is changed */
    public void playbackStateChanged(int nativePlaybackState) {
        new Handler(context.getMainLooper()).post(() -> {
            if (nativePlaybackState == NativePlaybackState.StoppedState) {
                mediaState = Player.STATE_IDLE;
                playWhenReady = false;
            } else if (nativePlaybackState == NativePlaybackState.PausedState) {
                playWhenReady = false;
            } else if (nativePlaybackState == NativePlaybackState.PlayingState) {
                playWhenReady = true;
            } else {
                Log.i(TAG, "Invalid native playback state: " + nativePlaybackState);
                return;
            }

            tryInvalidateState();
        });
    }


    @NonNull
    @Override
    protected State getState() {
        Player.Commands availableCommands = new Commands.Builder()
                .add(COMMAND_PLAY_PAUSE)
                .add(COMMAND_STOP)
                .add(COMMAND_SET_VOLUME)
                .add(COMMAND_GET_VOLUME)
                .add(COMMAND_GET_METADATA)
                .add(COMMAND_GET_CURRENT_MEDIA_ITEM)
                .build();

        List<MediaItemData> playlistData = new ArrayList<>();
        if (mediaItem != null) {
            playlistData.add(new MediaItemData.Builder(0).setMediaItem(mediaItem).build());
        }

        return new State.Builder()
                .setAvailableCommands(availableCommands) // Set which playback commands the player can handle
                .setPlayWhenReady(playWhenReady, PLAY_WHEN_READY_CHANGE_REASON_USER_REQUEST)
                .setPlaybackState(mediaState)
                .setPlaylist(playlistData)
                .setCurrentMediaItemIndex(0)
                .build();
    }

    @NonNull
    @Override
    protected ListenableFuture<?> handleStop() {
        stopNative();
        return Futures.immediateVoidFuture();
    }

    @NonNull
    @Override
    protected ListenableFuture<?> handleSetPlayWhenReady(boolean playWhenReady) {
        if (playWhenReady) {
            playNative();
        } else {
            pauseNative();
        }

        return Futures.immediateVoidFuture();
    }

    @NonNull
    @Override
    protected ListenableFuture<?> handleSetVolume(float volume) {
        setVolumeNative(volume);
        return Futures.immediateVoidFuture();
    }
}

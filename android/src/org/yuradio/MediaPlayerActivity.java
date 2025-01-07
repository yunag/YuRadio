package org.yuradio;

import android.content.ComponentName;
import android.os.Handler;
import android.util.Log;

import androidx.media3.session.MediaController;
import androidx.media3.session.SessionToken;

import com.google.common.util.concurrent.ListenableFuture;
import com.google.common.util.concurrent.MoreExecutors;

import org.qtproject.qt.android.bindings.QtActivity;

import java.util.concurrent.ExecutionException;

public class MediaPlayerActivity extends QtActivity {
    private static final String TAG = MediaPlayerActivity.class.getSimpleName();

    private MediaController controller = null;
    private ListenableFuture<MediaController> controllerFuture;

    public void registerMediaPlayerService() {
        new Handler(getMainLooper()).post(() -> {
            SessionToken sessionToken = new SessionToken(this, new ComponentName(this, MediaPlayerService.class));

            controllerFuture = new MediaController.Builder(this, sessionToken).buildAsync();
            controllerFuture.addListener(() -> {
                try {
                    controller = controllerFuture.get();
                } catch (ExecutionException | InterruptedException e) {
                    throw new RuntimeException(e);
                }
            }, MoreExecutors.directExecutor());
        });
    }

    public void unregisterMediaPlayerService() {
        new Handler(getMainLooper()).post(() -> {
            MediaController.releaseFuture(controllerFuture);
        });
    }

    @Override
    public void onDestroy() {
        Log.i(TAG, "onDestroy");

        MediaController.releaseFuture(controllerFuture);
        super.onDestroy();
    }
}

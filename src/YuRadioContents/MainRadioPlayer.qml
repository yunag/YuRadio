pragma Singleton

import QtQuick
import Main
import YuRadioContents

RadioPlayer {
    id: root

    property radiostation currentItem
    property string stationName

    function toggleRadio() {
        if (!playing) {
            play();
        } else {
            pause();
        }
    }

    stopOnPause: AppSettings.pauseButtonBehaviour === "stop"

    volume: AppSettings.volume
    onVolumeChanged: {
        AppSettings.volume = volume;
    }

    audioStreamRecorder: AudioStreamRecorder {
        stationName: root.stationName

        recordingPolicy: AppSettings.recordingPolicy
        recordingNameFormat: AppSettings.recordingNameFormat

        outputLocation: AppSettings.recordingsDirectory

        onOutputLocationChanged: {
            AppSettings.recordingsDirectory = outputLocation;
        }
    }

    onCurrentItemChanged: {
        /* TODO: Add option to enable this behavior
        if (audioStreamRecorder.recording) {
            audioStreamRecorder.stop();
            audioStreamRecorder.record();
        }
        */
        stationName = currentItem.name;
        const newMediaItem = constructMediaItem();
        if (currentItem.isValid()) {
            newMediaItem.artworkUri = currentItem.favicon;
            newMediaItem.author = currentItem.name;
            newMediaItem.source = currentItem.url;
            AppSettings.stationUuid = currentItem.uuid;
        } else {
            AppSettings.stationUuid = "";
        }
        mediaItem = newMediaItem;
    }
}

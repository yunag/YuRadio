pragma Singleton

import QtQml
import QtQuick

import Main
import YuRadioContents

QtObject {
    id: root

    property string lastStreamTitle
    property date lastStreamTitleDate
    property radiostation lastStation
    property bool streamTitleHistoryDirty: false

    property list<radiostation> bookmarkModel
    property Timer voteUpdateTimer: Timer {
        interval: 10 * 60 * 1000 // 10 minutes
        running: true
        repeat: true
        triggeredOnStart: true
        onTriggered: {
            Storage.removeExpiredVotes(interval);
        }
    }

    function init() {
        bookmarkModel = Storage.getBookmarks();
        Storage.removeExpiredTags(14 * 24 * 60 * 60 * 1000); // 2 weeks
    }

    function addBookmark(station: radiostation) {
        if (Storage.addBookmark(station)) {
            bookmarkModel.push(station);
        }
    }

    function deleteBookmark(uuid: string) {
        if (Storage.deleteBookmark(uuid)) {
            bookmarkModel = bookmarkModel.filter(bookmark => bookmark.uuid !== uuid);
        }
    }

    property Connections radioPlayerConnection: Connections {
        target: MainRadioPlayer

        function onStreamTitleChanged() {
            if (root.lastStreamTitle.length > 0 && root.lastStation.isValid()) {
                root.streamTitleHistoryDirty |= Storage.addTrackHistory(root.lastStreamTitle, root.lastStation.name, root.lastStation.favicon, root.lastStreamTitleDate);
            }
            root.lastStreamTitle = MainRadioPlayer.streamTitle;
            root.lastStation = MainRadioPlayer.currentItem;
            root.lastStreamTitleDate = new Date();
        }

        function onCurrentItemChanged() {
            root.lastStreamTitle = "";
            root.lastStreamTitleDate = new Date();
            root.lastStation = RadioStationFactory.create();
        }
    }
}

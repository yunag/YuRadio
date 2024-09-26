pragma Singleton

import QtQml
import QtQuick

import Main

QtObject {
    id: root

    property var database: null
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
            bookmarkModel.push(station)
        }
    }

    function deleteBookmark(uuid: string) {
        if (Storage.deleteBookmark(uuid)) {
            bookmarkModel = bookmarkModel.filter(bookmark => bookmark.uuid !== uuid)
        }
    }
}
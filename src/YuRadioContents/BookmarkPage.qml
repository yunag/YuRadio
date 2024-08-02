pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Effects
import QtQuick.Controls

import "radiobrowser.mjs" as RadioBrowser
import network
import YuRadioContents

Item {
    id: root

    focus: true

    required property NetworkManager networkManager
    required property RadioDrawer drawer
    property var stations: []

    component HighlightBar: Rectangle {
        width: ListView.view.currentItem?.width ?? 0
        height: ListView.view.currentItem?.height ?? 0

        color: "lightsteelblue"
        y: ListView.view.currentItem?.y ?? 0
        opacity: 0.6
        Behavior on y {
            SpringAnimation {
                spring: 4
                damping: 0.5
            }
        }
    }

    ListView {
        id: bookmarkListView

        displayMarginEnd: bottomBarDrawer.height
        currentIndex: -1
        clip: true
        focus: true

        boundsMovement: Flickable.StopAtBounds
        boundsBehavior: Flickable.DragOverBounds
        highlight: HighlightBar {}

        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            bottom: bottomBarDrawer.top
        }

        highlightFollowsCurrentItem: false
        model: Storage.bookmarkModel
        delegate: RadioStationDelegate {
            id: delegate

            focus: true
            focusPolicy: Qt.StrongFocus
            networkManager: root.networkManager
            onCurrentStationChanged: {
                if (currentStation) {
                    Qt.callLater(() => {
                      bookmarkListView.currentIndex = Qt.binding(() => delegate.currentStation ? index : -1);
                    });
                }
            }

            onClicked: {
                if (ListView.view.currentIndex == delegate.index) {
                    MainRadioPlayer.toggle();
                } else {
                    RadioBrowser.click(root.networkManager.baseUrl, stationuuid);
                    bookmarkListView.currentIndex = delegate.index;
                    MainRadioPlayer.currentItem = Object.assign({}, bookmarkListView.model.get(delegate.index));
                    Qt.callLater(MainRadioPlayer.play);
                }
            }
        }
    }

    RadioBottomBar {
        id: bottomBarDrawer
        listView: bookmarkListView
    }

    Connections {
        target: root.drawer
        function onOpened() {
            bottomBarDrawer.close();
        }
    }
}

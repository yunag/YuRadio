pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

import YuRadioContents

Loader {
    id: root

    required property int index
    required property int bitrate

    required property list<string> tags

    required property string name
    required property string country
    required property string countryCode
    required property string uuid

    required property url favicon
    required property url url

    readonly property bool currentStation: MainRadioPlayer.currentItem.isValid() && MainRadioPlayer.currentItem.uuid == uuid
    readonly property FilledGridView view: GridView.view as FilledGridView

    signal clicked
    signal moreOptionsMenuRequested(Item context)

    height: view.cellHeight
    width: view.cellWidth
    focus: true

    Component.onCompleted: {
        if (view.numItemsInRow >= 3) {
            sourceComponent = sceletonComponent;
            Utils.execLater(root, Utils.getRandomInt(200, 300), () => sourceComponent = mainComponent);
        } else {
            sourceComponent = mainComponent;
        }
    }

    Component {
        id: sceletonComponent

        Item {
            Rectangle {
                anchors.fill: parent
                anchors.margins: 5

                opacity: 0.3
                radius: 8

                gradient: Gradient {
                    GradientStop {
                        position: 0
                        color: AppConfig.isDarkTheme ? "#29323c" : "#e6e9f0"
                    }

                    GradientStop {
                        position: 1
                        color: AppConfig.isDarkTheme ? "#474445" : "#eef1f5"
                    }
                    orientation: Gradient.Vertical
                }
            }
        }
    }

    Component {
        id: mainComponent

        ItemDelegate {
            id: delegate

            onClicked: root.clicked()

            Accessible.name: qsTr("Radio station name: %1, Country: %2, Tags: %3").arg(root.name).arg(root.country).arg(root.tags.join(", "))

            focusPolicy: Qt.StrongFocus
            focus: true

            Binding {
                when: root.currentStation
                target: delegate.background
                property: "color"
                value: Qt.color("lightsteelblue").darker(AppConfig.isDarkTheme ? 1.8 : 1.05)
            }

            leftPadding: 5
            topPadding: 5
            bottomPadding: 5
            rightPadding: 0

            contentItem: RowLayout {
                RadioImage {
                    id: radioImage

                    Layout.fillHeight: true
                    Layout.preferredWidth: height
                    targetSource: root.favicon
                    sourceSize: Qt.size(height * Screen.devicePixelRatio, height * Screen.devicePixelRatio)

                    fillMode: Image.PreserveAspectFit
                    smooth: !root.view.moving

                    Image {
                        anchors {
                            left: parent.left
                            top: parent.top
                            leftMargin: 5
                            topMargin: 5
                        }

                        opacity: 0.8
                        cache: true
                        source: root.countryCode ? `https://flagsapi.com/${root.countryCode}/flat/24.png` : ''
                        sourceSize: Qt.size(24, 24)
                    }
                }

                ColumnLayout {
                    Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    Layout.leftMargin: 5
                    Layout.rightMargin: 5

                    Timer {
                        id: updateElideTimer
                        interval: 100
                        repeat: false

                        onRunningChanged: {
                            stationName.clip = running;
                            stationTags.clip = running;
                            if (running) {
                                stationName.elide = Text.ElideNone;
                                stationTags.elide = Text.ElideNone;
                            }
                        }

                        onTriggered: {
                            stationName.elide = Text.ElideRight;
                            stationTags.elide = Text.ElideRight;
                        }
                    }

                    ScalableLabel {
                        id: stationName

                        Layout.fillWidth: true

                        text: root.name ? root.name : "Unknown Station"
                        textFormat: Text.PlainText

                        font.bold: true
                        fontPointSize: 15

                        onWidthChanged: updateElideTimer.restart()
                    }

                    ScalableLabel {
                        id: stationTags

                        Layout.fillWidth: true

                        text: root.tags.join(", ")
                        textFormat: Text.PlainText

                        fontPointSize: 14
                    }
                }

                IconButton {
                    id: moreOptions

                    Layout.alignment: Qt.AlignVCenter
                    Layout.fillHeight: true
                    implicitWidth: 64
                    Accessible.name: qsTr("More options menu")

                    icon.source: "images/more-vert.svg"
                    icon.width: 32
                    icon.height: 32
                    icon.color: "transparent"
                    opacity: 0.7

                    onClicked: {
                        root.moreOptionsMenuRequested(moreOptions);
                    }
                }
            }

            ScalableLabel {
                anchors {
                    bottom: parent.bottom
                    right: parent.right
                    rightMargin: 10
                    bottomMargin: 5
                }

                text: qsTr("%1 kbps").arg(root.bitrate ? root.bitrate : "-")
                textFormat: Text.PlainText

                fontPointSize: 8
                opacity: 0.8
            }
        }
    }
}

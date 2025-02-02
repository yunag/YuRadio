pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

StackViewPage {
    id: root

    focus: true

    states: [
        State {
            name: "portraitLayout"
            when: root.width > 600
            PropertyChanges {
                columnLayout.width: contents.width * 3 / 5
            }
        }
    ]

    ScrollView {
        id: scrollView

        anchors.fill: parent

        contentWidth: -1
        contentHeight: contents.implicitHeight

        Item {
            id: contents

            width: parent.width
            implicitHeight: columnLayout.implicitHeight

            ColumnLayout {
                id: columnLayout

                anchors.horizontalCenter: parent.horizontalCenter
                width: parent.width - 30

                Image {
                    Layout.fillWidth: true
                    Layout.topMargin: 10
                    Layout.bottomMargin: 10

                    Layout.preferredWidth: 150
                    Layout.preferredHeight: 150

                    fillMode: Image.PreserveAspectFit
                    source: "images/shortwave.svg"
                    smooth: true
                }

                TextArea {
                    Layout.fillWidth: true
                    Layout.topMargin: 5

                    leftPadding: 0
                    rightPadding: 0

                    readOnly: true
                    textFormat: Text.MarkdownText
                    text: qsTr(`
## About
---
**YuRadio** is a cross-platform application built with Qt.
Application uses RadioBrowser API (see <a href="https://api.radio-browser.info/"><font color="${Material.color(Material.Blue)}">radiobrowser</font></a>)
Since this is a public API, some radio stations may have inappropriate content, so be aware!
<br/>
`)

                    wrapMode: Text.Wrap
                    background: Item {}

                    ScalableFontPicker {}
                }

                ScalableButton {
                    Layout.fillWidth: true
                    Layout.topMargin: 20
                    Material.background: Material.color(Material.Grey, Material.ShadeA200)
                    Material.foreground: Material.color(Material.Grey, Material.Shade100)

                    text: "Github"

                    onClicked: {
                        Qt.openUrlExternally("https://github.com/yunag/YuRadio");
                    }

                    icon.source: "images/github-mark.svg"
                }

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }
            }
        }
    }
}

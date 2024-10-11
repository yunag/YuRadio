pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

import YuRadioContents
import network
import Main

Item {
    id: root

    required property AudioStreamRecorder audioRecorder
    required property NetworkManager networkManager
    required property LanguageTranslator languageTranslator
    required property MusicInfoModel musicInfoModel

    property string translatedFontScaleString: qsTr("Font scale")

    property Component headerContent: RowLayout {
        spacing: 0

        Item {
            Layout.fillWidth: true
        }

        ToolButton {
            id: sleepTimerButton

            icon.source: AppSettings.enableSleepTimer ? "images/hourglass-on.svg" : "images/hourglass-off.svg"
            Accessible.name: qsTr("Sleep timer configuration")

            onClicked: {
                sleepTimerPopup.open();
            }
        }
    }

    focus: true

    SleepTimerPopup {
        id: sleepTimerPopup
    }

    TabBar {
        id: tabBar

        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }

        ScalableTabButton {
            text: qsTr("General")
        }
        ScalableTabButton {
            text: qsTr("Font")
        }
        ScalableTabButton {
            text: qsTr("Recordings")
        }
    }

    component ScalableTabButton: TabButton {
        ScalableFontPicker {}
    }

    StackLayout {
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
            top: tabBar.bottom
        }

        currentIndex: tabBar.currentIndex

        SettingsGeneralPage {
            musicInfoModel: root.musicInfoModel
            networkManager: root.networkManager
            languageTranslator: root.languageTranslator
        }

        SettingsFontPage {}

        SettingsRecordingPage {
            audioRecorder: root.audioRecorder
        }
    }
}

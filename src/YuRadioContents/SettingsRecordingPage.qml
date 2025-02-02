import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import YuRadioContents
import Main

ScrollView {
    id: root

    required property AudioStreamRecorder audioRecorder

    property Flickable flickable: contentItem as Flickable

    contentWidth: -1
    contentHeight: recordingsPageContents.implicitHeight

    Binding {
        root.flickable.boundsBehavior: Flickable.StopAtBounds
    }

    Item {
        id: recordingsPageContents

        implicitHeight: columnLayout.implicitHeight
        implicitWidth: Math.min(parent.width, Math.max(columnLayout.implicitWidth, 500))

        ColumnLayout {
            id: columnLayout

            anchors {
                fill: parent
                leftMargin: 10
                rightMargin: 10
                topMargin: 20
            }

            ScalableLabel {
                id: outputLocationLabel

                Layout.fillWidth: true

                text: qsTr("Output directory")
                wrapMode: Text.Wrap
            }

            DirectoryPicker {
                recorder: root.audioRecorder
                Layout.fillWidth: true
            }

            ScalableLabel {
                id: recordingNameLabel

                Layout.topMargin: 10
                Layout.fillWidth: true

                text: qsTr("Name format")
                wrapMode: Text.Wrap
            }

            ScalableComboBox {
                id: recordingNameCombo

                implicitContentWidthPolicy: ComboBox.WidestText
                Layout.fillWidth: true
                textRole: "text"

                model: [
                    {
                        text: "datetime",
                        field: AudioStreamRecorder.DateTime,
                        visible: true
                    },
                    {
                        text: "station_datetime",
                        field: AudioStreamRecorder.StationDateTime,
                        visible: AppSettings.recordingPolicy !== AudioStreamRecorder.SaveRecordingWhenStreamTitleChanges
                    },
                    {
                        text: "station_trackname_datetime",
                        field: AudioStreamRecorder.StationTrackNameDateTime,
                        visible: AppSettings.recordingPolicy === AudioStreamRecorder.SaveRecordingWhenStreamTitleChanges
                    },
                    {
                        text: "trackname_datetime",
                        field: AudioStreamRecorder.TrackNameDateTime,
                        visible: AppSettings.recordingPolicy === AudioStreamRecorder.SaveRecordingWhenStreamTitleChanges
                    },
                ].filter(x => x.visible)

                Component.onCompleted: {
                    currentIndex = model.findIndex(x => x.field === AppSettings.recordingNameFormat);
                }

                onActivated: {
                    AppSettings.recordingNameFormat = currentValue.field;
                }
            }

            ScalableLabel {
                id: recordingPolicyLabel

                Layout.topMargin: 5
                Layout.fillWidth: true

                text: qsTr("Recording policy")
                wrapMode: Text.Wrap
            }

            ScalableComboBox {
                id: recordingPolicyCombo

                implicitContentWidthPolicy: ComboBox.WidestText
                Layout.fillWidth: true

                textRole: "text"
                model: [
                    {
                        text: qsTr("No recording policy"),
                        field: AudioStreamRecorder.NoRecordingPolicy
                    },
                    {
                        text: qsTr("Save recording when stream title changes"),
                        field: AudioStreamRecorder.SaveRecordingWhenStreamTitleChanges
                    }
                ]

                Component.onCompleted: {
                    currentIndex = model.findIndex(x => x.field === AppSettings.recordingPolicy);
                }

                onActivated: {
                    AppSettings.recordingPolicy = currentValue.field;
                }
            }

            ScalableCheckBox {
                Layout.topMargin: 5
                Layout.fillWidth: true

                text: qsTr("Save recordings when switching between stations")
                checked: AppSettings.saveRecordingsWhenSwitchingBetweenStations
                onCheckedChanged: {
                    AppSettings.saveRecordingsWhenSwitchingBetweenStations = checked;
                }
            }

            ScalableButton {
                Layout.fillWidth: true

                text: root.audioRecorder.recording ? qsTr("Stop recording") : qsTr("Start recording")
                onClicked: {
                    if (root.audioRecorder.recording) {
                        root.audioRecorder.stop();
                    } else {
                        root.audioRecorder.record();
                    }
                }
            }

            Item {
                Layout.preferredHeight: 20
                Layout.fillWidth: true
                Layout.fillHeight: true
            }
        }
    }
}

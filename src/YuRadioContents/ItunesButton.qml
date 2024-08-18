import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material

Button {
    id: root
    property string link

    Material.background: Material.color(Material.Grey, Material.Shade200)
    Material.foreground: Material.color(Material.Grey, Material.ShadeA200)

    icon.source: "images/itunes-logo.svg"
    icon.color: "transparent"

    onClicked: {
        Qt.openUrlExternally(root.link);
    }
}
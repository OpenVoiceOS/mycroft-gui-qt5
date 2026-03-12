import QtQuick 2.12
import QtQuick.Layouts 1.4
import QtQuick.Controls 2.12

Item {
    id: root

    property string text:  sessionData.text  || ""
    property string title: sessionData.title || ""

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 8

        Label {
            visible: title.length > 0
            text: title
            font.pixelSize: 22
            font.weight: Font.DemiBold
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            Label {
                width: parent.width
                text: root.text
                font.pixelSize: 16
                wrapMode: Text.WordWrap
            }
        }
    }
}

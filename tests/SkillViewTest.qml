import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import QtQuick.Window 2.15
import Mycroft 1.0 as Mycroft

Window {
    id: root
    visible: true

    minimumHeight : deviceHeight || undefined
    maximumHeight : deviceHeight || undefined
    minimumWidth : deviceWidth || undefined
    maximumWidth : deviceWidth || undefined
    x: deviceWidth ? Screen.desktopAvailableHeight - width : undefined
    y: deviceHeight ? Screen.desktopAvailableHeight - height : undefined
    color: "black"

    Mycroft.SkillView {
        id: mainView
        activeSkills.whiteList: singleSkill.length > 0 ? singleSkill : null
        anchors.fill: parent
    }
}

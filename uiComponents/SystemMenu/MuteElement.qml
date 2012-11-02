import Qt 4.7

MenuListEntry {
    id: muteElement
    property int    ident:         0
    property alias  muteText:      muteToggle.text
    property bool   mute:          false
    property bool   delayUpdate:   false
    property string newText:       ""
    property bool   newMuteStatus: false
    property real uiScale;
    property real textScale;
    property real layoutScale;

    property int iconSpacing : 4
    property int rightMarging: 8

    content:
        Item {
        width: muteElement.width

            Text {
            id: muteToggle
                x: ident;
                anchors.verticalCenter: parent.verticalCenter
                text: runtime.getLocalizedString("Mute Sound")
                color: "#FFF";
                font.bold: false;
                font.pixelSize: 18 * textScale;
                font.family: "Prelude"
            }

            Image {
                id: muteIndicatorOn
                visible: !mute
                x: parent.width - width - iconSpacing - rightMarging
                anchors.verticalCenter: parent.verticalCenter
                scale: uiScale
                smooth: true

                source: "/usr/palm/sysmgr/images/statusBar/icon-mute.png"
             }

            Image {
                id: muteIndicatorOff
                visible: mute
                x: parent.width - width - iconSpacing - rightMarging
                anchors.verticalCenter: parent.verticalCenter
                scale: uiScale
                smooth: true

                source: "/usr/palm/sysmgr/images/statusBar/icon-mute-off.png"
             }
        }
}

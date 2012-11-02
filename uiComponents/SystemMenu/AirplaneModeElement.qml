import Qt 4.7

MenuListEntry {
    id: airplaneModeElement
    property int ident: 0
    property alias modeText:     airplaneMode.text
    property bool  airplaneOn:   false
    property real uiScale;
    property real layoutScale;

    property int iconSpacing : 4
    property int rightMarging: 8

    content:
        Item {
        width: airplaneModeElement.width

            Text {
            id: airplaneMode
                x: ident;
                anchors.verticalCenter: parent.verticalCenter
                text: runtime.getLocalizedString("Turn on Airplane Mode");
                color: selectable ? "#FFF" : "#AAA";
                font.bold: false;
                font.pixelSize: 18 * textScale;
                font.family: "Prelude"
            }

            Image {
                id: airplaneIndicatorOn
                visible: !airplaneOn
                x: parent.width - width - iconSpacing - rightMarging
                anchors.verticalCenter: parent.verticalCenter
                opacity: selectable ? 1.0 : 0.65;
                scale: uiScale
                smooth: true

                source: "/usr/palm/sysmgr/images/statusBar/icon-airplane.png"
             }

            Image {
                id: airplaneIndicatorOff
                visible: airplaneOn
                x: parent.width - width - iconSpacing - rightMarging
                anchors.verticalCenter: parent.verticalCenter
                opacity: selectable ? 1.0 : 0.65;
                scale: uiScale
                smooth: true

                source: "/usr/palm/sysmgr/images/statusBar/icon-airplane-off.png"
             }
        }
}

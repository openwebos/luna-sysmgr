import Qt 4.7

Item {
    property string name
    property bool   connected:      false
    property int    cod:            0
    property string address:        ""
    property string connStatus:     ""
    property string status:         ""
    
    property real uiScale: 1.0
    property real textScale: 1.0
    property real layoutScale: 1.0

    property int iconSpacing : 4 * layoutScale
    property int rightMarging: 8 * layoutScale

    Item {
        anchors.fill: parent
        Text {
            id: mainText
            anchors.verticalCenter: parent.verticalCenter
            text: name;
            horizontalAlignment: Text.AlignLeft
            width: parent.width - check.width - rightMarging - iconSpacing - 5
            elide: Text.ElideRight;
            color: "#FFF";
            font.bold: false;
            font.pixelSize: 16 * textScale
            font.family: "Prelude"
        }

        Text {
            id: statusText
            visible: status != ""
            y: mainText.y + mainText.baselineOffset + 1
            text: status;
            color: "#AAA";
            font.pixelSize: 10 * textScale
            font.family: "Prelude"
            font.capitalization: Font.AllUppercase
        }
    }

    Image {
        id: check
        x: parent.width - width - iconSpacing - rightMarging
        anchors.verticalCenter: parent.verticalCenter
        visible: connected
        scale: uiScale
        source: "/usr/palm/sysmgr/images/statusBar/system-menu-popup-item-checkmark.png"
    }
}

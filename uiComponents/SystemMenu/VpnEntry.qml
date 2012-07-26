import Qt 4.7

Item {
    property string name
    property bool   connected:      false
    property string connStatus:     ""
    property string status:         ((connStatus == "connecting") ? runtime.getLocalizedString("Connecting...") : ((connStatus == "connectfailed") ? runtime.getLocalizedString("Unable to connect") : ""))
    property string vpnProfileInfo: ""

    property int iconSpacing : 4
    property int rightMarging: 8

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
            font.pixelSize: 16
            font.family: "Prelude"
        }

        Text {
            id: statusText
            visible: status != ""
            y: mainText.y + mainText.baselineOffset + 1
            text: status;
            color: "#AAA";
            font.pixelSize: 10
            font.family: "Prelude"
            font.capitalization: Font.AllUppercase
        }
    }

    Image {
        id: check
        x: parent.width - width - iconSpacing - rightMarging
        anchors.verticalCenter: parent.verticalCenter
        visible: connected
        source: "/usr/palm/sysmgr/images/statusBar/system-menu-popup-item-checkmark.png"
    }
}


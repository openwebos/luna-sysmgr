import Qt 4.7

Item {
    property string name
    property int    profileId:      0
    property int    signalBars:     0
    property string securityType:   ""
    property string connStatus:     ""
    property string status:         ""
    property bool   statusInBold:   false
    property bool   connected:      false

    property int iconSpacing : 4
    property int rightMarging: 3

    Item {
        anchors.fill: parent
        Text {
            id: mainText
            anchors.verticalCenter: parent.verticalCenter
            text: name; color: "#FFF";
            horizontalAlignment: Text.AlignLeft
            width: parent.width - sigStrength.width - check.width - lock.width - rightMarging - 3*iconSpacing - 5
            elide: Text.ElideRight;
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
            font.bold: statusInBold;
            font.pixelSize: 10
            font.family: "Prelude"
            font.capitalization: Font.AllUppercase
        }
    }

    Image {
        id: sigStrength
        x: parent.width - width - iconSpacing - rightMarging
        anchors.verticalCenter: parent.verticalCenter

        source: "/usr/palm/sysmgr/images/statusBar/wifi-" + signalBars + ".png"
    }

    Image {
        id: lock
        x: sigStrength.x - width - iconSpacing
        anchors.verticalCenter: parent.verticalCenter
        visible: securityType != ""
        source: "/usr/palm/sysmgr/images/statusBar/system-menu-lock.png"
    }

    Image {
        id: check
        x: lock.x - width - iconSpacing
        anchors.verticalCenter: parent.verticalCenter
        visible: connected
        source: "/usr/palm/sysmgr/images/statusBar/system-menu-popup-item-checkmark.png"
    }
}

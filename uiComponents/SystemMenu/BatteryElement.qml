import Qt 4.7

MenuListEntry {
    id: thisElement
    property int ident: 0
    property string batteryText;

    Connections {
        target: NativeSystemMenuHandler
        onBatteryLevelUpdated: batteryText = batteryLevel
    }

    selectable: false
    content:
        Row {
            x: ident;
            width: parent.width
            Text {
                text: runtime.getLocalizedString("Battery: ");
                color: "#AAA";
                font.pixelSize: 18
                font.family: "Prelude"
            }

            Text {
                text: batteryText;
                color: "#AAA";
                font.pixelSize: 18;
                font.family: "Prelude"
            }
        }
}

import Qt 4.7
import "../ActionButton"


Item {
    property int  margin: 6
    property int  topOffset: 4

    property string dialogTitle: runtime.getLocalizedString("Dismissing Cards");
    property string dialogMessage: runtime.getLocalizedString("You can close an application by using your finger to flick it up and off screen while in Card View.");

    property alias actionButton1: okButton;

    signal okButtonPressed();

    width: 320;
    height: 170;

    id: dialog;

    Text {
        id: titleText;
        width: dialog.width - 2 * margin;
        font.family: "Prelude"
        font.pixelSize: 18
        font.bold: true;
        wrapMode: Text.Wrap;
        color: "#FFF";
        anchors.horizontalCenter: parent.horizontalCenter
        horizontalAlignment: Text.AlignLeft;
        y: margin + topOffset;

        text: dialogTitle;
    }

    Text {
        id: msgText;
        width: dialog.width - 2 * margin;
        font.family: "Prelude"
        font.pixelSize: 14
        font.bold: true;
        wrapMode: Text.Wrap;
        color: "#FFF";
        anchors.horizontalCenter: parent.horizontalCenter
        horizontalAlignment: Text.AlignLeft;
        y: titleText.y + titleText.height + margin;

        text: dialogMessage;
    }

 	ActionButton {
        id: okButton;
        caption: runtime.getLocalizedString("OK");
        width: dialog.width - 2 * margin - 1;
        height: visible ? 52 : 0;
        x: margin + 1;
        onAction: dialog.okButtonPressed();
		anchors.bottom: dialog.bottom;
    }
}
//*/

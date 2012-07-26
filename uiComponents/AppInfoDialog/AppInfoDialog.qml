import Qt 4.7
import "../ActionButton"

Item {
    property int  edgeOffset: 11
    property int  margin: 6
    property int  topOffset: 4

    property string dialogTitle: "None"
    property string dialogMessage: "None"
    property string appIdContext: ""
    property string iconUidStringContext: ""
    property int    numberOfButtons: 2 // valid between 0 and 3

    property alias actionButton1: removeButton;
    property alias actionButton2: cancelButton;

    signal removeButtonPressed(string appid,string iconuid);
    signal cancelButtonPressed();
	signal dialogDisappearedCompletely();
	signal dialogAppearedCompletely();
	
    width: 320 + 2 * edgeOffset
    height: titleText.height + msgText.height + ((numberOfButtons > 0) ? (removeButton.height + cancelButton.height) : edgeOffset) + 2*edgeOffset + 4*margin + topOffset;

    id: dialog;

    function setupDialog(title, message, numButtons) {
         dialogTitle     = title;
         dialogMessage   = message;
         numberOfButtons = numButtons;
     }

    function setRemoveButton(message, type) {
        setupButton(removeButton, message, type);
    }

    function setCancelButton(message, type) {
        setupButton(cancelButton, message, type);
    }

    function setupButton (button, message, type) {
        button.caption = message;

        if(type == "affirmative") {
            button.affirmative = true;
            button.negative    = false;
        } else if (type == "negative") {
            button.affirmative = false;
            button.negative    = true;
        } else {
            button.affirmative = false;
            button.negative    = false;
        }
    }

    function fade(fadeIn, fadeDuration) {
        fadeAnim.duration = fadeDuration;

        if(fadeIn) {
            opacity = 1.0;
        } else {
            opacity = 0.0;
        }
    }

    Behavior on opacity {
        NumberAnimation{ id: fadeAnim; duration: 300; }
    }

    onOpacityChanged: {
        if(opacity == 0.0) {
            visible = false;
            dialog.dialogDisappearedCompletely();
         } else {
            visible = true;
            dialog.dialogAppearedCompletely();
        }
    }

    BorderImage {
        source: "/usr/palm/sysmgr/images/popup-bg.png"
        width: parent.width;
        height: parent.height;
        border { left: 35; top: 40; right: 35; bottom: 40 }
    }

    Text {
        id: titleText;
        width: dialog.width - 2 * (edgeOffset + margin);
        font.family: "Prelude"
        font.pixelSize: 18
        font.bold: true;
        wrapMode: Text.Wrap;
        color: "#FFF";
        anchors.horizontalCenter: parent.horizontalCenter
        horizontalAlignment: Text.AlignLeft;
        y: edgeOffset + margin + topOffset;

        text: dialogTitle;
    }

    Text {
        id: msgText;
        width: dialog.width - 2 * (edgeOffset + margin);
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
        id: cancelButton;
        caption: runtime.getLocalizedString("Cancel");
        width: dialog.width - 2 * (edgeOffset + margin) - 1;
        height: visible ? 52 : 0;
        x: edgeOffset + margin + 1;
        y: msgText.y + msgText.height + margin;
        visible: numberOfButtons > 0;
        onAction: dialog.cancelButtonPressed();
    }
    
    ActionButton {
        id: removeButton;
        caption: runtime.getLocalizedString("Remove");
        width: dialog.width - 2 * (edgeOffset + margin) - 1;
        height: visible ? 52 : 0;
        x: edgeOffset + margin + 1;
        y: cancelButton.y + removeButton.height;
        visible: numberOfButtons > 1;
        onAction: dialog.removeButtonPressed(dialog.appIdContext,dialog.iconUidStringContext);
    }
}

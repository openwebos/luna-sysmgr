import Qt 4.7
import "../ActionButton"

Item {
    property int  edgeOffset: 11
    property int  margin: 6
    property int  topOffset: 4

    property string dialogTitle: "Title"
    property string dialogMessage: "Message Body."
    property int    numberOfButtons: 3 // valid between 0 and 3

    property alias actionButton1: button1;
    property alias actionButton2: button2;
    property alias actionButton3: button3;

    signal button1Pressed();
    signal button2Pressed();
    signal button3Pressed();

    width: 320 + 2 * edgeOffset
    height: titleText.height + msgText.height + ((numberOfButtons > 0) ? (button1.height + button2.height + button3.height) : edgeOffset) + 2*edgeOffset + 4*margin + topOffset;

    id: dialog;

    function setupDialog(title, message, numButtons) {
         dialogTitle     = title;
         dialogMessage   = message;
         numberOfButtons = numButtons;
     }

    function setButton1(message, type) {
        setupButton(button1, message, type);
    }

    function setButton2(message, type) {
        setupButton(button2, message, type);
    }

    function setButton3(message, type) {
        setupButton(button3, message, type);
    }

    function setupButton (button, message, type) {
        button.caption = message;
        button.affirmative = (type == "affirmative");
        button.negative = (type == "negative");
        button.visible = (type != "disabled");
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
         } else {
            visible = true;
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
        id: button1;
        caption: "Button 1";
        width: dialog.width - 2 * (edgeOffset + margin) - 1;
        height: visible ? 52 : 0;
        x: edgeOffset + margin + 1;
        y: msgText.y + msgText.height + margin;
        visible: numberOfButtons > 0;
        onAction: button1Pressed();
    }

    ActionButton {
        id: button2;
        caption: "Button 2";
        width: dialog.width - 2 * (edgeOffset + margin) - 1;
        height: visible ? 52 : 0;
        x: edgeOffset + margin + 1;
        y: button1.y + button1.height
        visible: numberOfButtons > 1;
        onAction: button2Pressed();
    }

    ActionButton {
        id: button3;
        caption: "Button 3";
        width: dialog.width - 2 * (edgeOffset + margin) - 1;
        height: visible ? 52 : 0;
        x: edgeOffset + margin + 1;
        y: button2.y + button2.height
        visible: numberOfButtons > 2;
        onAction: button3Pressed();
    }

}

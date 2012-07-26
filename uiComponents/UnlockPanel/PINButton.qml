import Qt 4.7

Item {
    property bool isPressed: false
    property string caption: ""
    property string imgSource: ""
    property bool   active: true

    BorderImage {
        id: pressedBkg
        source: "/usr/palm/sysmgr/images/pin/pin-key-highlight.png"
        visible: isPressed;
        width: parent.width;
        height: parent.height;
        border { left: 10; top: 10; right: 10; bottom: 10 }
    }

    Text {
        id: buttonText
        text: caption
        visible: caption != ""  && !buttonImg.visible;
        anchors.centerIn: parent
        color: "#FFF";
        font.bold: true;
        font.pixelSize: 30
        font.family: "Prelude"
        font.capitalization: Font.AllUppercase
    }

    Image {
        id: buttonImg
        source: imgSource
        visible: imgSource != "";
        anchors.centerIn: parent
    }

    MouseArea {
        id: mouseArea
        enabled: true;
        anchors.fill: parent
        onPressAndHold:  setPressed(true);
        onPressed: { mouse.accepted = true; setPressed(true); }
        onReleased: {setPressed(false);}
        onExited: {setPressed(false);}
        onCanceled: {setPressed(false);}
        onClicked: {
            actionPerformed()
         }
    }

    function setPressed (pressed) {
        if(active) {
            isPressed = pressed;
        }
    }

    function actionPerformed () {
        if(active) {
            action(caption)
        }
    }

    signal action(string text)
}

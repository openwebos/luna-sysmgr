import Qt 4.7
import "../ActionButton"
import CustomComponents 1.0


InputItem {
    property int  edgeOffset: 11
    property int  margin: 6
    property int  topOffset: 4
    property bool isPINEntry: true
    property int  minPassLength: 4
    property bool enforceMinLength: false
    property string queuedTitle: ""
    property string queuedHint: ""

    signal entryCanceled();

    signal requestFocusChange(bool focusRequest);

    signal passwordSubmitted(string password, bool isPIN);

    function setupDialog(isPIN, title, hintMessage, enforceLength, minLen) {
        isPINEntry = isPIN;
        titleText.text = title;
        enforceMinLength = enforceLength;
        minPassLength = minLen;
        passwordField.clearAll();
        passwordField.setHintText(hintMessage);
     }

    function queueUpTitle(newTitle, newHint) {
        queuedTitle = newTitle;
        queuedHint = newHint;
    }

    function fade(fadeIn, fadeDuration) {
        fadeAnim.duration = fadeDuration;

        if(fadeIn) {
            opacity = 1.0;
        } else {
            opacity = 0.0;
        }
    }

    width: 320 + 2 * edgeOffset
    height: buttonGrid.y + buttonGrid.height + edgeOffset + margin;
    focus: true;

    BorderImage {
        source: "/usr/palm/sysmgr/images/popup-bg.png"
        width: parent.width;
        height: parent.height;
        border { left: 35; top: 40; right: 35; bottom: 40 }
    }

    Text {
        id: titleText;
        font.family: "Prelude"
        font.pixelSize: 18
        font.bold: true;
        color: "#FFF";
        anchors.horizontalCenter: parent.horizontalCenter
        y: edgeOffset + margin + topOffset;

        text: "Device Locked";
    }

    PasswordField {
        id: passwordField;
        isPIN: isPINEntry;
        width: 320 - 4;
        x: edgeOffset + 3
        y: titleText.y + titleText.height + (isPINEntry ? 0 : 6)

        onTextFieldClicked: {
            if(!isPINEntry) {
                requestFocusChange(true);
            } else {
                focus = true;
            }
        }
    }

    PINPad {
        id: keyPad
        visible: isPINEntry
        x: edgeOffset
        anchors.top: passwordField.bottom

        onKeyAction: {
            if(keyText == "\b") {
                // backspace key pressed
                passwordField.deleteOne();
            } else {
                passwordField.keyInput(keyText, true);
                if(queuedTitle != "") {
                    titleText.text = queuedTitle;
                    queuedTitle = "";
                }
                if(queuedHint != "") {
                    passwordField.setHintText(queuedHint);
                    queuedHint = "";
                }
            }
        }
    }

    Grid {
        id: buttonGrid
        width: 320 - 2 * margin
        x: edgeOffset + margin
        anchors.top: isPINEntry ? keyPad.bottom : passwordField.bottom;

        columns: 2
        rows: 1
        spacing: margin + 1

        ActionButton {
            caption: runtime.getLocalizedString("Cancel");
            width: buttonGrid.width/buttonGrid.columns - margin / 2
            height:52
            onAction: entryCanceled();
        }

        ActionButton {
            caption: runtime.getLocalizedString("Done");
            affirmative: true
            width: buttonGrid.width/buttonGrid.columns - margin / 2
            height:52
            active: passwordField.enteredText.length >= (enforceMinLength ? minPassLength : 1);
            onAction: {
                if(passwordField.enteredText.length > 0) {
                    passwordSubmitted(passwordField.enteredText, isPINEntry);
                }
            }
        }
    }

    function isValidKey(keyCode) {
        if(((keyCode >= Qt.Key_Escape) && (keyCode <= Qt.Key_Direction_R)) ||
           ((keyCode >= Qt.Key_Back) && (keyCode <= Qt.Key_unknown))     ) {
            // filter keys here
            return false;
        }

        return true;
    }

    function isNumber(keyCode) {
        if((keyCode >= Qt.Key_0) && (keyCode <= Qt.Key_9)) {
            return true;
        }

        return false;
    }

    Behavior on opacity {
        NumberAnimation{ id: fadeAnim;
                         duration: 300;
                         onStarted: {
                               if(opacity == 0.0) {
                               }  else {
                               }
                            }
                        }
    }

    onOpacityChanged: {
        if(opacity == 0.0) {
            if(!isPINEntry) {
                // faded away, clear focus
                requestFocusChange(false);
            } else {
                focus = false;
            }
            visible = false;
        } else if(opacity == 1.0) {
            if(!isPINEntry) {
                // faded in, request focus
                requestFocusChange(true);
            } else {
                focus = true;
            }
            visible = true;
        } else {
            visible = true;
        }
    }

    Keys.onPressed: {
        event.accepted = true;

        if(isValidKey(event.key)) {
             passwordField.keyInput(event.text, isNumber(event.key));
             if(queuedTitle != "") {
                 titleText.text = queuedTitle;
                 queuedTitle = "";
             }
             if(queuedHint != "") {
                 passwordField.setHintText(queuedHint);
                 queuedHint = "";
             }
         } else if(event.key == Qt.Key_Backspace) {
             passwordField.deleteOne();
         }
    }

    Keys.onReleased: {
        event.accepted = true;
    }

    Keys.onDeletePressed: {
        passwordField.deleteOne();
        event.accepted = true;
    }

    Keys.onEnterPressed: {
        event.accepted = true;
        if(passwordField.enteredText.length > 0) {
            passwordSubmitted(passwordField.enteredText, isPINEntry);
        }
    }

    Keys.onReturnPressed: {
        event.accepted = true;
        if(passwordField.enteredText.length > 0) {
            passwordSubmitted(passwordField.enteredText, isPINEntry);
        }
    }
}

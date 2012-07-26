import Qt 4.7

Rectangle {
    id: itemRect
    width: parent.width
    height: 42
    color: "transparent"

    property Item content;
    property bool selectable: true
    property bool selected: false
    property bool forceSelected: false

    property int menuPosition:0 // 0 = middle, 1 = top, 2 = bottom

    BorderImage {
        id: highlight
        visible: (selectable && selected) || forceSelected
        source: menuPosition == 0 ? "/usr/palm/sysmgr/images/menu-selection-gradient-default.png" :
                ( menuPosition == 1 ? "/usr/palm/sysmgr/images/menu-selection-gradient-default.png" : "/usr/palm/sysmgr/images/menu-selection-gradient-last.png")
        width: parent.width - 8;
        height: parent.height
        anchors.horizontalCenter: parent.horizontalCenter
        border { left: 19; top: 0; right: 19; bottom: 0 }
        anchors.leftMargin: 5
        anchors.topMargin: 0
        anchors.bottomMargin: 0
        anchors.rightMargin: 5
    }

    Item {
        children: [content]
        y: (itemRect.height - content.height)/2
    }

    MouseArea {
        id: mouseArea
        enabled: selectable;
        anchors.fill: parent
        onPressAndHold:  setSelected(true);
        onPressed: { mouse.accepted = true; setSelected(true); }
        onReleased: {setSelected(false);}
        onExited: {setSelected(false);}
        onCanceled: {setSelected(false);}
        onClicked: {
            actionPerformed()
         }
    }

    function setSelected (select) {

        if(selectable) {
            selected = select;
        }
    }

    function actionPerformed () {
        if(selectable) {
            action()
        }
    }

    signal action()

    signal flickOverride(bool override)
}

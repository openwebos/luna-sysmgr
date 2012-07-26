import Qt 4.7

Column {
	id: drawer
        property MenuListEntry drawerHeader
        property Item drawerBody

        property bool active: true

        property int  maxViewHeight: 0

        state: "DRAWER_CLOSED"

        signal drawerOpened()
        signal drawerClosed()
        signal requestViewAdjustment(int offset)
        signal drawerFinishedClosingAnimation()

        function isOpen() {
            return (drawer.state == "DRAWER_OPEN");
        }

        function adjustViewIfNecessary() {
            if(maxViewHeight > 0) {
                var totalHeight = drawerHeader.height + body.childrenRect.height;

                if((drawer.y + totalHeight) > maxViewHeight) {
                    var offset = Math.min(((drawer.y + totalHeight) - maxViewHeight), drawer.y);
                    requestViewAdjustment(offset);
                }
            }
        }

        spacing: 0

        Rectangle {
        id: header
        width: parent.width
        height: drawerHeader.height
        color: "transparent"

        Item {
            width: parent.width
            children: drawerHeader
        }

        MouseArea {
            id: mouseArea
            anchors.fill: parent

            onPressed: { drawerHeader.setSelected(true); }
            onReleased: { drawerHeader.setSelected(false); }
            onExited: { drawerHeader.setSelected(false); }
            onCanceled: { drawerHeader.setSelected(false); }

            onClicked: {
                if(active) {
                     if (drawer.state == "DRAWER_CLOSED") {
                         open()
                     }
                     else if (drawer.state == "DRAWER_OPEN"){
                         close()
                     }
                     drawerHeader.actionPerformed()
                 }
             }
        }
    }

    Rectangle {
        id: body
        width: parent.width
        color: "transparent"
        clip: true
        children: { drawerBody }

        Behavior on height { NumberAnimation{ id: heightAnim; duration: 200} }

        onHeightChanged: {
                if(height == 0) {
                    drawerFinishedClosingAnimation()
                }
            }
    }

    states:[
        State {
            name: "DRAWER_OPEN"
            PropertyChanges { target: body; height: body.childrenRect.height}
        },
        State {
            name: "DRAWER_CLOSED"
            PropertyChanges { target: body; height: 0}
        }
    ]

    transitions: [
        Transition {
            to: "*"
            NumberAnimation { target: body; properties: "height"; duration: 350; easing.type:Easing.OutCubic }
        }
    ]

    function close () {
        if(drawer.state == "DRAWER_CLOSED")
            return;

        drawer.state = "DRAWER_CLOSED"
        drawerClosed()
    }

    function open () {
        if(drawer.state == "DRAWER_OPEN")
            return;

        drawer.state = "DRAWER_OPEN"
        drawerOpened()
        adjustViewIfNecessary();
    }
}

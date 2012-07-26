import Qt 4.7

Item {
    id: menuconatiner
    clip: true;

    property int  maxHeight: 410
    property int  headerIdent:   14
    property int  edgeOffset: 11
    property alias mainMenuItem: mainMenu;
    property alias scrollable: flickableArea.interactive;


    width: mainMenu.width + clipRect.anchors.leftMargin + clipRect.anchors.rightMargin;
    height: maxHeight;

    function setMaximumHeight(h) {
        maxHeight = h + clipRect.anchors.bottomMargin + clipRect.anchors.topMargin;
    }

    function setWidth(w) {
        width = w;
    }

    function setContent(menuContent) {
        mainMenu.children = [content]
    }

    signal menuScrollStarted();


    // ------------------------------------------------------------


    BorderImage {
        id: menuBorder
        source: "/usr/palm/sysmgr/images/menu-dropdown-bg.png"
        width: parent.width;
        height: Math.max(border.top + border.bottom, Math.min(menuconatiner.height,  (mainMenu.height + clipRect.anchors.topMargin + clipRect.anchors.bottomMargin)));
        border { left: 30; top: 10; right: 30; bottom: 30 }
    }

    Rectangle { // clipping rect inside the menu border
        id: clipRect
        anchors.fill: parent
        color: "transparent"
        clip: true
        anchors.leftMargin: 11
        anchors.topMargin: 0
        anchors.bottomMargin:15
        anchors.rightMargin: 11

        Flickable {
            id: flickableArea
            width: mainMenu.width;
            height: Math.min(menuconatiner.height - clipRect.anchors.topMargin - clipRect.anchors.bottomMargin, mainMenu.height);
            contentWidth: mainMenu.width; contentHeight: mainMenu.height

            onMovementStarted: {
                menuScrollStarted();
            }

            Item {
                id: mainMenu
                width: 320
                height:  700
            }
        }
    }

    Item {
        id: maskTop
        z:10
        width: parent.width - 22
        anchors.horizontalCenter: parent.horizontalCenter
        y: 0
        opacity: !flickableArea.atYBeginning ? 1.0 : 0.0

        BorderImage {
            width: parent.width
            source: "/usr/palm/sysmgr/images/menu-dropdown-scrollfade-top.png"
            border { left: 20; top: 0; right: 20; bottom: 0 }
        }

        Image {
            anchors.horizontalCenter: parent.horizontalCenter
            y:0
            source: "/usr/palm/sysmgr/images/menu-arrow-up.png"
        }

        Behavior on opacity { NumberAnimation{ duration: 70} }
    }

    Item {
        id: maskBottom
        z:10
        width: parent.width - 22
        anchors.horizontalCenter: parent.horizontalCenter
        y: flickableArea.height - 28
        opacity: !flickableArea.atYEnd ? 1.0 : 0.0

        BorderImage {
            width: parent.width
            source: "/usr/palm/sysmgr/images/menu-dropdown-scrollfade-bottom.png"
            border { left: 20; top: 0; right: 20; bottom: 0 }
        }

        Image {
            anchors.horizontalCenter: parent.horizontalCenter
            y:10
            source: "/usr/palm/sysmgr/images/menu-arrow-down.png"
        }

        Behavior on opacity { NumberAnimation{ duration: 70} }
    }


}

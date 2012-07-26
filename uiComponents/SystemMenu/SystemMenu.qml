import Qt 4.7


Item {
    id: systemmenu
    property int  maxHeight: 410
    property int  headerIdent:   14
    property int  subItemIdent:  16
    property int  dividerWidthOffset: 7
    property int  itemIdent:     subItemIdent + headerIdent
    property int  edgeOffset: 11
    property bool flickableOverride: false

    property bool airplaneModeInProgress: false

    width: 300; height: maxHeight;

    // ------------------------------------------------------------
    // External interface to the System Menu is defined here:


    // ---- Signals ----
    signal closeSystemMenu()
    signal airplaneModeTriggered()
    signal rotationLockTriggered(bool isLocked)
    signal muteToggleTriggered(bool isMuted)
    signal menuBrightnessChanged(real value, bool save)

    function setHeight(newheight) {
        if(newheight <= maxHeight) {
            height = newheight;
        }
        else {
            height = maxHeight;
        }
        // WORK-AROUND: changing the height directly was not causing atYEnd to update.. this pokes it
        flickableArea.resizeContent(flickableArea.contentWidth, flickableArea.contentHeight, 0.0);
    }

    function setMaximumHeight(height) {
        maxHeight = height;
    }

    function updateDate() {
        date.updateDate();
    }

    function setRotationLockText(newText, showLocked) {
        if(!rotation.delayUpdate) {
            rotation.modeText      = newText;
            rotation.locked        = showLocked;
        } else {
            rotation.newText       = newText;
            rotation.newLockStatus = showLocked;
        }
    }

    function setMuteControlText(newText, showMuteOn) {
        if(!muteControl.delayUpdate) {
            muteControl.muteText      = newText;
            muteControl.mute          = showMuteOn;
        } else {
            muteControl.newText       = newText;
            muteControl.newMuteStatus = showMuteOn;
        }
    }

    function setAirplaneModeStatus(newText, state) {
        airplane.modeText = newText;
        airplane.airplaneOn = ((state == 2) || (state == 3));
        airplaneModeInProgress = ((state == 1) || (state == 2));

        if(inProgress) {
            wifi.close();
            vpn.close();
            bluetooth.close();
        }
    }

    function updateChangedFields() {
        if(rotation.delayUpdate) {
            rotation.delayUpdate   = false;
            rotation.modeText      = rotation.newText;
            rotation.locked        = rotation.newLockStatus;
        }

        if(muteControl.delayUpdate) {
            muteControl.delayUpdate   = false;
            muteControl.muteText      = muteControl.newText;
            muteControl.mute          = muteControl.newMuteStatus;
        }
    }

    function setSystemBrightness(newValue) {
        brightness.brightnessValue = Math.max(0.0, Math.min(newValue, 1.0));
    }

    // ------------------------------------------------------------


    BorderImage {
        source: "/usr/palm/sysmgr/images/menu-dropdown-bg.png"
        width: parent.width;
        height: Math.min(systemmenu.height,  (mainMenu.height + clipRect.anchors.topMargin + clipRect.anchors.bottomMargin));
        border { left: 30; top: 10; right: 30; bottom: 30 }
    }

    Rectangle { // clipping rect inside the menu border
        id: clipRect
        anchors.fill: parent
        color: "transparent"
        clip: true
        anchors.leftMargin: 7
        anchors.topMargin: 0
        anchors.bottomMargin:14
        anchors.rightMargin: 7

        Flickable {
            id: flickableArea
            width: mainMenu.width;
            height: Math.min(systemmenu.height - clipRect.anchors.topMargin - clipRect.anchors.bottomMargin, mainMenu.height);
            contentWidth: mainMenu.width; contentHeight: mainMenu.height;
            interactive: !flickableOverride

            NumberAnimation on contentItem.y {
                id: viewAnimation;
                duration: 200;
                easing.type: Easing.InOutQuad
            }


            Column {
                id: mainMenu
                spacing: 0
                width: clipRect.width

                DateElement {
                    id: date
                    menuPosition: 1; // top
                    ident: headerIdent;
                }

                MenuDivider {widthOffset: dividerWidthOffset}

                BatteryElement {
                    id: battery
                    ident: headerIdent;
                }

                MenuDivider {widthOffset: dividerWidthOffset}

                BrightnessElement {
                    id: brightness
                    visible:    true
                    margin:      5;

                    onBrightnessChanged: {
                        menuBrightnessChanged(value, save);
                    }

                    onFlickOverride: {
                        flickableOverride = override;
                    }
                }

                MenuDivider {widthOffset: dividerWidthOffset}

                WiFiElement {
                    id: wifi
                    objectName: "wifiMenu"
                    visible: false
                    ident:         headerIdent;
                    internalIdent: subItemIdent;
                    active: !airplaneModeInProgress;
                    maxViewHeight : maxHeight - clipRect.anchors.topMargin - clipRect.anchors.bottomMargin;

                    onMenuCloseRequest: {
                        closeMenuTimer.interval = delayMs;
                        closeMenuTimer.start();
                    }

                    onRequestViewAdjustment: {
                        // this is not working correctly in QML right now.
//                        viewAnimation.to = flickableArea.contentItem.y - offset;
//                        viewAnimation.start();
                    }
                }

                MenuDivider {visible: wifi.visible; widthOffset: dividerWidthOffset}

                VpnElement {
                    id: vpn
                    objectName: "vpnMenu"
                    visible: false
                    ident:         headerIdent;
                    internalIdent: subItemIdent;
                    active: !airplaneModeInProgress;
                    maxViewHeight : maxHeight - clipRect.anchors.topMargin - clipRect.anchors.bottomMargin;

                    onMenuCloseRequest: {
                        closeMenuTimer.interval = delayMs;
                        closeMenuTimer.start();
                    }

                    onRequestViewAdjustment: {
                        // this is not working correctly in QML right now.
//                        viewAnimation.to = flickableArea.contentItem.y - offset;
//                        viewAnimation.start();
                    }
                }

                MenuDivider {visible: vpn.visible; widthOffset: dividerWidthOffset}

                BluetoothElement {
                    id: bluetooth
                    objectName: "bluetoothMenu"
                    visible: false
                    ident:         headerIdent;
                    internalIdent: subItemIdent;
                    active: !airplaneModeInProgress;
                    maxViewHeight : maxHeight - clipRect.anchors.topMargin - clipRect.anchors.bottomMargin;

                    onMenuCloseRequest: {
                        closeMenuTimer.interval = delayMs;
                        closeMenuTimer.start();
                    }

                    onRequestViewAdjustment: {
                        // this is not working correctly in QML right now.
//                        viewAnimation.to = flickableArea.contentItem.y - offset;
//                        viewAnimation.start();
                    }
                }

                MenuDivider {visible: bluetooth.visible; widthOffset: dividerWidthOffset}

                AirplaneModeElement {
                    id: airplane
                    visible:    true
                    ident:      headerIdent;
                    objectName: "airplaneMode"
                    selectable: !airplaneModeInProgress;

                    onAction: {
                        airplaneModeTriggered()

                        closeMenuTimer.interval = 250;
                        closeMenuTimer.start();
                    }
                }

                MenuDivider {visible: airplane.visible; widthOffset: dividerWidthOffset}

                RotationLockElement {
                    id: rotation
                    visible: true
                    ident:         headerIdent;

                    onAction: {
                        rotation.delayUpdate = true;
                        rotationLockTriggered(rotation.locked)

                        closeMenuTimer.interval = 250;
                        closeMenuTimer.start();
                    }
                }

                MenuDivider {visible: rotation.visible; widthOffset: dividerWidthOffset}

                MuteElement {
                    id: muteControl
                    visible: true
                    menuPosition: 2; // bottom
                    ident:         headerIdent;

                    onAction: {
                        muteControl.delayUpdate = true;
                        muteToggleTriggered(muteControl.mute)

                        closeMenuTimer.interval = 250;
                        closeMenuTimer.start();
                    }
                }
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
        y: flickableArea.height - 29
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


    Timer{
        id      : closeMenuTimer
        repeat  : false;
        running : false;

        onTriggered: closeSystemMenu()
    }


    property bool resetWhenInvisible: false;

    onVisibleChanged: {
        if(resetWhenInvisible) {
            resetMenu();
            resetWhenInvisible = false;
        }

        if(!visible) {
            updateChangedFields();
        }
    }

    function flagMenuReset() {
        resetWhenInvisible = true;
    }

    function resetMenu () {
        wifi.close();
        bluetooth.close();
        vpn.close();
    }
}

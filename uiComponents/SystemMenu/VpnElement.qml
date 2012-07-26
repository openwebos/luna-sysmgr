import Qt 4.7

Drawer {
    id: vpnMenu
    property int ident:        0
    property int internalIdent: 0
    property bool coloseOnConnect: false

    // ------------------------------------------------------------
    // External interface to the VPM Element is defined here:

    signal menuCloseRequest(int delayMs)
    signal menuOpened()
    signal menuClosed()
    signal prefsTriggered()
    signal itemSelected(string name, string status, string profInfo)
    signal adjustYPosition()

    function setVpnState(connected, state) {
        if(!connected) {
            vpnTitleState.text = runtime.getLocalizedString("Off");
        } else {
            vpnTitleState.text = state;
        }
    }

    function addVpnEntry(name, connStatus, profInfo) {
        vpnList.append({"entryText": name,
                        "connectionStatus": connStatus,
                        "vpnInfo": profInfo,
                        "isConnected": (connStatus == "connected"),
                        "showSelected": (connStatus == "connecting"),
                        "listIndex": vpnList.count})

        if(vpnMenu.isOpen() && (connStatus === "connected") && coloseOnConnect) {
            // close the menu upon connection
            menuCloseRequest(350);
            coloseOnConnect = false;
        }

	vpnListView.height = (vpnPrefs.height+separator.height) * vpnList.count
    }

    function clearVpnList() {
        vpnList.clear()
	vpnListView.height = 1
    }

    function forceDisconnectAllProfiles() {
        for(var index = 0; index < vpnList.count; index++) {
            var entry = vpnList.get(index)
            entry.connectionStatus = "disconnected";
            entry.isConnected = false;
        }
    }

    // ------------------------------------------------------------


    width: parent.width

    onDrawerOpened: {
        coloseOnConnect = false;
        menuOpened();
    }

    onDrawerClosed: menuClosed()

    drawerHeader:
    MenuListEntry {
        selectable: vpnMenu.active
        content: Item {
                    width: parent.width;

                    Text{
                        id: vpnTitle
                        text: runtime.getLocalizedString("VPN");
                        x: ident;
                        anchors.verticalCenter: parent.verticalCenter
                        color: vpnMenu.active ? "#FFF" : "#AAA";
                        font.bold: false;
                        font.pixelSize: 18
                        font.family: "Prelude"
                    }

                    Text {
                        id: vpnTitleState
                        x: vpnMenu.width - width - 14;
                        anchors.verticalCenter: parent.verticalCenter
                        text: runtime.getLocalizedString("init");
                        width: vpnMenu.width - vpnTitle.width - 35
                        horizontalAlignment: Text.AlignRight
                        elide: Text.ElideRight;
                        color: "#AAA";
                        font.pixelSize: 13
                        font.capitalization: Font.AllUppercase
                    }
                }
    }

    drawerBody:
    Column {
        spacing: 0
        width: parent.width

        MenuDivider { id: separator }

        ListView {
	    id: vpnListView
            width: parent.width
	    height: 1
            interactive: false
            spacing: 0
            model: vpnList
            delegate: vpnListDelegate
        }

        MenuListEntry {
            id: vpnPrefs
            selectable: true
            content: Text {
		x: ident + internalIdent;
		text: runtime.getLocalizedString("VPN Preferences");
		color: "#FFF";
		font.bold: false;
		font.pixelSize: 18;
		font.family: "Prelude"
	    }
            onAction: {
                prefsTriggered();
                menuCloseRequest(300);
            }
        }
    }

    Component {
        id: vpnListDelegate
        Column {
            spacing: 0
            width: parent.width
            property int index: listIndex

            MenuListEntry {
                id: entry
                selectable: true
                forceSelected: showSelected

                content: VpnEntry {
                            id: vpnData
                            x: ident + internalIdent;
                            width: vpnMenu.width-x;
                            name: entryText;
                            connected: isConnected;
                            connStatus: connectionStatus;
                            vpnProfileInfo: vpnInfo;
                         }

                onAction: {
                    itemSelected(vpnData.name, vpnData.connStatus, vpnData.vpnProfileInfo)
                    coloseOnConnect = false;

                    if(vpnData.connected) {
                        menuCloseRequest(350);
                    } else {
                        coloseOnConnect = true;
                    }
                }
            }

            MenuDivider { }

        }

    }

    ListModel {
        id: vpnList
    }
}


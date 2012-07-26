import Qt 4.7
import SystemMenu 1.0

Drawer {
    id: wifiMenu
    property int ident:         0
    property int internalIdent: 0

    property bool isWifiOn: false
    property bool coloseOnConnect: false

    // ------------------------------------------------------------
    // External interface to the WiFi Element is defined here:

    signal menuCloseRequest(int delayMs)
    signal menuOpened()
    signal menuClosed()
    signal onOffTriggered()
    signal prefsTriggered()
    signal itemSelected(int index, string name, int profileId, string securityType, string connState)

    function setWifiState(isOn, state) {
        if(!isWifiOn && isOn) {
            if(wifiMenu.state == "DRAWER_OPEN") {
                wifiSpinner.on = true;
            }
        }

        isWifiOn = isOn
        wifiTitleState.text = state

        if(!isWifiOn) {
            wifiSpinner.on = false;
            clearWifiList();
        }
    }

    function addWifiNetworkEntry(name, profId, sigBars, secType, connectionStatus, isConnected) {
        wifiList.append({"wifiName": name,
                         "profId":profId,
                         "sigBars": sigBars,
                         "secType": secType,
                         "connectionStatus": connectionStatus,
                         "isConnected": isConnected,
                         "listIndex": wifiList.count,
                         "itemStatus": "",
                         "boldStatus": false,
                         "showSelected": false
                        });
        wifiListView.height = (wifiOnOff.height+separator.height) * wifiList.count;
    }

    function clearWifiList() {
        wifiList.clear()
        wifiListView.height = 1
    }

    function wifiConnectStateUpdate(connected, ssid, state) {
        if(isWifiOn) {
            if(ssid != "") {
                for(var index = 0; index < wifiList.count; index++) {
                    var entry = wifiList.get(index)
                    entry.boldStatus = false;
                    if(entry.wifiName == ssid) {
                        if(state == "userSelected") {
                            entry.connectionStatus = "connecting";
                            entry.isConnected = false;
                            entry.itemStatus = runtime.getLocalizedString("Connecting...");
                            entry.showSelected = true;
                        } else if((state == "associated") || (state == "associating")) {
                            entry.connectionStatus = state;
                            entry.isConnected = false;
                            entry.itemStatus = runtime.getLocalizedString("Connecting...");
                        } else if((state == "ipFailed") || (state == "associationFailed")) {
                            entry.connectionStatus = state;
                            entry.isConnected = false;
                            if(state == "ipFailed") {
                                entry.itemStatus = runtime.getLocalizedString("IP configuration failed");
                                entry.boldStatus = true;
                            } else {
                                entry.itemStatus = runtime.getLocalizedString("Association failed");
                            }
                        } else if(state == "ipConfigured") {
                            entry.connectionStatus = state;
                            entry.isConnected = true;
                            entry.itemStatus = "";
                            if(index != 0) {
                                // move the connected item to the top
                                wifiList.move(index, 0, 1);
                            }

                            if(coloseOnConnect) {
                                menuCloseRequest(1000);
                                coloseOnConnect = false;
                            }
                        } else if(state == "notAssociated") {
                            entry.connectionStatus = "";
                            entry.isConnected = false;
                            entry.itemStatus = "";
                        }
                    } else {
                        entry.isConnected = false;
                        entry.itemStatus = "";
                        entry.connectionStatus = "";
                        entry.showSelected = false;
                    }
                }
            } else if (!connected){
                for(var index = 0; index < wifiList.count; index++) {
                    var entry = wifiList.get(index)
                    entry.isConnected = false;
                    entry.boldStatus = false;
                }
            }
        }
    }

    Connections {
        target: NativeSystemMenuHandler

        onWifiListUpdated:  {
            wifiSpinner.on = false;
        }
    }

    // ------------------------------------------------------------


    width: parent.width

    onDrawerOpened: menuOpened()
    onDrawerClosed: menuClosed()

    onDrawerFinishedClosingAnimation: {
        clearWifiList();
    }

    drawerHeader:
    MenuListEntry {
        selectable: wifiMenu.active
        content: Item {
                    width: parent.width;

                    Text{
                        id: wifiTitle
                        x: ident;
                        anchors.verticalCenter: parent.verticalCenter
                        text: runtime.getLocalizedString("Wi-Fi");
                        color: wifiMenu.active ? "#FFF" : "#AAA";
                        font.bold: false;
                        font.pixelSize: 18
                        font.family: "Prelude"
                    }

                    AnimatedSpinner {
                    //Spinner {
                        id: wifiSpinner
                        x: wifiTitle.width + 18;
                        y:-17
                        on:false
                    }

                    Text {
                        id: wifiTitleState
                        x: wifiMenu.width - width - 14;
                        anchors.verticalCenter: parent.verticalCenter
                        text: runtime.getLocalizedString("init");
                        width: wifiMenu.width - wifiTitle.width - 60
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

        MenuDivider  { id: separator }

        MenuListEntry {
            id: wifiOnOff
            selectable: true
            content: Text {  id: wifiOnOffText;
                             x: ident + internalIdent;
                             text: isWifiOn ? runtime.getLocalizedString("Turn off WiFi") : runtime.getLocalizedString("Turn on WiFi");
                             color: "#FFF";
                             font.bold: false;
                             font.pixelSize: 18
                             font.family: "Prelude"
                         }

            onAction: {
                onOffTriggered()
                wifiSpinner.on = !isWifiOn;
                if(isWifiOn) {
                    menuCloseRequest(300);
                } else {
                    coloseOnConnect = true;
                }
            }
        }

        MenuDivider {}

        ListView {
            id: wifiListView
            width: parent.width
            interactive: false
            spacing: 0
            height: 1
            model: wifiList
            delegate: wifiListDelegate
        }

        MenuListEntry {
            selectable: true
            content: Text { x: ident + internalIdent; text: runtime.getLocalizedString("Wi-Fi Preferences"); color: "#FFF"; font.bold: false; font.pixelSize: 18; font.family: "Prelude"}
            onAction: {
                clearWifiList()
                prefsTriggered()
                menuCloseRequest(300);
            }
        }
    }

    Component {
        id: wifiListDelegate
        Column {
            spacing: 0
            width: parent.width
            property int index: listIndex

            MenuListEntry {
                id: entry
                selectable: true
                forceSelected: showSelected

                content: WifiEntry {
                            id: wifiNetworkData
                            x: ident + internalIdent;
                            width: wifiMenu.width-x;
                            name:         wifiName;
                            profileId:    profId;
                            signalBars:   sigBars;
                            securityType: secType;
                            connStatus:   connectionStatus;
                            status:       itemStatus;
                            statusInBold: boldStatus;
                            connected:    isConnected;
                         }
                onAction: {
                    itemSelected(index,
                                 wifiNetworkData.name,
                                 wifiNetworkData.profileId,
                                 wifiNetworkData.securityType,
                                 wifiNetworkData.connStatus)

                    if((wifiNetworkData.connStatus == "ipConfigured") ||
                       (wifiNetworkData.connStatus == "associated") ||
                       (wifiNetworkData.connStatus == "ipFailed") ||
                       (wifiNetworkData.connStatus == "associationFailed")  ) {
                        menuCloseRequest(300);
                    } else if((wifiNetworkData.profileId == 0) && (wifiNetworkData.securityType != "")) {
                        menuCloseRequest(300);
                    }

                    coloseOnConnect = true;
                }
            }

            MenuDivider {}
        }

    }

    ListModel {
        id: wifiList
    }

    onMenuOpened: {
        coloseOnConnect = false;
        if(isWifiOn) {
            wifiSpinner.on = true
        }
    }

    onMenuClosed: {
        coloseOnConnect = false;
        wifiSpinner.on = false
    }
}


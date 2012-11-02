import Qt 4.7

Image {
    property real uiScale: 1.0
    property real layoutScale: 1.0
    property int widthOffset: 7 * layoutScale
    scale: uiScale
    width: (parent.width - widthOffset) / uiScale
    anchors.horizontalCenter: parent.horizontalCenter
    source: "/usr/palm/sysmgr/images/menu-divider.png"
}

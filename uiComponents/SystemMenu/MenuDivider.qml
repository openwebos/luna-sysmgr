import Qt 4.7

Image {
    property int widthOffset: 7
    width: parent.width - widthOffset
    anchors.horizontalCenter: parent.horizontalCenter
    source: "/usr/palm/sysmgr/images/menu-divider.png"
}

import Qt 4.7

Image {
        id: spinner
        property bool on: true

        source: "/usr/palm/sysmgr/images/activity-indicator-single-32x32.png"
        visible: spinner.on
        smooth: true

        NumberAnimation on rotation {
        running: spinner.on; from: 0; to: 360; loops: Animation.Infinite; duration: 3000
        }
}

	

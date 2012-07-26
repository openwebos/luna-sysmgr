import Qt 4.7

Rectangle {
    width: 1024
    height: 768

    property bool mainTimerRunning: false
    property int isLandscape: (runtime.orientation+1)%2

    Image {
        id: bg
        source: "../../images/dockmode/time/clock_bg.png"
    }

    VisualItemModel{
        id: clockList
        AnalogClock{glass: 1; timerRunning: mainTimerRunning}
        DigitalClock{timerRunning: mainTimerRunning}
        AnalogClock{glass: 0; timerRunning: mainTimerRunning}
    }

    ListView {
        id: flickable
        anchors.fill: parent
        focus: true
        highlightRangeMode: ListView.StrictlyEnforceRange
        orientation: ListView.Horizontal
        snapMode: ListView.SnapOneItem
        model: clockList
        boundsBehavior: Flickable.DragOverBounds
    }

    Row {
         spacing: 10
         anchors.centerIn: parent
         anchors.verticalCenterOffset: isLandscape ? 340 : 400
         Image { id: clockdot1; source: "../../images/dockmode/time/indicator/"+(flickable.currentIndex==0 ? "on" : "off") + ".png" }
         Image { id: clockdot2; source: "../../images/dockmode/time/indicator/"+(flickable.currentIndex==1 ? "on" : "off") + ".png" }
         Image { id: clockdot3; source: "../../images/dockmode/time/indicator/"+(flickable.currentIndex==2 ? "on" : "off") + ".png" }
    }
}




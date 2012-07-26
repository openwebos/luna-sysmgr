import Qt 4.7

Item {
    id: analogclock
    width: 1024; height: 768

    property int glass: 1
    property variant type: ["matte","glass"]
    property bool timerRunning: false

    function getHours() {
        var date = new Date
        return date.getHours()
    }

    function getMinutes(){
        var date = new Date
        return date.getMinutes()
    }

    function getSeconds(){
        var date = new Date
        return date.getSeconds()
    }

    function getDay(){
        var date = new Date 
        var dateString=glass ? Qt.formatDate(date, Qt.DefaultLocaleLongDate) : date.getDate();
        return dateString;
    }

    function getWeekDay(){
        return runtime.getLocalizedDay();
    }

    property int hours: getHours()
    property int minutes: getMinutes()
    property int seconds: getSeconds()
    property int milliseconds
    property int secondAdjustment
    property string day:getDay()
    property string weekday:getWeekDay()

    function timeChanged() {
        var date = new Date;
        hours = getHours();
        minutes = getMinutes();
        seconds = getSeconds();
        day = getDay();
        weekday = getWeekDay();
    }

    Timer {
        interval: 100; running: timerRunning; repeat: true;
        onTriggered: analogclock.timeChanged()
    }

     Image {
         id: face
         source: "../../images/dockmode/time/analog/"+type[glass]+"/base.png"
         anchors.centerIn: parent
     }

     Text {
         id: dayText
         text: day
         parent: face
         anchors.centerIn: parent
         anchors.horizontalCenterOffset: glass ? 0 : 108
         anchors.verticalCenterOffset: glass ? 300 : -2
         font.family: "prelude"
         font.pointSize: glass ? 30 : 30
         color: "#e1e1e1"
     }

     Text {
         id: dayofWeekText
         text: weekday
         visible: !glass
         parent: face
         anchors.centerIn: parent
         anchors.horizontalCenterOffset: glass ? 0 : -108
         anchors.verticalCenterOffset: glass ? 260 : -2
         font.family: "prelude"
         font.pointSize: glass ? 16 : 30
         color: "#e1e1e1"
     }

     Image {
         id: hourHand
         source: "../../images/dockmode/time/analog/"+type[glass]+"/hour.png"
         anchors.centerIn: parent
         parent: face
         smooth: true
         transform: Rotation {
             id: hourRotation
             origin.x: hourHand.width/2; origin.y: hourHand.height/2;
             angle: (analogclock.hours * 30) + (analogclock.minutes * 0.5)
             Behavior on angle {
                 RotationAnimation{ direction: RotationAnimation.Clockwise }
             }
         }
     }

     Image {
         id: minuteHand
         source: "../../images/dockmode/time/analog/"+type[glass]+"/minute.png"
         anchors.centerIn: parent
         parent: face
         smooth: true
         transform: Rotation {
             id: minuteRotation
             origin.x: minuteHand.width/2; origin.y: minuteHand.height/2;
             angle: analogclock.minutes * 6
             Behavior on angle {
                 RotationAnimation{ direction: RotationAnimation.Clockwise }
             }
         }

     }

     Image {
         id: secondHand
         source: glass ? "" : "../../images/dockmode/time/analog/"+type[glass]+"/second.png"
         anchors.centerIn: parent
         parent: face
         smooth: true
         transform: Rotation {
             id: secondRotation
             origin.x: secondHand.width/2; origin.y: secondHand.height/2;
             angle: analogclock.seconds * 6
             Behavior on angle {
                 RotationAnimation{ direction: RotationAnimation.Clockwise; easing.type: Easing.OutBack; duration: 300 }
             }
         }
     }
 }

import Qt 4.7

Item {
    id: digitalclock
    width: 1024; height: 768

    //constants
    property int timeOffset: -4
    property int dateOffset: -4
    property int timeLandSize: 158//205
    property int timePortSize: 132//175
    property int dateLandSize: 52//60
    property int datePortSize: 44//55
    property bool timerRunning: false

    function setHours() {
        var d = new Date

	return runtime.twelveHourClock ? (d.getHours() > 12 ? d.getHours()-12: d.getHours() == 0 ? 12 : d.getHours()) : d.getHours();
    }

    function setMinutes(){
        var d = new Date
        return d.getMinutes()
    }

    function setDate(){
        var d = new Date
        return d.getDate()
    }

    function setYear(){
        var d = new Date
        return d.getFullYear()
    }

    function setMonth(){
        var d = new Date
        var monthString=Qt.formatDate(d,"MMM");
        return runtime.getLocalizedMonth();
    }

    function setAMPM(){
        var d = new Date
        var ampmString= Qt.formatTime(d,"AP");
        return runtime.getLocalizedAMPM();
    }

    property int hours: setHours()
    property int minutes: setMinutes()
    property int date: setDate()
    property int year: setYear()
    property string month: setMonth()
    property string ampm: setAMPM()
    property int isLandscape: (runtime.orientation+1)%2
    property variant orientation: ["portrait", "landscape"]

    function timeChanged() {
        var d = new Date;
        hours = setHours();
        minutes = setMinutes()

        date = setDate();
        year = setYear();
        month = setMonth();
        ampm = setAMPM();

    }

    Timer {
        interval: 100; running: timerRunning; repeat: true;
        onTriggered: digitalclock.timeChanged()
    }

    Row {
         spacing: 0
         anchors.centerIn: parent
         anchors.verticalCenterOffset: -48
         Image { id: bgHour1; source: "../../images/dockmode/time/digital/"+orientation[isLandscape]+"/flippers-time.png" }
         Item {  width: 4; height: 50 }
         Image { id: bgHour2; source: "../../images/dockmode/time/digital/"+orientation[isLandscape]+"/flippers-time.png" }
         Item {  width: 22; height: 50 }
         Image { id: dots; source: "../../images/dockmode/time/digital/"+orientation[isLandscape]+"/dots.png" }
         Item {  width: 22; height: 50 }
         Image { id: bgMin1; source: "../../images/dockmode/time/digital/"+orientation[isLandscape]+"/flippers-time.png"}
         Item {  width: 4; height: 50 }
         Image { id: bgMin2; source: "../../images/dockmode/time/digital/"+orientation[isLandscape]+"/flippers-time.png"}
    }

    Row {
         spacing: 2
         anchors.centerIn: parent
         anchors.verticalCenterOffset: 136
         Image { id: bgMonth1; source: "../../images/dockmode/time/digital/"+orientation[isLandscape]+"/flippers-date.png" }
         Image { id: bgMonth2; source: "../../images/dockmode/time/digital/"+orientation[isLandscape]+"/flippers-date.png" }
         Image { id: bgMonth3; source: "../../images/dockmode/time/digital/"+orientation[isLandscape]+"/flippers-date.png" }
         Image { id: bgBlank1; source: "../../images/dockmode/time/digital/"+orientation[isLandscape]+"/flippers-date.png" }
         Image { id: bgDay1; source: "../../images/dockmode/time/digital/"+orientation[isLandscape]+"/flippers-date.png" }
         Image { id: bgDay2; source: "../../images/dockmode/time/digital/"+orientation[isLandscape]+"/flippers-date.png" }
         Image { id: bgBlank2; source: "../../images/dockmode/time/digital/"+orientation[isLandscape]+"/flippers-date.png" }
         Image { id: bgYear1; source: "../../images/dockmode/time/digital/"+orientation[isLandscape]+"/flippers-date.png" }
         Image { id: bgYear2; source: "../../images/dockmode/time/digital/"+orientation[isLandscape]+"/flippers-date.png" }
         Image { id: bgYear3; source: "../../images/dockmode/time/digital/"+orientation[isLandscape]+"/flippers-date.png" }
         Image { id: bgYear4; source: "../../images/dockmode/time/digital/"+orientation[isLandscape]+"/flippers-date.png" }
    }

    Text {
        id: ampmText
        text: runtime.twelveHourClock ? ampm : ""
        anchors.verticalCenterOffset: isLandscape ? -95 : -80
        anchors.horizontalCenterOffset: isLandscape ? -42 : -38
        anchors.centerIn: parent; font.family: "prelude"; font.pointSize: isLandscape ? 20 : 15; color: "#e1e1e1"; smooth: true
        parent: bgHour1
    }


    Text {
        id: hourTens
        text: parseInt(hours/10)
        anchors.verticalCenterOffset: timeOffset; anchors.centerIn: parent; font.family: "prelude"; font.pointSize: isLandscape ? timeLandSize : timePortSize; color: "#e1e1e1"; smooth: true
        parent: bgHour1
    }

    Text {
        id: hourOnes
        text: hours%10
        anchors.verticalCenterOffset: timeOffset; anchors.centerIn: parent; font.family: "prelude"; font.pointSize: isLandscape ? timeLandSize : timePortSize; color: "#e1e1e1"; smooth: true
        parent: bgHour2
    }

    Text {
        id: minuteTens
        text: parseInt(minutes/10)
        anchors.verticalCenterOffset: timeOffset; anchors.centerIn: parent; font.family: "prelude"; font.pointSize: isLandscape ? timeLandSize : timePortSize; color: "#e1e1e1"; smooth: true
        parent: bgMin1
    }
    Text {
        id: minuteOnes
        text: minutes%10
        anchors.verticalCenterOffset: timeOffset; anchors.centerIn: parent; font.family: "prelude"; font.pointSize: isLandscape ? timeLandSize : timePortSize; color: "#e1e1e1"; smooth: true
        parent: bgMin2
    }



    Text {
        id: monthFirst
        text: month.substring(0,1)
        anchors.verticalCenterOffset: dateOffset; anchors.centerIn: parent; font.family: "prelude"; font.pointSize: isLandscape ? dateLandSize : datePortSize; color: "#e1e1e1"; smooth: true
        parent: bgMonth1
    }
    Text {
        id: monthSecond
        text: month.substring(1,2)
        anchors.verticalCenterOffset: dateOffset; anchors.centerIn: parent; font.family: "prelude"; font.pointSize: isLandscape ? dateLandSize : datePortSize; color: "#e1e1e1"; smooth: true
        parent: bgMonth2
    }

    Text {
        id: monthThird
        text: month.substring(2,3)
        anchors.verticalCenterOffset: dateOffset; anchors.centerIn: parent; font.family: "prelude"; font.pointSize: isLandscape ? dateLandSize : datePortSize; color: "#e1e1e1"; smooth: true
        parent: bgMonth3
    }

    Text {
        id: dayTens
        text: parseInt(date/10)
        anchors.verticalCenterOffset: dateOffset; anchors.centerIn: parent; font.family: "prelude"; font.pointSize: isLandscape ? dateLandSize : datePortSize; color: "#e1e1e1"; smooth: true
        parent: bgDay1
    }
    Text {
        id: dayOnes
        text: date%10
        anchors.verticalCenterOffset: dateOffset; anchors.centerIn: parent; font.family: "prelude"; font.pointSize: isLandscape ? dateLandSize : datePortSize; color: "#e1e1e1"; smooth: true
        parent: bgDay2
    }

    Text {
        id: yearThousands
        text: parseInt(year/1000)
        anchors.verticalCenterOffset: dateOffset; anchors.centerIn: parent; font.family: "prelude"; font.pointSize: isLandscape ? dateLandSize : datePortSize; color: "#e1e1e1"; smooth: true
        parent: bgYear1
    }
    Text {
        id: yearHundreads
        text: parseInt(year/100)%10
        anchors.verticalCenterOffset: dateOffset; anchors.centerIn: parent; font.family: "prelude"; font.pointSize: isLandscape ? dateLandSize : datePortSize; color: "#e1e1e1"; smooth: true
        parent: bgYear2
    }

    Text {
        id: yearTens
        text: parseInt(year/10)%10
        anchors.verticalCenterOffset: dateOffset; anchors.centerIn: parent; font.family: "prelude"; font.pointSize: isLandscape ? dateLandSize : datePortSize; color: "#e1e1e1"; smooth: true
        parent: bgYear3

    }
    Text {
        id: yearOnes
        text: year%10
        anchors.verticalCenterOffset: dateOffset; anchors.centerIn: parent; font.family: "prelude"; font.pointSize: isLandscape ? dateLandSize : datePortSize; color: "#e1e1e1"; smooth: true
        parent: bgYear4
    }


    Row {
         spacing: 0
         anchors.centerIn: parent
         anchors.verticalCenterOffset: -48
         Image { id: bgHour1Mask; source: "../../images/dockmode/time/digital/"+orientation[isLandscape]+"/flippers-time-mask.png" }
         Item {  width: 4; height: 50 }
         Image { id: bgHour2Mask; source: "../../images/dockmode/time/digital/"+orientation[isLandscape]+"/flippers-time-mask.png" }
         Item {  width: 72; height: 50 }
         Image { id: bgMin1Mask; source: "../../images/dockmode/time/digital/"+orientation[isLandscape]+"/flippers-time-mask.png"}
         Item {  width: 4; height: 50 }
         Image { id: bgMin2Mask; source: "../../images/dockmode/time/digital/"+orientation[isLandscape]+"/flippers-time-mask.png"}
    }



    Row {
         spacing: 2
         anchors.centerIn: parent
         anchors.verticalCenterOffset: 136
         Image { id: bgMonth1Mask; source: "../../images/dockmode/time/digital/"+orientation[isLandscape]+"/flippers-date-mask.png" }
         Image { id: bgMonth2Mask; source: "../../images/dockmode/time/digital/"+orientation[isLandscape]+"/flippers-date-mask.png" }
         Image { id: bgMonth3Mask; source: "../../images/dockmode/time/digital/"+orientation[isLandscape]+"/flippers-date-mask.png" }
         Image { id: bgBlank1Mask; source: "../../images/dockmode/time/digital/"+orientation[isLandscape]+"/flippers-date-mask.png" }
         Image { id: bgDay1Mask; source: "../../images/dockmode/time/digital/"+orientation[isLandscape]+"/flippers-date-mask.png" }
         Image { id: bgDay2Mask; source: "../../images/dockmode/time/digital/"+orientation[isLandscape]+"/flippers-date-mask.png" }
         Image { id: bgBlank2Mask; source: "../../images/dockmode/time/digital/"+orientation[isLandscape]+"/flippers-date-mask.png" }
         Image { id: bgYear1Mask; source: "../../images/dockmode/time/digital/"+orientation[isLandscape]+"/flippers-date-mask.png" }
         Image { id: bgYear2Mask; source: "../../images/dockmode/time/digital/"+orientation[isLandscape]+"/flippers-date-mask.png" }
         Image { id: bgYear3Mask; source: "../../images/dockmode/time/digital/"+orientation[isLandscape]+"/flippers-date-mask.png" }
         Image { id: bgYear4Mask; source: "../../images/dockmode/time/digital/"+orientation[isLandscape]+"/flippers-date-mask.png" }
    }

}

import QtQuick
import QtQuick.Controls
import QtMultimedia

ApplicationWindow {
    id: window
    visible: true
    width: 720 //matched height and width to touch display 2
    height: 1280
    color: "#000000"

    Item {
        id: rotator //rotates whole ui as one thing
        width: root.width
        height: root.height
        anchors.centerIn: parent
        transform: Rotation {
            angle: 90
            origin.x: rotator.width / 2
            origin.y: rotator.height / 2
        }

        Item {
            id: root
            width: 1280
            height: 720

            //load resources------------------------------------
            FontLoader {
                id: jetBrainsMonoNerdFont
                source: "qrc:/qt/qml/QTralphEdisplay/fonts/JetBrainsMonoNerdFont-Regular.ttf"
            }

            SoundEffect {
                id: alarmSound
                source: "qrc:/qt/qml/QTralphEdisplay/sounds/alarm.wav"
            }

            //time and date logic------------------------------

            //get current time
            function currentTime(){
                return Qt.formatTime(new Date(), "h:mm AP")
            }

            //get current date
            function currentDate(){
                return Qt.formatDate(new Date(), "ddd, MMMM d yyyy")
            }

            //timers for fetching data-----------------------------

            //1 sec timer
            Timer {
                interval: 1000
                running: true
                repeat: true
                triggeredOnStart: true
                onTriggered: {
                    clockText.text = root.currentTime()
                    dateText.text = root.currentDate()
                }
            }

            //longer update timer
            Timer {
                interval: 10000
                running: true
                repeat: true
                triggeredOnStart: true
                onTriggered: {
                    backend.fetchEvents()
                    backend.fetchWeather()
                    backend.loadPictures()
                }
            }

            //left column-------------------------------------
            Column {
                id: leftColumn
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.margins: 24
                width:426
                spacing: 4

                Text {
                    id: clockText
                    text: root.currentTime()
                    width: parent.width
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignHCenter
                    font.pixelSize: 80
                    font.family: "jetBrainsMonoNerdFont"
                    color: "#ffffff"
                }

                Text {
                    id: dateText
                    text: root.currentDate()
                    width: parent.width
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignHCenter
                    font.pixelSize: 32
                    font.family: "jetBrainsMonoNerdFont"
                    color: "#ffffff"
                }
                Text {
                    id: eventsText
                    text: "\n" + backend.events
                    width: parent.width
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignHCenter
                    font.pixelSize: 20
                    font.family: "jetBrainsMonoNerdFont"
                    color: "#ffffff"

                }
            }

            //center column------------------------------------------------------
            Column {
                id: centerColumn
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.left: leftColumn.right
                anchors.right: rightColumn.left
                anchors.margins: 24
                spacing: 4

                //GUI timer-------------------------------------------
                property int totalSeconds: 0
                property int setSeconds: 0
                property bool timerRunning: false

                Timer {
                    id: ticker
                    interval: 1000
                    repeat: true
                    running: centerColumn.timerRunning
                    onTriggered: {
                        if (centerColumn.totalSeconds > 0) {
                            centerColumn.totalSeconds--
                        } else {
                            centerColumn.timerRunning = false
                            alarmSound.play()
                        }
                    }
                }

                function format(s) {
                    let h = Math.floor(s / 3600)
                    let m = Math.floor((s % 3600) / 60)
                    let sec = s % 60
                    return String(h).padStart(2, '0') + ":" +
                    String(m).padStart(2, '0') + ":" +
                    String(sec).padStart(2, '0')
                }

                Row {
                    spacing: 10
                    anchors.horizontalCenter: parent.horizontalCenter

                    Button {
                        text: "Reset"
                        font.family: "jetBrainsMonoNerdFont"
                        implicitWidth: 100
                        implicitHeight: 50
                        onClicked: {
                            alarmSound.stop()
                            centerColumn.timerRunning = false
                            centerColumn.totalSeconds = 0
                        }
                    }

                    Button {
                        text: centerColumn.timerRunning ? "Pause" : "Start"
                        font.family: "jetBrainsMonoNerdFont"
                        implicitWidth: 100
                        implicitHeight: 50
                        onClicked: {
                            if (centerColumn.totalSeconds > 0)
                                centerColumn.timerRunning = !centerColumn.timerRunning
                        }
                    }
                }

                Text {
                    id: timerText
                    text: centerColumn.format(centerColumn.totalSeconds)
                    font.family: "jetBrainsMonoNerdFont"
                    font.pixelSize: 48
                    color: "white"
                    anchors.horizontalCenter: parent.horizontalCenter

                    SequentialAnimation on color{
                        id: flashAnimation
                        running: alarmSound.playing
                        loops: Animation.Infinite
                        ColorAnimation { to: "red"; duration: 500 }
                        ColorAnimation { to: "white"; duration: 500 }
                        onStopped: timerText.color = "white"
                    }
                }

                //buttons
                Row {
                    spacing: 10
                    anchors.horizontalCenter: parent.horizontalCenter

                    Button {
                        text: "-15 sec"
                        font.family: "jetBrainsMonoNerdFont"
                        implicitWidth: 100
                        implicitHeight: 50

                        //repeat on hold
                        property bool held: false
                        Timer {
                            interval: 250
                            repeat: true
                            running: parent.held
                            onTriggered: {
                                centerColumn.totalSeconds = Math.max(0, centerColumn.totalSeconds - 15)
                            }
                        }
                        onPressed: held = true
                        onReleased: held = false

                        onClicked: {
                            //subtract 15, but don't go below 0
                            centerColumn.totalSeconds = Math.max(0, centerColumn.totalSeconds - 15)
                        }
                    }
                    Button {
                        text: "+15 sec"
                        font.family: "jetBrainsMonoNerdFont"
                        implicitWidth: 100
                        implicitHeight: 50

                        //repeat on hold
                        property bool held: false
                        Timer {
                            interval: 250
                            repeat: true
                            running: parent.held
                            onTriggered: {
                                alarmSound.stop()
                                centerColumn.totalSeconds += 15
                            }
                        }
                        onPressed: held = true
                        onReleased: held = false

                        onClicked: {
                            alarmSound.stop()
                            centerColumn.totalSeconds += 15
                        }
                    }
                }
            }

            //logo-------------------------------------------
            Image {
                id: logo
                source: "qrc:/qt/qml/QTralphEdisplay/icons/pooleIndustriesWhite.svg"
                anchors.horizontalCenter: centerColumn.horizontalCenter
                anchors.bottom: centerColumn.bottom
                width: centerColumn.width
                fillMode: Image.PreserveAspectFit
            }

            //right column----------------------------------------------
            Column {
                id: rightColumn
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.right: parent.right
                anchors.margins: 24
                width:426
                spacing: 4

                Text {
                    id: liveWeather
                    text: backend.liveWeather
                    width: parent.width
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignHCenter
                    font.pixelSize: 48
                    font.family: "jetBrainsMonoNerdFont"
                    color: "#ffffff"
                }

                Text {
                    id: weatherForcast
                    text: backend.weather
                    width: parent.width
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignHCenter
                    font.pixelSize: 24
                    font.family: "jetBrainsMonoNerdFont"
                    color: "#ffffff"
                }
            }

            //picture frame---------------------------------------------
            property int photoIndex: 0

            Timer {
                id: photoTimer
                interval: 10000
                running: true
                repeat: true
                triggeredOnStart: true
                onTriggered: {
                    if (backend.pictures.length > 0)
                        root.photoIndex = (root.photoIndex + 1) % backend.pictures.length
                    if (root.photoIndex > backend.pictures.length)
                        root.photoIndex = 0
                }
            }

            Image {
                anchors.bottom: rightColumn.bottom
                anchors.horizontalCenter: rightColumn.horizontalCenter
                width: 300
                height: 350
                fillMode: Image.PreserveAspectCrop
                source: backend.pictures.length > 0 ? backend.pictures[root.photoIndex] : ""

                //frame around image
                Rectangle {
                    anchors.fill: parent
                    color: "transparent"
                    border.color: "white"
                    border.width: 6
                }
            }
        }
    }
}

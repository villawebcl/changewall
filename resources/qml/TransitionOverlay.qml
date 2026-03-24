import QtQuick 2.15

Item {
    id: root

    property url oldSource: ""
    property url newSource: ""
    property real progress: 0.0

    Rectangle {
        anchors.fill: parent
        color: "black"
    }

    Item {
        anchors.fill: parent
        clip: true

        Image {
            anchors.fill: parent
            source: root.oldSource
            fillMode: Image.PreserveAspectCrop
            asynchronous: true
            cache: false
            smooth: true
            mipmap: true
            opacity: 1.0 - (root.progress * 0.85)
            scale: 1.0 + ((1.0 - root.progress) * 0.035)
            transformOrigin: Item.Center
        }

        Image {
            anchors.fill: parent
            source: root.newSource
            fillMode: Image.PreserveAspectCrop
            asynchronous: true
            cache: false
            smooth: true
            mipmap: true
            opacity: Math.min(1.0, root.progress * 1.15)
            scale: 1.045 - (root.progress * 0.045)
            transformOrigin: Item.Center
        }
    }

    Image {
        anchors.fill: parent
        source: ""
    }

    Rectangle {
        anchors.fill: parent
        color: "black"
        opacity: 0.06 * (1.0 - Math.abs((root.progress * 2.0) - 1.0))
    }

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: Qt.rgba(0, 0, 0, 0.18) }
            GradientStop { position: 0.2; color: Qt.rgba(0, 0, 0, 0.04) }
            GradientStop { position: 0.8; color: Qt.rgba(0, 0, 0, 0.04) }
            GradientStop { position: 1.0; color: Qt.rgba(0, 0, 0, 0.18) }
        }
        opacity: 0.35
    }
}

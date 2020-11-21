import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilnotes 1.0

import "harbour"

CoverBackground {
    id: cover

    property bool encryptedPageSelected
    readonly property int coverActionHeight: Theme.itemSizeSmall

    signal newNote()

    Rectangle {
        id: titleBackground

        anchors.fill: appTitle
        color: Theme.primaryColor
        opacity: HarbourTheme.opacityFaint
    }

    Label {
        id: appTitle

        width: parent.width
        height: implicitHeight + Theme.paddingLarge
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        anchors.top: parent.top
        //: Application title
        //% "Foil Notes"
        text: qsTrId("foilnotes-app_name")
    }

    HarbourHighlightIcon {
        id: leftEdge

        y: titleBackground.height
        source: "images/edge.svg"
        sourceSize.height: Theme.paddingMedium
        highlightColor: titleBackground.color
        opacity: HarbourTheme.opacityFaint
        smooth: true
    }

    Repeater {
        id: edge

        model: Math.ceil(titleBackground.width/leftEdge.width) - 1
        delegate: Component {
            HarbourHighlightIcon {
                x: (index + 1) * width
                y: leftEdge.y
                source: "images/edge.svg"
                sourceSize.height: leftEdge.height
                highlightColor: leftEdge.highlightColor
                opacity: leftEdge.opacity
                smooth: true
            }
        }
    }

    Item {
        id: content

        readonly property int lineCount: Math.round((height - coverActionHeight/cover.parent.scale)/encryptedLabel.lineHeight)
        property bool locking

        width: cover.width * 2
        anchors {
            top: leftEdge.bottom
            bottom: parent.bottom
        }

        x: (encryptedPageSelected && FoilNotesModel.foilState === FoilNotesModel.FoilNotesReady) ? 0 : -cover.width
        Behavior on x {
            SmoothedAnimation {
                id: transition

                duration: 200
            }
        }

        Binding {
            target: encryptedLabel
            property: "text"
            value: FoilNotesModel.text
            when: !transition.running && !content.locking
        }

        Repeater {
            model: content.lineCount
            delegate: Rectangle {
                y: (index + 1) * encryptedLabel.lineHeight
                width: content.width
                height: Theme.paddingSmall/4
                color: Theme.primaryColor
                opacity: HarbourTheme.opacityLow
            }
        }

        Image {
            readonly property real size: Math.floor(3*cover.width/5)
            x: (cover.width - width)/2
            y: (cover.height - height)/2 - parent.y
            height: size
            sourceSize.height: size
            source: HarbourTheme.darkOnLight ? "images/fancy-lock-dark.svg" : "images/fancy-lock.svg"
            opacity: 0.1
        }

        Label {
            id: encryptedLabel

            x: Theme.paddingSmall/2
            y: Theme.paddingSmall/2
            opacity: 0.6
            font.italic: true
            width: cover.width - Theme.paddingSmall
            horizontalAlignment: Text.AlignLeft
            lineHeightMode: Text.FixedHeight
            lineHeight: Math.floor(appTitle.implicitHeight + Theme.paddingSmall)
            wrapMode: Text.Wrap
            elide: Text.ElideRight
            maximumLineCount: content.lineCount
        }

        Label {
            id: plainTextLabel

            x: encryptedLabel.x + cover.width
            y: encryptedLabel.y
            text: FoilNotesPlaintextModel.text
            font.italic: true
            opacity: encryptedLabel.opacity
            width: encryptedLabel.width
            horizontalAlignment: Text.AlignLeft
            lineHeightMode: Text.FixedHeight
            lineHeight: encryptedLabel.lineHeight
            wrapMode: Text.Wrap
            elide: Text.ElideRight
            maximumLineCount: encryptedLabel.maximumLineCount
        }
    }

    CoverActionList {
        CoverAction {
            readonly property url lockIcon: Qt.resolvedUrl("images/" + (HarbourTheme.darkOnLight ? "lock-dark.svg" : "lock.svg"))
            iconSource: FoilNotesModel.keyAvailable ? lockIcon : "image://theme/icon-cover-new"
            onTriggered: {
                if (FoilNotesModel.keyAvailable) {
                    // Prevent secret text from disappearing before transition animation has completed
                    content.locking = true
                    FoilNotesModel.lock(false)
                    pageStack.navigateForward(PageStackAction.Immediate)
                    content.locking = false
                } else {
                    cover.newNote()
                }
            }
        }
    }
}

import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilnotes 1.0

import "harbour"

CoverBackground {
    id: cover

    property bool encryptedPageSelected

    signal newNote()

    readonly property bool _jailed: HarbourProcessState.jailedApp
    readonly property bool _darkOnLight: ('colorScheme' in Theme) && Theme.colorScheme === 1

    Rectangle {
        id: titleBackground

        anchors.fill: appTitle
        color: Theme.primaryColor
        opacity: FoilNotes.opacityFaint
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
        opacity: FoilNotes.opacityFaint
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

        readonly property real lineHeight: Math.floor(appTitle.implicitHeight + Theme.paddingSmall)
        readonly property int lineCount: Math.floor(height/lineHeight)
        property bool locking

        width: cover.width * 2
        anchors {
            top: leftEdge.bottom
            bottom: parent.bottom
        }

        x: ((encryptedPageSelected && FoilNotesModel.foilState === FoilNotesModel.FoilNotesReady) || _jailed) ? 0 : -cover.width
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
                y: (index + 1) * content.lineHeight
                width: content.width
                height: Theme.paddingSmall/4
                color: Theme.primaryColor
                opacity: FoilNotes.opacityLow
                smooth: true
                visible: !_jailed
            }
        }

        Image {
            readonly property real size: Math.floor(3*cover.width/5)
            x: (cover.width - width)/2
            y: (cover.height - height)/2 - parent.y
            height: size
            sourceSize.height: size
            source: _darkOnLight ? "images/fancy-lock-dark.svg" : "images/fancy-lock.svg"
            opacity: 0.1
        }

        HarbourHighlightIcon {
            anchors.verticalCenter: parent.verticalCenter
            sourceSize.width: cover.width
            highlightColor: Theme.secondaryColor
            visible: _jailed
            source: _jailed ? "images/jail.svg" : ""
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
            lineHeight: content.lineHeight
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
            lineHeight: content.lineHeight
            wrapMode: Text.Wrap
            elide: Text.ElideRight
            maximumLineCount: encryptedLabel.maximumLineCount
        }
    }

    OpacityRampEffect {
        // round it up to multiple of the line height, so that the opacity
        // doesn't start changing in the middle of the line
        readonly property real opacityRampHeight: Math.ceil(coverActionArea.height/cover.parent.scale/content.lineHeight) * content.lineHeight

        sourceItem: content
        direction: OpacityRamp.TopToBottom
        offset: 1 - opacityRampHeight/content.height
        slope: content.height/opacityRampHeight
    }

    CoverActionList {
        enabled: !_jailed
        CoverAction {
            readonly property url lockIcon: Qt.resolvedUrl("images/" + (_darkOnLight ? "lock-dark.svg" : "lock.svg"))
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

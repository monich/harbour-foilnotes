import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilnotes 1.0

import "harbour"

CoverBackground {
    id: cover

    property bool encryptedPageSelected
    readonly property int lineCount: Math.round((height - topOffset - coverActionHeight/parent.scale)/label.lineHeight)
    readonly property int topOffset: leftEdge.y + leftEdge.height
    readonly property int coverActionHeight: Theme.itemSizeSmall
    readonly property string coverText: (encryptedPageSelected &&
        FoilNotesModel.foilState === FoilNotesModel.FoilNotesReady) ?
        FoilNotesModel.text : FoilNotesPlaintextModel.text

    signal newNote()

    Image {
        visible: opacity > 0
        source: HarbourTheme.darkOnLight ? "images/fancy-lock-dark.svg" : "images/fancy-lock.svg"
        height: size
        sourceSize.height: size
        anchors.centerIn: parent
        opacity: (FoilNotesModel.keyAvailable && encryptedPageSelected) ? 0.1 : 0
        readonly property real size: Math.floor(3*cover.width/5)
    }

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

    Repeater {
        model: lineCount
        delegate: Rectangle {
            y: topOffset + (index + 1) * label.lineHeight
            width: parent.width
            height: Theme.paddingSmall/4
            color: Theme.primaryColor
            opacity: HarbourTheme.opacityLow
        }
    }

    Label {
        id: label
        x: Theme.paddingSmall/2
        y: topOffset + Theme.paddingSmall/2
        text: coverText
        opacity: 0.6
        font.italic: true
        width: cover.width - Theme.paddingSmall
        horizontalAlignment: Text.AlignLeft
        lineHeightMode: Text.FixedHeight
        lineHeight: Math.floor(appTitle.implicitHeight + Theme.paddingSmall)
        wrapMode: Text.Wrap
        elide: Text.ElideRight
        maximumLineCount: cover.lineCount
    }

    CoverActionList {
        CoverAction {
            readonly property url lockIcon: Qt.resolvedUrl("images/" + (HarbourTheme.darkOnLight ? "lock-dark.svg" : "lock.svg"))
            iconSource: FoilNotesModel.keyAvailable ? lockIcon : "image://theme/icon-cover-new"
            onTriggered: {
                if (FoilNotesModel.keyAvailable) {
                    FoilNotesModel.lock(false)
                    pageStack.navigateForward(PageStackAction.Immediate)
                } else {
                    cover.newNote()
                }
            }
        }
    }
}

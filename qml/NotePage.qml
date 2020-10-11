import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilnotes 1.0

import "harbour"

Page {
    id: page

    property int pagenr: 1
    property color color: FoilNotesSettings.availableColors[0]
    property alias body: textArea.text
    property alias dirty: shortSaveTimer.running
    property alias actionMenuText: actionMenuItem.text
    property bool secret

    readonly property real screenHeight: isPortrait ? Screen.height : Screen.width
    readonly property bool canUndo: "_editor" in textArea && textArea._editor.canUndo
    readonly property bool canRedo: "_editor" in textArea && textArea._editor.canRedo

    signal saveBody(var body)
    signal deleteNote()
    signal performAction()

    onStatusChanged: {
        if (status === PageStatus.Inactive) {
            if (dirty) {
                saveNote()
            }
        } else if (status === PageStatus.Active) {
            if (!body.length) {
                textArea.focus = true
            }
        }
    }

    function pickColor() {
        pageStack.push("Sailfish.Silica.ColorPickerPage", {
            colors: FoilNotesSettings.availableColors
        }).colorClicked.connect(function(color) {
            page.color = color
            pageStack.pop()
        })
    }

    function saveNote() {
        longSaveTimer.stop()
        shortSaveTimer.stop()
        saveBody(body)
    }

    HarbourQrCodeGenerator {
        id: generator

        text: page.body
    }

    SilicaFlickable {
        id: noteview

        anchors.fill: parent
        contentHeight: column.y + column.height

        PullDownMenu {
            MenuItem {
                id: actionMenuItem

                enabled: text.length > 0
                visible: enabled
                onClicked: page.performAction()
            }
            MenuItem {
                //: Show QR code for the current note
                //% "Show QR code"
                text: qsTrId("foilnotes-menu-show_qrcode")
                visible: generator.qrcode !== ""
                onClicked: pageStack.push("QrCodePage.qml", {
                    qrcode: generator.qrcode,
                    allowedOrientations: page.allowedOrientations
                })
            }
            MenuItem {
                //: Select note color
                //% "Select color"
                text: qsTrId("foilnotes-menu-select_color")
                onClicked: page.pickColor()
            }
            MenuItem {
                //: Delete this note from note page
                //% "Delete note"
                text: qsTrId("foilnotes-menu-delete_note")
                onClicked: page.deleteNote()
            }
        }

        Column {
            id: column
            width: page.width

            Item {
                id: headerItem
                width: parent.width
                height: Theme.itemSizeLarge

                Item {
                    id: colorItem

                    height: Theme.itemSizeSmall
                    width: Theme.itemSizeSmall
                    anchors {
                        right: parent.right
                        rightMargin: Theme.horizontalPageMargin
                        verticalCenter: parent.verticalCenter
                    }

                    // Pick the most different standard color out od these two:
                    readonly property color pagenrColor1: Theme.primaryColor
                    readonly property color pagenrColor2: Theme.highlightBackgroundColor
                    readonly property real pagenrColorDiff1: HarbourTheme.colorDifference(page.color, pagenrColor1)
                    readonly property real pagenrColorDiff2: HarbourTheme.colorDifference(page.color, pagenrColor2)
                    readonly property color pagenrColor: (pagenrColorDiff2 > pagenrColorDiff2) ? pagenrColor2 : pagenrColor1

                    Loader {
                        active: page.secret
                        anchors.fill: parent
                        sourceComponent: Item {
                            HarbourHighlightIcon {
                                source: "images/lock.svg"
                                highlightColor: page.color
                                sourceSize: Qt.size(colorItem.width, colorItem.height)
                                anchors.fill: parent
                            }

                            HarbourFitLabel {
                                text: page.pagenr
                                color: colorItem.pagenrColor
                                font.bold: true
                                maxFontSize: Theme.fontSizeLarge
                                maxWidth: Math.round(parent.width * 12 / 16) - Theme.paddingSmall
                                maxHeight: Math.round(parent.height * 9 / 16) - Theme.paddingSmall
                                anchors {
                                    centerIn: parent
                                    verticalCenterOffset: Math.round(parent.height * 3 / 16)
                                }
                            }
                        }
                    }

                    Loader {
                        active: !page.secret
                        height: Theme.itemSizeExtraSmall
                        width: Theme.itemSizeExtraSmall
                        anchors.centerIn: parent
                        sourceComponent: Item {
                            Rectangle {
                                color: page.color
                                radius: Theme.paddingSmall/2
                                anchors.fill: parent
                            }

                            HarbourFitLabel {
                                text: page.pagenr
                                color: colorItem.pagenrColor
                                font.bold: true
                                maxFontSize: Theme.fontSizeLarge
                                maxWidth: parent.width - Theme.paddingSmall
                                maxHeight: parent.height - Theme.paddingSmall
                                anchors.centerIn: parent
                            }
                        }
                    }

                    MouseArea {
                        anchors {
                            fill: parent
                            margins: -Theme.paddingMedium
                        }
                        onClicked: page.pickColor()
                    }
                }
            }

            TextArea {
                id: textArea
                font {
                    family: Theme.fontFamily
                    pixelSize: Theme.fontSizeMedium
                }
                width: parent.width
                height: Math.max(noteview.height - headerItem.height, implicitHeight)
                //: Placeholder text for new notes. At this point there is nothing else on the screen.
                //% "Write a note..."
                placeholderText: qsTrId("foilnotes-placeholder-empty_note")
                background: null

                onTextChanged: {
                    longSaveTimer.start()
                    shortSaveTimer.restart()
                }

                Timer {
                    // Save the text at least every 10 seconds
                    id: longSaveTimer
                    interval: 10000
                    onTriggered: page.saveNote()
                }

                Timer {
                    // or after 2 seconds of not typing anything
                    id: shortSaveTimer
                    interval: 2000
                    onTriggered: page.saveNote()
                }

                Connections {
                    target: Qt.application
                    onActiveChanged: {
                        if (page.dirty) {
                            page.saveNote()
                        }
                    }
                }
            }
        }

        VerticalScrollDecorator {}
    }

    InteractionHintLabel {
        id: selectionHint

        anchors.fill: page
        visible: opacity > 0
        opacity: item ? 1.0 : 0.0
        property Item item: null
        Behavior on opacity { FadeAnimation { duration: 1000 } }
    }

    Loader {
        opacity: page.canUndo ? 1 : 0
        active: opacity > 0
        anchors {
            top: parent.top
            topMargin: page.screenHeight - height - Theme.paddingLarge
            left: parent.left
            leftMargin: Theme.horizontalPageMargin
        }
        sourceComponent: Component {
            HarbourHintIconButton {
                id: undoButton

                //: Hint text
                //% "Undo"
                hint: qsTrId("foilnotes-hint-undo")
                icon.source: "images/undo.svg"
                onClicked: {
                    cancelHint()
                    textArea._editor.undo()
                }
                onShowHint: {
                    selectionHint.text = hint
                    selectionHint.item = undoButton
                }
                onHideHint: cancelHint()
                function cancelHint() {
                    if (selectionHint.item === undoButton) {
                        selectionHint.item = null
                    }
                }
            }
        }
        Behavior on opacity { FadeAnimation { } }
    }

    Loader {
        opacity: page.canRedo ? 1 : 0
        active: opacity > 0
        anchors {
            top: parent.top
            topMargin: page.screenHeight - height - Theme.paddingLarge
            right: parent.right
            rightMargin: Theme.horizontalPageMargin
        }
        sourceComponent: Component {
            HarbourHintIconButton {
                id: redoButton

                //: Hint text
                //% "Redo"
                hint: qsTrId("foilnotes-hint-redo")
                icon.source: "images/redo.svg"
                onClicked: {
                    cancelHint()
                    textArea._editor.redo()
                }
                onShowHint: {
                    selectionHint.text = hint
                    selectionHint.item = redoButton
                }
                onHideHint: cancelHint()
                function cancelHint() {
                    if (selectionHint.item === redoButton) {
                        selectionHint.item = null
                    }
                }
            }
        }
        Behavior on opacity { FadeAnimation { } }
    }
}

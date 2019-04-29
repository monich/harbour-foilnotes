import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilnotes 1.0

Page {
    id: page

    property int pagenr: 1
    property color color: FoilNotesSettings.availableColors[0]
    property alias body: textArea.text
    property alias dirty: shortSaveTimer.running
    property alias actionMenuText: actionMenuItem.text

    readonly property string imageProvider: HarbourTheme.darkOnLight ? ImageProviderDarkOnLight : ImageProviderDefault
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

    QrCodeGenerator {
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

                ColorItem {
                    id: colorItem
                    color: page.color
                    number: page.pagenr
                    onClicked: page.pickColor()
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
            IconButton {
                id: undoButton

                icon {
                    source: "image://" + imageProvider + "/" + Qt.resolvedUrl("images/undo.svg")
                    sourceSize: Qt.size(Theme.itemSizeSmall, Theme.itemSizeSmall)
                }
                onClicked: {
                    cancelHint()
                    textArea._editor.undo()
                }
                onReleased: cancelHint()
                onCanceled: cancelHint()
                onPressAndHold: {
                    //: Hint text
                    //% "Undo"
                    selectionHint.text = qsTrId("foilnotes-hint-undo")
                    selectionHint.item = undoButton
                }
                function cancelHint() {
                    if (selectionHint.item === undoButton) {
                        selectionHint.item = null
                    }
                }
            }
        }
        Behavior on opacity { FadeAnimator {} }
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
            IconButton {
                id: redoButton

                icon {
                    source: "image://" + imageProvider + "/" + Qt.resolvedUrl("images/redo.svg")
                    sourceSize: Qt.size(Theme.itemSizeSmall, Theme.itemSizeSmall)
                }
                onClicked: {
                    cancelHint()
                    textArea._editor.redo()
                }
                onReleased: cancelHint()
                onCanceled: cancelHint()
                onPressAndHold: {
                    //: Hint text
                    //% "Redo"
                    selectionHint.text = qsTrId("foilnotes-hint-redo")
                    selectionHint.item = redoButton
                }
                function cancelHint() {
                    if (selectionHint.item === redoButton) {
                        selectionHint.item = null
                    }
                }
            }
        }
        Behavior on opacity { FadeAnimator {} }
    }
}

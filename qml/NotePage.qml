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

    readonly property real screenHeight: isLandscape ? Screen.width : Screen.height
    readonly property bool canUndo: "_editor" in textArea && textArea._editor.canUndo
    readonly property bool canRedo: "_editor" in textArea && textArea._editor.canRedo

    property var colorEditorModel

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

    // Otherwise width is changing with a delay, causing visible layout changes
    onIsLandscapeChanged: width = isLandscape ? Screen.height : Screen.width

    Component {
        id: colorEditorModelComponent

        HarbourColorEditorModel {
            onColorsChanged: FoilNotesSettings.availableColors = colors
        }
    }

    function pickColor() {
        if (!colorEditorModel) {
            colorEditorModel = colorEditorModelComponent.createObject(page, {
                colors: FoilNotesSettings.availableColors
            })
        }
        var dialog = pageStack.push(Qt.resolvedUrl("harbour/HarbourColorPickerDialog.qml"), {
            allowedOrientations: page.allowedOrientations,
            acceptDestinationAction: PageStackAction.Replace,
            colorModel: colorEditorModel,
            color: page.color,
            //: Pulley menu item
            //% "Reset colors"
            resetColorsMenuText: qsTrId("color_picker-menu-reset_colors"),
            //: Dialog title label
            //% "Select color"
            acceptText: qsTrId("color_picker-action-select_color"),
            //: Dialog title label
            //% "Add color"
            addColorAcceptText: qsTrId("color_picker-action-add_color"),
            //: Hue slider label
            //% "Color"
            addColorHueSliderText: qsTrId("color_picker-slider-hue"),
            //: Brightness slider label
            //% "Brightness"
            addColorBrightnessSliderText: qsTrId("color_picker-slider-brightness"),
            //: Text field description
            //% "Hex notation"
            addColorHexNotationText: qsTrId("color_picker-text-hex_notation")
        })
        dialog.resetColors.connect(function() {
            colorEditorModel.colors = FoilNotesSettings.defaultColors
        })
        dialog.accepted.connect(function() {
            page.color = dialog.color
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
                id: nfcShareMenuItem

                //: Share current note via NFC
                //% "Share via NFC"
                text: qsTrId("foilnotes-menu-nfc_share")
                visible: NfcSystem.present && NfcSystem.enabled
                onClicked: {
                    pageStack.push(Qt.resolvedUrl("NfcSharePage.qml"), {
                        allowedOrientations: page.allowedOrientations,
                        noteColor: page.color,
                        noteText: page.body
                    })
                }
            }
            MenuItem {
                id: qrCodeMenuItem

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
                // Don't show more than 4 menu items in landscape
                visible: isPortrait || !actionMenuItem.visible || !nfcShareMenuItem.visible || !qrCodeMenuItem.visible
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
                height: isLandscape ? Theme.itemSizeSmall : Theme.itemSizeLarge
                clip: true

                Rectangle {
                    id: headerBackground

                    anchors {
                        top: parent.top
                        bottom: edge.top
                    }
                    width: parent.width
                    color: page.color
                    opacity: FoilNotes.opacityFaint
                }

                HarbourHighlightIcon {
                    id: edge

                    anchors.bottom: parent.bottom
                    source: "images/edge.svg"
                    sourceSize.height: Theme.paddingMedium
                    highlightColor: headerBackground.color
                    opacity: headerBackground.opacity
                    smooth: true
                }

                Repeater {
                    model: Math.ceil(parent.width/edge.width) - 1
                    delegate: Component {
                        HarbourHighlightIcon {
                            x: (index + 1) * width
                            y: edge.y
                            source: "images/edge.svg"
                            sourceSize.height: edge.height
                            highlightColor: edge.highlightColor
                            opacity: edge.opacity
                            smooth: true
                        }
                    }
                }

                Item {
                    id: colorItem

                    width: height
                    height: parent.height - (isLandscape ? (3 * Theme.paddingMedium) : (2 * Theme.paddingLarge))
                    anchors {
                        right: parent.right
                        rightMargin: Theme.horizontalPageMargin
                        verticalCenter: parent.verticalCenter
                    }

                    // Pick the most different standard color out of these two:
                    readonly property color pagenrColor1: Theme.primaryColor
                    readonly property color pagenrColor2: Theme.highlightBackgroundColor
                    readonly property real pagenrColorDiff1: FoilNotes.colorDifference(page.color, pagenrColor1)
                    readonly property real pagenrColorDiff2: FoilNotes.colorDifference(page.color, pagenrColor2)
                    readonly property color pagenrColor: (pagenrColorDiff2 > pagenrColorDiff2) ? pagenrColor2 : pagenrColor1

                    Loader {
                        active: page.secret
                        anchors.fill: parent
                        sourceComponent: Item {
                            HarbourHighlightIcon {
                                source: "images/lock-header.svg"
                                highlightColor: page.color
                                sourceSize: Qt.size(colorItem.width, colorItem.height)
                                anchors.fill: parent
                            }

                            HarbourFitLabel {
                                text: page.pagenr
                                color: colorItem.pagenrColor
                                font.bold: true
                                maxFontSize: Theme.fontSizeLarge
                                maxWidth: Math.round(parent.width * 24 / 29) - 2 * Theme.paddingSmall
                                maxHeight: Math.round(parent.height * 17 / 29)
                                anchors {
                                    centerIn: parent
                                    verticalCenterOffset: Math.round(parent.height * 6 / 29)
                                }
                            }
                        }
                    }

                    Loader {
                        active: !page.secret
                        anchors.fill: parent
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

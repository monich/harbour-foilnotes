import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilnotes 1.0

Page {
    id: page

    property int pagenr: 1
    property color color: FoilNotesSettings.availableColors[0]
    property alias body: textArea.text
    property alias dirty: shortSaveTimer.running

    signal saveBody(var body)
    signal deleteNote()

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

    SilicaFlickable {
        id: noteview

        anchors.fill: parent
        contentHeight: column.y + column.height

        PullDownMenu {
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
}

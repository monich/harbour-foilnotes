import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilnotes 1.0

Page {
    id: page

    property alias notesModel: organizeModel.sourceModel
    property string title
    property bool secret

    readonly property int columnCount: isPortrait ? appPortraitColumnCount : appLandscapeColumnCount
    readonly property int cellSize: isPortrait ? appPortraitCellSize : appLandscapeCellSize

    SilicaFlickable {
        id: flickable

        anchors.fill: parent

        SilicaGridView {
            id: grid

            cellHeight: page.cellSize
            cellWidth: page.cellSize
            anchors.fill: parent
            interactive: !dragItem
            clip: true
            model: HarbourOrganizeListModel {
                id: organizeModel
            }

            property Item dragItem
            property int pageHeaderHeight
            readonly property int cellsPerRow: Math.floor(width/cellWidth)

            function dragDone() {
                var prevContentY = contentY
                organizeModel.dragIndex = -1
                contentY = prevContentY
            }

            header: PageHeader {
                id: pageHeader

                title: page.title
                //: Page description
                //% "Press and drag to reorder"
                description: qsTrId("foilnotes-organize_page-description")
                Component.onCompleted: grid.pageHeaderHeight = height
                onHeightChanged: grid.pageHeaderHeight = height
            }

            delegate: MouseArea {
                id: noteDelegate
                width: grid.cellWidth
                height: grid.cellHeight
                readonly property bool highlighted: (pressed && containsMouse)

                readonly property int index: model.index

                NoteItem {
                    id: noteItem

                    scale: noteDelegate.dragging ? 1.1 : 1
                    body: model.body
                    color: model.color
                    pageNumber: model.pagenr
                    width: grid.cellWidth
                    height: grid.cellHeight
                    highlighted: noteDelegate.highlighted || scale > 1
                    secret: page.secret

                    states: [
                        State {
                            name: "dragging"
                            when: noteDelegate.dragging
                            ParentChange {
                                target: noteItem
                                parent: grid
                                x: noteDelegate.mapToItem(grid, 0, 0).x
                                y: noteDelegate.mapToItem(grid, 0, 0).y
                            }
                        },
                        State {
                            when: !noteDelegate.dragging
                            ParentChange {
                                target: noteItem
                                parent: noteDelegate
                                x: 0
                                y: 0
                            }
                        }
                    ]

                    transitions: [
                        Transition {
                            from: "dragging"
                            SequentialAnimation {
                                id: transitionAnimation
                                ParentAnimation {
                                    target: noteItem
                                    SmoothedAnimation {
                                        properties: "x,y"
                                        duration: 150
                                    }
                                }
                                ScriptAction { script: grid.dragDone() }
                            }
                        }
                    ]

                    Behavior on scale {
                        NumberAnimation {
                            easing.type: Easing.InQuad
                            duration: 150
                        }
                    }

                    Connections {
                        target: noteDelegate.dragging ? noteItem : null
                        onXChanged: noteItem.updateDragPos()
                        onYChanged: noteItem.scrollAndUpdateDragPos()
                    }

                    Connections {
                        target: noteDelegate.dragging ? grid : null
                        onContentXChanged: noteItem.updateDragPos()
                        onContentYChanged: noteItem.updateDragPos()
                    }

                    function scrollAndUpdateDragPos() {
                        if (y < 0) {
                            if (grid.contentY > grid.originY) {
                                scrollAnimation.to = grid.originY
                                if (!scrollAnimation.running) {
                                    scrollAnimation.duration = Math.max(scrollAnimation.minDuration,
                                        Math.min(scrollAnimation.maxDuration,
                                        (grid.contentY - grid.originY)*1000/grid.height))
                                    scrollAnimation.start()
                                }
                            }
                        } else if ((y + height) > grid.height) {
                            var maxContentY = grid.contentHeight - grid.height - grid.pageHeaderHeight
                            if (grid.contentY < maxContentY) {
                                scrollAnimation.to = maxContentY
                                if (!scrollAnimation.running) {
                                    scrollAnimation.duration = Math.max(scrollAnimation.minDuration,
                                        Math.min(scrollAnimation.maxDuration,
                                        (maxContentY - grid.contentY)*1000/grid.height))
                                    scrollAnimation.start()
                                }
                            }
                        } else {
                            scrollAnimation.stop()
                        }
                        updateDragPos()
                    }

                    function updateDragPos() {
                        var row = Math.floor((grid.contentY + y + height/2)/grid.cellHeight)
                        organizeModel.dragPos = grid.cellsPerRow * row + Math.floor((grid.contentX + x + width/2)/grid.cellWidth)
                    }
                }

                onPressAndHold: grid.dragItem = noteDelegate

                readonly property real dragThreshold: page.cellSize/10
                readonly property bool dragging: grid.dragItem === noteDelegate

                onDraggingChanged: {
                    if (dragging) {
                        organizeModel.dragIndex = index
                    }
                }

                function stopDrag() {
                    if (grid.dragItem === noteDelegate) {
                        grid.dragItem = null
                    }
                }

                onReleased: stopDrag()
                onCanceled: stopDrag()

                drag.target: dragging ? noteItem : null
                drag.axis: Drag.XandYAxis
            }

            SmoothedAnimation {
                id: scrollAnimation
                target: grid
                duration: minDuration
                properties: "contentY"

                readonly property int minDuration: 250
                readonly property int maxDuration: 5000
            }

            addDisplaced: Transition { SmoothedAnimation { properties: "x,y"; duration: 150 } }
            moveDisplaced: Transition { SmoothedAnimation { properties: "x,y"; duration: 150 } }
            removeDisplaced: Transition { SmoothedAnimation { properties: "x,y"; duration: 150 } }

            VerticalScrollDecorator { }
        }

        ViewPlaceholder {
            enabled: !grid.count
            //: Placeholder text
            //% "You do not have any notes."
            text: qsTrId("foilnotes-plaintext_view-placeholder")
        }
    }
}

import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilnotes 1.0

import "harbour"

Dialog {
    id: thisPage

    acceptDestinationAction: PageStackAction.Pop
    forwardNavigation: false

    property color noteColor
    property string noteText

    Component.onCompleted: {
        shareClient.sharePlaintext(noteColor, noteText)
    }

    NfcMode {
        enableModes: NfcSystem.P2PTarget + NfcSystem.P2PInitiator
        disableModes: NfcSystem.ReaderWriter
        active: Qt.application.active && thisPage.status === PageStatus.Active
    }

    NfcPeer {
        id: peer
        path: NfcAdapter.peerPath
    }

    FoilNotesNfcShareClient {
        id: shareClient
        onDone: {
            backNavigation = false
            forwardNavigation = true
            accept()
        }
    }

    Item {
        id: iconArea

        anchors {
            top: parent.top
            bottom: parent.bottom
            bottomMargin: isLandscape ? 0 : Math.round(2 * thisPage.height / 3)
            left: parent.left
            right: parent.right
            rightMargin: isLandscape ? Math.round(2 * thisPage.width / 3) : 0
        }

        HarbourHighlightIcon {
            anchors.centerIn: parent
            sourceSize.height: Theme.itemSizeHuge
            highlightColor: !NfcSystem.enabled ? Theme.secondaryColor :
                peer.present ? Theme.highlightColor :
                Theme.primaryColor
            scale: peer.present ? 1.2 : 1
            source: "images/nfc.svg"

            Behavior on scale {
                SpringAnimation {
                    spring: 5
                    mass: 0.5
                    damping: 0.1
                    duration: 200
                }
            }
       }
    }

    Item {
        id: contentArea

        anchors {
            bottom: parent.bottom
            right: parent.right
        }

        InfoLabel {
            id: infoLabel

            // Something like ViewPlaceholder but without a pulley hint
            visible: opacity > 0
            verticalAlignment: Text.AlignVCenter
            anchors {
                top: parent.top
                bottom: parent.bottom
            }
            // NOTE: if NFC is completely unavailable we should
            // never get here in the first place
            text:(!NfcSystem.enabled || !NfcSystem.present) ?
                    //: Full screen info label
                    //% "NFC is off"
                    qsTrId("foilnotes-nfc_share-info-nfc_off") :
                shareClient.state === FoilNotesNfcShareClient.Idle ?
                    //: Full screen info label
                    //% "Touch another NFC capable device with Foil Notes running full screen."
                    qsTrId("foilnotes-nfc_share-info-ready") :
                    ""
            Behavior on opacity { FadeAnimation { duration: 150 } }
        }

        BusyIndicator {
            anchors.centerIn: parent
            size: BusyIndicatorSize.Large
            running: infoLabel.text === ""
        }
    }

    states: [
        State {
            name: "portrait"
            when: !isLandscape
            changes: [
                AnchorChanges {
                    target: contentArea
                    anchors {
                        left: thisPage.left
                        top: iconArea.bottom
                    }
                }
            ]
        },
        State {
            name: "landscape"
            when: isLandscape
            changes: [
                AnchorChanges {
                    target: contentArea
                    anchors {
                        left: iconArea.right
                        top: thisPage.top
                    }
                }
            ]
        }
    ]
}

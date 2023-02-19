import QtQuick 2.0
import Sailfish.Silica 1.0
import org.nemomobile.configuration 1.0

Page {
    property alias title: pageHeader.title

    // jolla-settings expects these properties:
    property var applicationName
    property var applicationIcon

    readonly property string _rootPath: "/apps/harbour-foilnotes/"

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: content.height

        Column {
            id: content
            width: parent.width

            PageHeader {
                id: pageHeader

                rightMargin: Theme.horizontalPageMargin + (appIcon.visible ? (height - appIcon.padding) : 0)
                title: applicationName ? applicationName : "Foil Notes"
                description: applicationName ?
                    //: Settings page header description (app version)
                    //% "Version %1"
                    qsTrId("foilnotes-settings_page-header-version").arg("1.1.1") : ""

                Image {
                    id: appIcon

                    readonly property int padding: Theme.paddingLarge
                    readonly property int size: pageHeader.height - 2 * padding
                    x: pageHeader.width - width - Theme.horizontalPageMargin
                    y: padding
                    width: size
                    height: size
                    sourceSize: Qt.size(size,size)
                    source: applicationIcon ? applicationIcon : ""
                    visible: appIcon.status === Image.Ready
                }
            }

            TextSwitch {
                //: Text switch label
                //% "Automatic locking"
                text: qsTrId("foilnotes-settings_page-autolock-text")
                //: Text switch description
                //% "Require to enter Foil password after unlocking the screen."
                description: qsTrId("foilnotes-settings_page-autolock-description")
                automaticCheck: false
                checked: autoLockConfig.value
                onClicked: autoLockConfig.value = !autoLockConfig.value

                ConfigurationValue {
                    id: autoLockConfig

                    key: _rootPath + "autoLock"
                    defaultValue: true
                }
            }
        }
    }
}

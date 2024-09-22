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
        contentHeight: content.height + content.padding

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
                    qsTrId("foilnotes-settings_page-header-version").arg("1.1.5") : ""

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

            SectionHeader {
                //: Section label
                //% "Appearance"
                text: qsTrId("foilnotes-settings_page-section-appearance")
            }

            FontSizeSlider {
                //: Slider label
                //% "Grid text size"
                label: qsTrId("foilnotes-settings_page-grid_font_size-label")
                configKey: _rootPath + "gridFontSize"
                defaultValue: "small"
            }

            FontSizeSlider {
                //: Slider label
                //% "Editor text size"
                label: qsTrId("foilnotes-settings_page-editor_font_size-label")
                configKey: _rootPath + "editorFontSize"
                defaultValue: "medium"
            }

            SectionHeader {
                //: Section label
                //% "Security"
                text: qsTrId("foilnotes-settings_page-section-security")
            }

            TextSwitch {
                //: Text switch label
                //% "Automatic locking"
                text: qsTrId("foilnotes-settings_page-autolock-text")
                //: Text switch description
                //% "Automatically lock the notes when the screen is locked and require to enter Foil password after unlocking the screen."
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

            Slider {
                readonly property int min: value /60
                readonly property int sec: value  % 60

                visible: opacity > 0
                opacity: autoLockConfig.value ? 1.0 : 0.0
                width: parent.width
                minimumValue: 0
                maximumValue: 300
                value: autoLockTimeConfig.value / 1000
                stepSize: 5
                //: Slider label
                //% "Locking delay"
                label: qsTrId("foilnotes-settings_page-autolock_delay-label")
                valueText: !value ?
                    //: Slider value (no delay)
                    //% "No delay"
                    qsTrId("foilnotes-settings_page-autolock_delay-value-no_delay") :
                    //: Slider value
                    //% "%1 sec"
                    !min ? qsTrId("foilnotes-settings_page-autolock_delay-value-sec",value).arg(sec) :
                    //: Slider value
                    //% "%1 min"
                    !sec ? qsTrId("foilnotes-settings_page-autolock_delay-value-min",value).arg(min) :
                    qsTrId("foilnotes-settings_page-autolock_delay-value-min",value).arg(min) + " " +
                    qsTrId("foilnotes-settings_page-autolock_delay-value-sec",value).arg(sec)
                onSliderValueChanged: autoLockTimeConfig.value = value * 1000
                Behavior on opacity { FadeAnimation { } }

                ConfigurationValue {
                    id: autoLockTimeConfig

                    key: _rootPath + "autoLockTime"
                    defaultValue: 15000
                }
            }
        }
    }
}

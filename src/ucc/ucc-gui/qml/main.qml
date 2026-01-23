// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Uniwill Control Center Contributors

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import UCC 1.0

ApplicationWindow {
    id: root
    width: 1024
    height: 768
    visible: true
    title: qsTr("Uniwill Control Center")

    // Initialize monitoring when app starts (dashboard is default tab)
    Component.onCompleted: {
        systemMonitor.monitoringActive = true;
    }

    menuBar: MenuBar {
        Menu {
            title: qsTr("&File")
            Action {
                text: qsTr("&Settings")
                shortcut: StandardKey.Preferences
            }
            MenuSeparator { }
            Action {
                text: qsTr("&Quit")
                shortcut: StandardKey.Quit
                onTriggered: Qt.quit()
            }
        }
        Menu {
            title: qsTr("&Profiles")
            Action {
                text: qsTr("&New Profile")
                onTriggered: console.log("Create new profile")
            }
            Action {
                text: qsTr("&Refresh")
                shortcut: StandardKey.Refresh
                onTriggered: profileManager.refresh()
            }
        }
        Menu {
            title: qsTr("&Help")
            Action {
                text: qsTr("&About")
                onTriggered: console.log("Show about dialog")
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // Sidebar
        Rectangle {
            Layout.preferredWidth: 200
            Layout.fillHeight: true
            color: "#2c2c2c"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 5

                Label {
                    text: qsTr("Navigation")
                    font.bold: true
                    color: "#ffffff"
                }

                Button {
                    Layout.fillWidth: true
                    text: qsTr("Dashboard")
                    onClicked: stackView.currentIndex = 0
                }

                Button {
                    Layout.fillWidth: true
                    text: qsTr("Profiles")
                    onClicked: stackView.currentIndex = 1
                }

                Button {
                    Layout.fillWidth: true
                    text: qsTr("Performance")
                    onClicked: stackView.currentIndex = 2
                }

                Button {
                    Layout.fillWidth: true
                    text: qsTr("Hardware")
                    onClicked: stackView.currentIndex = 3
                }

                Button {
                    Layout.fillWidth: true
                    text: qsTr("Fan Control")
                    onClicked: stackView.currentIndex = 4
                }

                Item {
                    Layout.fillHeight: true
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: "#555555"
                }

                Label {
                    text: profileManager.connected ? qsTr("Connected") : qsTr("Disconnected")
                    color: profileManager.connected ? "#4caf50" : "#f44336"
                    font.pixelSize: 12
                }
            }
        }

        // Main content area
        StackLayout {
            id: stackView
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: 0

            // Control monitoring based on current tab
            onCurrentIndexChanged: {
                // Only monitor when dashboard (index 0) is visible
                systemMonitor.monitoringActive = (currentIndex === 0);
            }

            // Dashboard page
            Page {
                padding: 20

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 20

                    Label {
                        text: qsTr("Dashboard")
                        font.pixelSize: 24
                        font.bold: true
                    }

                    GridLayout {
                        columns: 2
                        rowSpacing: 10
                        columnSpacing: 20

                        Label {
                            text: qsTr("Active Profile:")
                            font.bold: true
                        }
                        Label {
                            text: profileManager.activeProfile || qsTr("None")
                        }

                        Label {
                            text: qsTr("GPU Temperature:")
                            font.bold: true
                        }
                        Label {
                            text: systemMonitor.gpuTemp
                        }

                        Label {
                            text: qsTr("Fan Speed:")
                            font.bold: true
                        }
                        Label {
                            text: systemMonitor.fanSpeed
                        }

                        Label {
                            text: qsTr("Display Brightness:")
                            font.bold: true
                        }
                        Slider {
                            from: 0
                            to: 100
                            value: systemMonitor.displayBrightness
                            onMoved: systemMonitor.displayBrightness = value
                        }
                    }

                    GroupBox {
                        title: qsTr("Quick Controls")
                        Layout.fillWidth: true

                        RowLayout {
                            Switch {
                                text: qsTr("Webcam")
                                checked: systemMonitor.webcamEnabled
                                onToggled: systemMonitor.webcamEnabled = checked
                            }

                            Switch {
                                text: qsTr("Fn Lock")
                                checked: systemMonitor.fnLock
                                onToggled: systemMonitor.fnLock = checked
                            }
                        }
                    }

                    Item {
                        Layout.fillHeight: true
                    }
                }
            }

            // Profiles page
            Page {
                padding: 20

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 15

                    Label {
                        text: qsTr("Profiles")
                        font.pixelSize: 24
                        font.bold: true
                    }

                    GroupBox {
                        title: qsTr("Select Profile")
                        Layout.fillWidth: true
                        Layout.preferredHeight: 100

                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 10

                            Label {
                                text: qsTr("Active Profile:")
                                font.bold: true
                            }

                            ComboBox {
                                id: profileCombo
                                Layout.fillWidth: true
                                
                                model: profileManager.allProfiles
                                currentIndex: profileManager.activeProfileIndex

                                onCurrentIndexChanged: {
                                    if (currentIndex >= 0) {
                                        profileManager.setActiveProfileByIndex(currentIndex);
                                    }
                                }

                                delegate: ItemDelegate {
                                    width: profileCombo.width
                                    text: modelData
                                    highlighted: profileCombo.highlightedIndex === index
                                    padding: 8
                                }

                                contentItem: Text {
                                    text: profileCombo.displayText
                                    font: profileCombo.font
                                    color: profileCombo.enabled ? "#000000" : "#999999"
                                    verticalAlignment: Text.AlignVCenter
                                    elide: Text.ElideRight
                                    padding: 8
                                }

                                background: Rectangle {
                                    implicitWidth: 200
                                    implicitHeight: 40
                                    border.color: profileCombo.activeFocus ? "#1976d2" : "#cccccc"
                                    border.width: profileCombo.activeFocus ? 2 : 1
                                    radius: 3
                                    color: profileCombo.enabled ? "#ffffff" : "#f5f5f5"
                                }

                                popup: Popup {
                                    y: profileCombo.height + 3
                                    width: profileCombo.width
                                    implicitHeight: contentItem.implicitHeight

                                    contentItem: ListView {
                                        clip: true
                                        model: profileCombo.model
                                        currentIndex: profileCombo.highlightedIndex

                                        delegate: ItemDelegate {
                                            width: parent ? parent.width : 200
                                            text: modelData
                                            highlighted: ListView.isCurrentItem
                                            onClicked: {
                                                profileCombo.currentIndex = index;
                                                profileCombo.popup.close();
                                            }
                                            padding: 8
                                        }

                                        ScrollIndicator.vertical: ScrollIndicator { }
                                    }

                                    background: Rectangle {
                                        border.color: "#cccccc"
                                        border.width: 1
                                        radius: 3
                                    }
                                }
                            }

                            Label {
                                text: qsTr("Select a profile to activate it")
                                font.pixelSize: 12
                                color: "#666666"
                            }
                        }
                    }

                    Item {
                        Layout.fillHeight: true
                    }
                }
            }

            // Performance page placeholder
            Page {
                padding: 20
                Label {
                    text: qsTr("Performance Controls")
                    font.pixelSize: 24
                    font.bold: true
                }
            }

            // Hardware page placeholder
            Page {
                padding: 20
                Label {
                    text: qsTr("Hardware Information")
                    font.pixelSize: 24
                    font.bold: true
                }
            }

            // Fan Control Tab
            Page {
                padding: 20
                FanControlTab {
                    anchors.fill: parent
                }
            }
        }
    }
}

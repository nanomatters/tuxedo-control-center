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
                        title: qsTr("Default Profiles")
                        Layout.fillWidth: true

                        ListView {
                            width: parent.width
                            height: 150
                            model: profileManager.defaultProfiles
                            delegate: ItemDelegate {
                                width: ListView.view.width
                                text: modelData
                                highlighted: modelData === profileManager.activeProfile
                                onClicked: profileManager.setActiveProfile(modelData)
                            }
                        }
                    }

                    GroupBox {
                        title: qsTr("Custom Profiles")
                        Layout.fillWidth: true

                        ListView {
                            width: parent.width
                            height: 150
                            model: profileManager.customProfiles
                            delegate: ItemDelegate {
                                width: ListView.view.width
                                text: modelData
                                highlighted: modelData === profileManager.activeProfile
                                onClicked: profileManager.setActiveProfile(modelData)
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
        }
    }
}

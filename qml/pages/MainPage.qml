import QtQuick 2.0;
import Sailfish.Silica 1.0;
import "../components";

Page {
    id: page;
    allowedOrientations: (Orientation.Portrait | Orientation.Landscape);

    property string currentCategory : "";

    SilicaListView {
        id: view;
        currentIndex: -1;
        model: modelSubscriptions;
        header: Column {
            spacing: Theme.paddingSmall;
            anchors {
                left: (parent ? parent.left : undefined);
                right: (parent ? parent.right : undefined);
            }

            PageHeader {
                title: qsTr ("Feed'me");
            }
            Label {
                text: qsTr ("Read your daily news, easily and quickly.");
                color: Theme.secondaryHighlightColor;
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere;
                font.pixelSize: Theme.fontSizeExtraSmall;
                anchors {
                    left: parent.left;
                    right: parent.right;
                    margins: Theme.paddingLarge;
                }
            }
            BackgroundItem {
                id: itemAll;
                onClicked: {

                    pageStack.push (streamPage);
                }

                Label {
                    text: qsTr ("See all...");
                    textFormat: Text.PlainText;
                    truncationMode: TruncationMode.Fade;
                    color: Theme.primaryColor;
                    anchors {
                        left: parent.left;
                        right: parent.right;
                        margins: Theme.paddingLarge;
                        verticalCenter: parent.verticalCenter;
                    }
                }
            }
        }
        section {
            property: "category";
            delegate: BackgroundItem {
                id: itemCategory;
                onPressAndHold: {
                    currentCategory = (currentCategory !== section ? section : "");
                }
                onClicked: {

                    pageStack.push (streamPage);
                }
                ListView.onAdd: AddAnimation { target: itemCategory; }

                Label {
                    text: section;
                    textFormat: Text.PlainText;
                    truncationMode: TruncationMode.Fade;
                    color: (currentCategory === section ? Theme.highlightColor : Theme.primaryColor);
                    anchors {
                        left: parent.left;
                        right: parent.right;
                        margins: Theme.paddingLarge;
                        verticalCenter: parent.verticalCenter;
                    }
                }
            }
        }
        delegate: BackgroundItem {
            id: itemFeed;
            height: (visible ? implicitHeight : 0);
            visible: (model ['category'] === currentCategory);
            onClicked: {

                pageStack.push (streamPage);
            }
            ListView.onAdd: AddAnimation { target: itemFeed; }

            Label {
                text: model ['title'];
                textFormat: Text.PlainText;
                truncationMode: TruncationMode.Fade;
                color: Theme.secondaryColor;
                anchors {
                    left: parent.left;
                    right: bubble.left;
                    margins: Theme.paddingLarge;
                    verticalCenter: parent.verticalCenter;
                }
            }
            Bubble {
                id: bubble;
                value: model ['count'];
                anchors {
                    right: parent.right;
                    margins: Theme.paddingLarge;
                    verticalCenter: parent.verticalCenter;
                }
            }
        }
        anchors {
            top: parent.top;
            left: parent.left;
            right: parent.right;
            bottom: panelBottom.top;
        }

        PullDownMenu {
            MenuItem {
                text: qsTr ("Login / logout");
                anchors {
                    left: parent.left;
                    right: parent.right;
                }
                onClicked: {

                }
            }
            MenuItem {
                text: qsTr ("Add new feed...");
                anchors {
                    left: parent.left;
                    right: parent.right;
                }
                onClicked: {

                }
            }
            MenuItem {
                text: qsTr ("Go offline / online");
                anchors {
                    left: parent.left;
                    right: parent.right;
                }
                onClicked: {

                }
            }
            MenuItem {
                text: qsTr ("Refresh all");
                anchors {
                    left: parent.left;
                    right: parent.right;
                }
                onClicked: {

                }
            }
        }
        VerticalScrollDecorator {}
    }
    OpacityRampEffect {
        sourceItem: view;
        enabled: (!view.atYEnd);
        direction: OpacityRamp.TopToBottom;
        offset: 0.35;
        slope: 1.25;
        width: view.width;
        height: view.height;
        anchors.fill: null;
    }
    Button {
        id: panelBottom;
        text: qsTr ("Back to top");
        anchors {
            left: parent.left;
            right: parent.right;
            bottom: parent.bottom;
            bottomMargin: (view.atYBeginning ? -height : 0);
            margins: 0;
        }
        onClicked: { view.scrollToTop (); }
    }
}



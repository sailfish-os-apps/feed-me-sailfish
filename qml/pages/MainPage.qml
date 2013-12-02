import QtQuick 2.0;
import Sailfish.Silica 1.0;
import "../components";

Page {
    id: page;
    allowedOrientations: (Orientation.Portrait | Orientation.Landscape);

    Component.onCompleted: { Database.loadSubscriptions (); }

    property string currentCategory : "";

    Connections {
        target: Database;
        onSubscriptionsListChanged: {
            modelSubcriptions.clear ();
            modelSubcriptions.append (Database.subscriptionsList);
        }
    }
    SilicaListView {
        id: view;
        currentIndex: -1;
        model: ListModel {
            id: modelSubcriptions;
        }
        header: Column {
            spacing: Theme.paddingSmall;
            anchors {
                left: (parent ? parent.left : undefined);
                right: (parent ? parent.right : undefined);
            }

            PageHeader {
                title: qsTr ("Feed'me");

                BusyIndicator {
                    running: true;
                    size: BusyIndicatorSize.Medium;
                    anchors {
                        left: parent.left;
                        margins: Theme.paddingLarge;
                        verticalCenter: parent.verticalCenter;
                    }
                }
            }
            Label {
                text: qsTr ("Read your daily news, easily and quickly.");
                color: Theme.secondaryHighlightColor;
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere;
                font.family: Theme.fontFamilyHeading;
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
                    text: qsTr ("All items...");
                    textFormat: Text.PlainText;
                    truncationMode: TruncationMode.Fade;
                    font.family: Theme.fontFamilyHeading;
                    color: Theme.secondaryColor;
                    anchors {
                        left: parent.left;
                        right: parent.right;
                        margins: Theme.paddingLarge;
                        verticalCenter: parent.verticalCenter;
                    }
                }
            }
            BackgroundItem {
                id: itemStarred;
                onClicked: {

                    pageStack.push (streamPage);
                }

                Label {
                    text: qsTr ("Marked items...");
                    textFormat: Text.PlainText;
                    truncationMode: TruncationMode.Fade;
                    color: Theme.secondaryColor;
                    font.family: Theme.fontFamilyHeading;
                    anchors {
                        left: parent.left;
                        right: parent.right;
                        margins: Theme.paddingLarge;
                        verticalCenter: parent.verticalCenter;
                    }
                }
            }
        }
        footer: Button {
            text: qsTr ("Back to top");
            visible: (!view.atYBeginning && view.visibleArea.heightRatio < 1.0 && !pulley.active);
            anchors {
                left: parent.left;
                right: parent.right;
                margins: 0;
            }
            onClicked: { view.scrollToTop (); }
        }
        section {
            property: "categoryId";
            delegate: BackgroundItem {
                id: itemCategory;
                onPressAndHold: {
                    currentCategory = (currentCategory !== section ? section : "");
                }
                onClicked: {
                    //currentStream = categoryId;
                    pageStack.push (streamPage);
                }
                ListView.onAdd: AddAnimation { target: itemCategory; }

                property string categoryId : section;

                Label {
                    text: Database.categoryInfo [itemCategory.categoryId] ['label'];
                    textFormat: Text.PlainText;
                    truncationMode: TruncationMode.Fade;
                    font.family: Theme.fontFamilyHeading;
                    color: (currentCategory === section ? Theme.highlightColor : Theme.primaryColor);
                    anchors {
                        left: parent.left;
                        right: bubble.left;
                        leftMargin: Theme.paddingLarge;
                        rightMargin: Theme.paddingSmall;
                        verticalCenter: parent.verticalCenter;
                    }
                }
                Bubble {
                    id: bubble;
                    value: Database.categoryInfo [itemCategory.categoryId] ['counter'];
                    anchors {
                        right: parent.right;
                        margins: Theme.paddingSmall;
                        verticalCenter: parent.verticalCenter;
                    }
                }
            }
        }
        delegate: BackgroundItem {
            id: itemFeed;
            height: (visible ? implicitHeight : 0);
            visible: (model ['categoryId'] === currentCategory);
            onClicked: {
                //currentStream = feedId;
                pageStack.push (streamPage);
            }
            ListView.onAdd: AddAnimation { target: itemFeed; }

            property string feedId : model ['feedId'];

            Label {
                text: Database.feedInfo [itemFeed.feedId] ['title'];
                textFormat: Text.PlainText;
                truncationMode: TruncationMode.Fade;
                font.pixelSize: Theme.fontSizeSmall;
                font.family: Theme.fontFamilyHeading;
                color: Theme.secondaryColor;
                anchors {
                    left: parent.left;
                    right: bubble.left;
                    leftMargin: (Theme.paddingLarge * 2);
                    rightMargin: Theme.paddingSmall;
                    verticalCenter: parent.verticalCenter;
                }
            }
            Bubble {
                id: bubble;
                value: Database.feedInfo [itemFeed.feedId] ['counter'];
                anchors {
                    right: parent.right;
                    margins: Theme.paddingSmall;
                    verticalCenter: parent.verticalCenter;
                }
            }
        }
        anchors {
            top: parent.top;
            left: parent.left;
            right: parent.right;
            bottom: parent.bottom;
        }

        PullDownMenu {
            id: pulley;

            MenuItem {
                text: qsTr ("Login / logout");
                font.family: Theme.fontFamilyHeading;
                anchors {
                    left: parent.left;
                    right: parent.right;
                }
                onClicked: {

                }
            }
            MenuItem {
                text: qsTr ("Add new feed...");
                font.family: Theme.fontFamilyHeading;
                anchors {
                    left: parent.left;
                    right: parent.right;
                }
                onClicked: {

                }
            }
            MenuItem {
                text: qsTr ("Go offline / online");
                font.family: Theme.fontFamilyHeading;
                anchors {
                    left: parent.left;
                    right: parent.right;
                }
                onClicked: {

                }
            }
            MenuItem {
                text: qsTr ("Refresh all");
                font.family: Theme.fontFamilyHeading;
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
        offset: 0.65;
        slope: 1.35;
        width: view.width;
        height: view.height;
        anchors.fill: null;
    }
}



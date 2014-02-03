import QtQuick 2.0;
import Sailfish.Silica 1.0;
import harbour.feedme.myQtCoreImports 5.1;
import "../components";

Page {
    id: page;
    allowedOrientations: (Orientation.Portrait | Orientation.Landscape);

    property string currentCategory : "";

    RemorsePopup { id: remorseLogout; }
    RemorseItem { id: remorseUnsubscribe; }
    Connections {
        target: Feedly;
        onSubscriptionsListChanged: {
            view.positionViewAtBeginning ();
            modelSubcriptions.clear ();
            modelSubcriptions.append (Feedly.subscriptionsList);
        }
    }
    SilicaListView {
        id: view;
        currentIndex: -1;
        model: ListModel {
            id: modelSubcriptions;

            Component.onCompleted: { append (Feedly.subscriptionsList); }
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
                    running: Feedly.isPolling;
                    visible: running;
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
                    Feedly.currentStreamId = categoryAll.streamId;
                    pageStack.push (streamPage);
                }

                property CategoryInfo categoryAll : Feedly.getCategoryInfo (Feedly.getStreamIdAll ());

                Label {
                    text: itemAll.categoryAll.label;
                    textFormat: Text.PlainText;
                    truncationMode: TruncationMode.Fade;
                    font.family: Theme.fontFamilyHeading;
                    color: Theme.secondaryColor;
                    anchors {
                        left: parent.left;
                        right: bubbleAll.left;
                        margins: Theme.paddingLarge;
                        verticalCenter: parent.verticalCenter;
                    }
                }
                Bubble {
                    id: bubbleAll;
                    value: itemAll.categoryAll.counter;
                    anchors {
                        right: parent.right;
                        margins: Theme.paddingSmall;
                        verticalCenter: parent.verticalCenter;
                    }
                }
            }
            BackgroundItem {
                id: itemStarred;
                onClicked: {
                    Feedly.currentStreamId = categoryStarred.streamId;
                    pageStack.push (streamPage);
                }

                property CategoryInfo categoryStarred : Feedly.getCategoryInfo (Feedly.getStreamIdMarked ());

                Label {
                    text: itemStarred.categoryStarred.label;
                    textFormat: Text.PlainText;
                    truncationMode: TruncationMode.Fade;
                    color: Theme.secondaryColor;
                    font.family: Theme.fontFamilyHeading;
                    anchors {
                        left: parent.left;
                        right: bubbleStarred.left;
                        margins: Theme.paddingLarge;
                        verticalCenter: parent.verticalCenter;
                    }
                }
                Bubble {
                    id: bubbleStarred;
                    value: itemStarred.categoryStarred.counter;
                    anchors {
                        right: parent.right;
                        margins: Theme.paddingSmall;
                        verticalCenter: parent.verticalCenter;
                    }
                }
            }
        }
        footer: Item {
            height: btnBackToTop.height;
            anchors {
                left: parent.left;
                right: parent.right;
                margins: 0;
            }
        }
        section {
            property: "categoryId";
            delegate: BackgroundItem {
                id: itemCategory;
                onClicked: {
                    Feedly.currentStreamId = categoryInfo.streamId;
                    pageStack.push (streamPage);
                }
                ListView.onAdd: AddAnimation { target: itemCategory; }

                property bool         isCurrentSection : (currentCategory === section);
                property CategoryInfo categoryInfo     : Feedly.getCategoryInfo (section);

                MouseArea {
                    id: clicker;
                    width: height;
                    anchors {
                        top: parent.top;
                        left: parent.left;
                        bottom: parent.bottom;
                    }
                    onClicked: { currentCategory = (!itemCategory.isCurrentSection ? section : ""); }

                    Image {
                        source: "../img/arrow-right.png";
                        rotation: (itemCategory.isCurrentSection ? +90 : 0);
                        anchors.centerIn: parent;

                        Behavior on rotation { NumberAnimation { duration: 180; } }
                    }
                }
                Label {
                    text: itemCategory.categoryInfo.label;
                    textFormat: Text.PlainText;
                    truncationMode: TruncationMode.Fade;
                    font.family: Theme.fontFamilyHeading;
                    color: (itemCategory.isCurrentSection ? Theme.highlightColor : Theme.primaryColor);
                    anchors {
                        left: clicker.right;
                        right: bubble.left;
                        leftMargin: Theme.paddingSmall;
                        rightMargin: Theme.paddingSmall;
                        verticalCenter: parent.verticalCenter;
                    }
                }
                Bubble {
                    id: bubble;
                    value: itemCategory.categoryInfo.counter;
                    anchors {
                        right: parent.right;
                        margins: Theme.paddingSmall;
                        verticalCenter: parent.verticalCenter;
                    }
                }
            }
        }
        delegate: ListItem {
            id: itemFeed;
            height: (visible ? (menuOpen ? _menuItem.height + contentItem.height : contentItem.height) : 0);
            visible: (feedInfo.categoryId === currentCategory);
            menu: Component {
                ContextMenu {
                    MenuItem {
                        text: qsTr ("Unsubscribe this feed");
                        onClicked: {
                            remorseUnsubscribe.execute (itemFeed,
                                                        qsTr ("Removing feed"),
                                                        function () {
                                                            // TODO : remove feed and unsubscribe
                                                        },
                                                        5000);
                        }
                    }
                }
            }
            onClicked: {
                Feedly.currentStreamId = feedInfo.streamId;
                pageStack.push (streamPage);
            }
            ListView.onAdd: AddAnimation { target: itemFeed; }

            property FeedInfo feedInfo : Feedly.getFeedInfo (model ['feedId']);

            Label {
                text: itemFeed.feedInfo.title;
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
                value: itemFeed.feedInfo.counter;
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
                text: qsTr ("Logout account");
                font.family: Theme.fontFamilyHeading;
                enabled: !Feedly.isOffline;
                anchors {
                    left: parent.left;
                    right: parent.right;
                }
                onClicked: {
                    remorseLogout.execute ("Login out",
                                     function () {
                                         // TODO : logout, delete cache, show login page
                                     },
                                     5000);
                }
            }
            MenuItem {
                text: qsTr ("Add new feed...");
                font.family: Theme.fontFamilyHeading;
                enabled: !Feedly.isOffline;
                anchors {
                    left: parent.left;
                    right: parent.right;
                }
                onClicked: {
                    // TODO : add dialog to search / add feed
                }
            }
            MenuItem {
                text: qsTr ("Offline mode : <b>%1</b>").arg (Feedly.isOffline ? qsTr ("ON") : qsTr ("OFF"));
                textFormat: Text.StyledText;
                font.family: Theme.fontFamilyHeading;
                anchors {
                    left: parent.left;
                    right: parent.right;
                }
                onClicked: {
                    Feedly.isOffline = !Feedly.isOffline;
                }
            }
            MenuItem {
                text: qsTr ("Refresh all");
                font.family: Theme.fontFamilyHeading;
                enabled: !Feedly.isOffline;
                anchors {
                    left: parent.left;
                    right: parent.right;
                }
                onClicked: {
                    Feedly.refreshAll ();
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
    Button {
        id: btnBackToTop;
        text: qsTr ("Back to top");
        visible: (!view.atYBeginning && view.visibleArea.heightRatio < 1.0 && !pulley.active);
        anchors {
            left: parent.left;
            right: parent.right;
            bottom: parent.bottom;
            margins: 0;
        }
        onClicked: { view.scrollToTop (); }
    }
    Loader {
        active: !Feedly.isLogged;
        asynchronous: true;
        sourceComponent: SilicaWebView {
            id: webView;
            url: Feedly.getOAuthPageUrl ();
        }
        anchors.fill: parent;
    }
}



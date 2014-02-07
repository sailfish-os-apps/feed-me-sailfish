import QtQuick 2.0;
import Sailfish.Silica 1.0;
import harbour.feedme.myQtCoreImports 5.1;
import "../components";

Page {
    id: page;
    allowedOrientations: (Orientation.Portrait | Orientation.Landscape);

    readonly
    property alias  viewItem        : view;
    property string currentCategory : "";

    function loadStream (streamId) {
        streamPage.viewItem.currentIndex = -1;
        Feedly.currentStreamId = "";
        pageStack.push (streamPage);
        Feedly.currentStreamId = streamId;
    }

    RemorsePopup { id: remorseLogout; }
    RemorseItem { id: remorseUnsubscribe; }
    SilicaFlickable {
        id: view;
        clip: true;
        contentWidth: width;
        contentHeight: layoutSubscriptions.height;
        anchors {
            top: parent.top;
            left: parent.left;
            right: parent.right;
            bottom: parent.bottom;
            bottomMargin: (panelStatus.expanded ? panelStatus.height : 0);
        }

        PullDownMenu {
            id: pulley;

            MenuItem {
                text: qsTr ("Logout & sweep account");
                font.family: Theme.fontFamilyHeading;
                enabled: !Feedly.isOffline;
                anchors {
                    left: parent.left;
                    right: parent.right;
                }
                onClicked: {
                    remorseLogout.execute ("Logging out, sweeping data...",
                                           function () { Feedly.logoutAndSweepAll (); },
                                           5000);
                }
            }
            MenuItem {
                text: qsTr ("Add new feed... [TODO]");
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
                onClicked: { Feedly.isOffline = !Feedly.isOffline; }
            }
            MenuItem {
                text: qsTr ("Refresh all cache (Slow)");
                font.family: Theme.fontFamilyHeading;
                enabled: !Feedly.isOffline;
                anchors {
                    left: parent.left;
                    right: parent.right;
                }
                onClicked: { Feedly.refreshAll (); }
            }
            MenuItem {
                text: qsTr ("Sync read statuses (Fast)");
                font.family: Theme.fontFamilyHeading;
                enabled: !Feedly.isOffline;
                anchors {
                    left: parent.left;
                    right: parent.right;
                }
                onClicked: { Feedly.syncAllFlags (); }
            }
        }
        Column {
            id: layoutSubscriptions;
            spacing: Theme.paddingSmall;
            anchors {
                left:  (parent ? parent.left  : undefined);
                right: (parent ? parent.right : undefined);
            }

            PageHeader {
                title: qsTr ("Feed'me");
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
            ListItem {
                id: itemAll;
                onClicked: { loadStream (categoryAll.streamId); }

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
            ListItem {
                id: itemStarred;
                onClicked: { loadStream (categoryStarred.streamId); }

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
            Repeater {
                model: Feedly.subscriptionsList;
                delegate: ListItem {
                    id: itemSubscription;
                    visible: (itemSubscription.isCategoryItem || itemSubscription.category === currentCategory);
                    height: contentHeight + (menuOpen ? _menuItem.height : 0);
                    contentHeight: Theme.itemSizeSmall;
                    menu: (itemSubscription.isCategoryItem ? menuCategory : menuFeed);
                    anchors {
                        left:  (parent ? parent.left  : undefined);
                        right: (parent ? parent.right : undefined);
                    }
                    onClicked: { loadStream (itemSubscription.isCategoryItem ? itemSubscription.category : itemSubscription.feed); }
                    ListView.onAdd: AddAnimation { target: itemSubscription; }

                    property string       feed             : model ['feedId'];
                    property string       category         : model ['categoryId'];
                    property bool         isCategoryItem   : (feed === "" && category !== "");
                    property bool         isCurrentSection : (currentCategory === itemSubscription.category);
                    property FeedInfo     feedInfo         : Feedly.getFeedInfo (itemSubscription.feed);
                    property CategoryInfo categoryInfo     : Feedly.getCategoryInfo (itemSubscription.category);

                    Component {
                        id: menuCategory;

                        ContextMenu {
                            MenuItem {
                                text: qsTr ("Force refresh this category");
                                onClicked: { Feedly.refreshStream (itemSubscription.category); }
                            }
                            MenuItem {
                                text: qsTr ("Edit this category [TODO]");
                                onClicked: {
                                    // TODO : dialog to edit the category
                                }
                            }
                        }
                    }
                    Component {
                        id: menuFeed;

                        ContextMenu {
                            MenuItem {
                                text: qsTr ("Force refresh this feed");
                                onClicked: { Feedly.refreshStream (itemSubscription.feed); }
                            }
                            MenuItem {
                                text: qsTr ("Unsubscribe this feed [TODO]");
                                onClicked: {
                                    remorseUnsubscribe.execute (itemSubscription,
                                                                qsTr ("Removing feed"),
                                                                function () {
                                                                    // TODO : remove feed and unsubscribe
                                                                },
                                                                5000);
                                }
                            }
                        }
                    }
                    Item {
                        id: contentCategory;
                        visible: itemSubscription.isCategoryItem;
                        height: itemSubscription.contentHeight;
                        anchors {
                            top: parent.top;
                            left: parent.left;
                            right: parent.right;
                        }

                        MouseArea {
                            id: clicker;
                            width: height;
                            anchors {
                                top: parent.top;
                                left: parent.left;
                                bottom: parent.bottom;
                            }
                            onClicked: { currentCategory = (!itemSubscription.isCurrentSection ? itemSubscription.category : ""); }

                            Image {
                                source: "../img/arrow-right.png";
                                rotation: (itemSubscription.isCurrentSection ? +90 : 0);
                                anchors.centerIn: parent;

                                Behavior on rotation { NumberAnimation { duration: 180; } }
                            }
                        }
                        Label {
                            text: (itemSubscription.categoryInfo ? itemSubscription.categoryInfo.label : "");
                            textFormat: Text.PlainText;
                            truncationMode: TruncationMode.Fade;
                            font.family: Theme.fontFamilyHeading;
                            color: (itemSubscription.isCurrentSection ? Theme.highlightColor : Theme.primaryColor);
                            anchors {
                                left: clicker.right;
                                right: bubbleCategory.left;
                                leftMargin: Theme.paddingSmall;
                                rightMargin: Theme.paddingSmall;
                                verticalCenter: parent.verticalCenter;
                            }
                        }
                        Bubble {
                            id: bubbleCategory;
                            value: (itemSubscription.categoryInfo ? itemSubscription.categoryInfo.counter : 0);
                            anchors {
                                right: parent.right;
                                margins: Theme.paddingSmall;
                                verticalCenter: parent.verticalCenter;
                            }
                        }
                    }
                    Item {
                        id: contentFeed;
                        visible: !itemSubscription.isCategoryItem;
                        height: itemSubscription.contentHeight;
                        anchors {
                            top: parent.top;
                            left: parent.left;
                            right: parent.right;
                        }

                        Item {
                            id: holder;
                            width: height;
                            anchors {
                                top: parent.top;
                                left: parent.left;
                                bottom: parent.bottom;
                            }

                            Image {
                                id: symbolStatus;
                                visible: (itemSubscription.feedInfo && itemSubscription.feedInfo.status !== FeedInfo.Idle);
                                source: "../img/pending.png";
                                anchors.centerIn: parent;
                            }
                            BusyIndicator {
                                running: visible;
                                visible: (itemSubscription.feedInfo && itemSubscription.feedInfo.status === FeedInfo.Fetching);
                                size: BusyIndicatorSize.Medium;
                                anchors.centerIn: parent;
                            }
                        }
                        Label {
                            text: (itemSubscription.feedInfo ? itemSubscription.feedInfo.title : "");
                            textFormat: Text.PlainText;
                            truncationMode: TruncationMode.Fade;
                            font.pixelSize: Theme.fontSizeSmall;
                            font.family: Theme.fontFamilyHeading;
                            color: Theme.primaryColor;
                            anchors {
                                left: holder.right;
                                right: bubbleFeed.left;
                                leftMargin: Theme.paddingSmall;
                                rightMargin: Theme.paddingSmall;
                                verticalCenter: parent.verticalCenter;
                            }
                        }
                        Bubble {
                            id: bubbleFeed;
                            value: (itemSubscription.feedInfo ? itemSubscription.feedInfo.counter : 0);
                            anchors {
                                right: parent.right;
                                margins: Theme.paddingSmall;
                                verticalCenter: parent.verticalCenter;
                            }
                        }
                    }
                }
            }
            Item {
                id: footer;
                height: btnBackToTop.height;
                anchors {
                    left:  (parent ? parent.left  : undefined);
                    right: (parent ? parent.right : undefined);
                }
            }
        }
        VerticalScrollDecorator {}
    }
    Rectangle {
        visible: btnBackToTop.visible;
        gradient: Gradient {
            GradientStop { position: 0.0; color: Qt.rgba (0, 0, 0, 0); }
            GradientStop { position: 1.0; color: Qt.rgba (0, 0, 0, 1); }
        }
        anchors {
            fill: btnBackToTop;
            topMargin: (-btnBackToTop.height / 2);
        }
    }
    Button {
        id: btnBackToTop;
        text: qsTr ("Back to top");
        visible: (!view.atYBeginning && view.visibleArea.heightRatio < 1.0 && !pulley.active);
        anchors {
            left: view.left;
            right: view.right;
            bottom: view.bottom;
        }
        onClicked: { view.scrollToTop (); }
    }
}

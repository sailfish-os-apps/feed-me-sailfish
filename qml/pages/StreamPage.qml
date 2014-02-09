import QtQuick 2.0;
import Sailfish.Silica 1.0;
import harbour.feedme.myQtCoreImports 5.1;
import "../components";

Page {
    id: page;
    allowedOrientations: (Orientation.Portrait | Orientation.Landscape);

    readonly property alias viewItem : view;

    RemorsePopup {
        id: remorseMarkAllRead;
    }
    Connections {
        target: Feedly;
        onCurrentStreamIdChanged: { view.positionViewAtBeginning (); }
        onShowOnlyUnreadChanged:  { view.positionViewAtBeginning (); }
    }
    SilicaListView {
        id: view;
        clip: true;
        model: Feedly.newsStreamList;
        highlightRangeMode: ListView.ApplyRange;
        highlightFollowsCurrentItem: true;
        preferredHighlightBegin: (height * 1 / 4);
        preferredHighlightEnd:   (height * 3 / 4);
        currentIndex: -1;
        onCurrentIndexChanged: {
            if (currentIndex >= 0 && currentIndex < Feedly.newsStreamList.count ()) {
                Feedly.currentEntryId = Feedly.newsStreamList.valueAt (streamPage.viewItem.currentIndex) ['entryId'];
            }
            else {
                Feedly.currentEntryId = "";
            }
        }
        header: Column {
            anchors {
                left:  (parent ? parent.left  : undefined);
                right: (parent ? parent.right : undefined);
            }

            Item {
                height: Theme.paddingLarge;
                width: parent.width;
            }
            Text {
                text: Feedly.currentStreamId !== ''
                      ? (Feedly.currentStreamId.indexOf ("user/") === 0
                         ? Feedly.getCategoryInfo (Feedly.currentStreamId) ['label']
                         : Feedly.getFeedInfo     (Feedly.currentStreamId) ['title'])
                      : "";
                color: Theme.highlightColor;
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere;
                horizontalAlignment: Text.AlignRight;
                maximumLineCount: 2;
                elide: Text.ElideRight;
                font.family: Theme.fontFamilyHeading;
                font.pixelSize: Theme.fontSizeLarge;
                anchors {
                    left: parent.left;
                    right: parent.right;
                    leftMargin: (Theme.paddingLarge * 4);
                    rightMargin: Theme.paddingLarge;
                }
            }
        }
        section {
            property: "date";
            delegate: Label {
                text: Qt.formatDate (new Date (section), Qt.SystemLocaleLongDate);
                font.pixelSize: Theme.fontSizeExtraSmall;
                font.family: Theme.fontFamilyHeading;
                color: Theme.secondaryHighlightColor;
                height: Theme.itemSizeMedium;
                horizontalAlignment: Text.AlignRight;
                verticalAlignment: Text.AlignBottom;
                anchors {
                    left: parent.left;
                    right: parent.right;
                    margins: Theme.paddingLarge;
                }
            }
        }
        delegate: ListItem {
            id: itemNews;
            contentHeight: Theme.itemSizeMedium;
            anchors {
                left: parent.left;
                right: parent.right;
            }
            onClicked: {
                view.currentIndex = model.index;
                pageStack.push (contentPage);
            }
            ListView.onAdd: AddAnimation { target: itemNews; }

            property bool        isCurrentIdx : (model.index === view.currentIndex);
            property ContentInfo contentInfo  : Feedly.getContentInfo (model ['entryId']);

            Item {
                height: itemNews.contentHeight;
                anchors {
                    top: parent.top;
                    left: parent.left;
                    right: parent.right;
                }

                GlassItem {
                    color: Theme.highlightColor;
                    visible: itemNews.contentInfo.unread;
                    anchors {
                        horizontalCenter: parent.right;
                        verticalCenter: parent.verticalCenter;
                    }
                }
                Image {
                    id: imgThumbnail;
                    source: (!Feedly.isOffline  && itemNews.contentInfo.thumbnail !== "" ? itemNews.contentInfo.thumbnail : "../img/noimage.png");
                    fillMode: Image.PreserveAspectCrop;
                    asynchronous: true;
                    cache: true;
                    width: height;
                    anchors {
                        top: parent.top;
                        left: parent.left;
                        bottom: parent.bottom;
                    }

                    BusyIndicator {
                        running: (parent.status === Image.Loading);
                        visible: running;
                        size: BusyIndicatorSize.Medium;
                        anchors.centerIn: parent;
                    }
                }
                Text {
                    text: itemNews.contentInfo.title;
                    textFormat: Text.PlainText;
                    wrapMode: Text.WrapAtWordBoundaryOrAnywhere;
                    maximumLineCount: 2;
                    font.pixelSize: Theme.fontSizeSmall;
                    font.family: Theme.fontFamilyHeading;
                    elide: Text.ElideRight;
                    color: (itemNews.highlighted || itemNews.isCurrentIdx ? Theme.highlightColor : (itemNews.contentInfo.unread ? Theme.primaryColor : Theme.secondaryColor));
                    anchors {
                        left: imgThumbnail.right;
                        right: parent.right;
                        leftMargin: Theme.paddingMedium;
                        rightMargin: Theme.paddingMedium;
                        verticalCenter: parent.verticalCenter;
                    }
                }
            }
        }
        footer: Item {
            height: btnBackToTop.height;
            anchors {
                left:  (parent ? parent.left  : undefined);
                right: (parent ? parent.right : undefined);
            }
        }
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
                text: qsTr ("Show only unread : <b>%1</b>").arg (Feedly.showOnlyUnread ? qsTr ("ON") : qsTr ("OFF"));
                textFormat: Text.StyledText;
                font.family: Theme.fontFamilyHeading;
                anchors {
                    left: parent.left;
                    right: parent.right;
                }
                onClicked: { Feedly.showOnlyUnread = !Feedly.showOnlyUnread; }
            }
            MenuItem {
                text: qsTr ("Check for newer items...");
                enabled: !Feedly.isOffline;
                font.family: Theme.fontFamilyHeading;
                anchors {
                    left: parent.left;
                    right: parent.right;
                }
                onClicked: {
                    Feedly.refreshStream (Feedly.currentStreamId);
                    // TODO : still missing logic to compute 'X new items to show' diff count !
                }
            }
            MenuItem {
                text: qsTr ("Mark all as read");
                font.family: Theme.fontFamilyHeading;
                anchors {
                    left: parent.left;
                    right: parent.right;
                }
                onClicked: {
                    remorseMarkAllRead.execute (qsTr ("Marking all news read"),
                                                function () {
                                                    Feedly.markCurrentStreamAsRead ();
                                                },
                                                3000);
                }
            }
        }
        PushUpMenu {
            id: pulleyUp;

            MenuItem {
                text: qsTr ("Mark all as read");
                font.family: Theme.fontFamilyHeading;
                anchors {
                    left: parent.left;
                    right: parent.right;
                }
                onClicked: {
                    remorseMarkAllRead.execute (qsTr ("Marking all news read"),
                                                function () {
                                                    Feedly.markCurrentStreamAsRead ();
                                                },
                                                3000);
                }
            }
            MenuItem {
                text: qsTr ("Load %1 older items...").arg (Feedly.pageSize);
                font.family: Theme.fontFamilyHeading;
                anchors {
                    left: parent.left;
                    right: parent.right;
                }
                onClicked: { delayLoadMore.start (); }
            }
        }
        ViewPlaceholder {
            text: qsTr ("No news in this stream.");
            enabled: (view.count == 0);
        }
        VerticalScrollDecorator { }
    }
    ScrollDimmer { flickable: view; }
    Timer {
        id: delayLoadMore;
        interval: 500;
        running: false;
        repeat: false;
        onTriggered: {
            pulleyUp.active = false;
            var save = view.contentY;
            Feedly.currentPageCount++;
            view.contentY = save;
        }
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
        visible: (!view.atYBeginning && view.visibleArea.heightRatio < 1.0 && !pulley.active && !pulleyUp.active);
        anchors {
            left: parent.left;
            right: parent.right;
            bottom: parent.bottom;
            margins: 0;
        }
        onClicked: { view.positionViewAtBeginning (); }
    }
}

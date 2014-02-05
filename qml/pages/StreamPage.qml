import QtQuick 2.0;
import Sailfish.Silica 1.0;
import harbour.feedme.myQtCoreImports 5.1;
import "../components";

Page {
    id: page;
    allowedOrientations: (Orientation.Portrait | Orientation.Landscape);

    RemorsePopup {
        id: remorseMarkAllRead;
    }
    Connections {
        target: Feedly;
        onNewsStreamListChanged: {
            view.positionViewAtBeginning ();
            modelNewsStream.clear ();
            modelNewsStream.append (Feedly.newsStreamList);
        }
    }
    SilicaListView {
        id: view;
        currentIndex: -1;
        model: ListModel {
            id: modelNewsStream;
        }
        header: Column {
            spacing: Theme.paddingSmall;
            anchors {
                left: (parent ? parent.left : undefined);
                right: (parent ? parent.right : undefined);
            }

            Item {
                height: Theme.paddingMedium;
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
        delegate: BackgroundItem {
            id: itemNews;
            height: Theme.itemSizeMedium;
            anchors {
                left: parent.left;
                right: parent.right;
            }
            onClicked: {
                Feedly.currentEntryId = contentInfo.entryId;
                pageStack.push (contentPage);
            }

            property ContentInfo contentInfo : Feedly.getContentInfo (model ['entryId']);

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
                source: (itemNews.contentInfo.thumbnail !== "" ? itemNews.contentInfo.thumbnail : "../img/noimage.png");
                fillMode: Image.PreserveAspectCrop;
                asynchronous: true;
                cache: true;
                width: height;
                anchors {
                    top: parent.top;
                    left: parent.left;
                    bottom: parent.bottom;
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
                color: Theme.primaryColor;
                anchors {
                    left: imgThumbnail.right;
                    right: parent.right;
                    leftMargin: Theme.paddingSmall;
                    rightMargin: Theme.paddingLarge;
                    verticalCenter: parent.verticalCenter;
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
        anchors.fill: parent;

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
                text: qsTr ("Check for newer items... [TODO]");
                font.family: Theme.fontFamilyHeading;
                anchors {
                    left: parent.left;
                    right: parent.right;
                }
                onClicked: {
                    // TODO : load the items that were cached since stream was opened, and maybe pull new ones before
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
                text: qsTr ("Load older items... [TODO]");
                font.family: Theme.fontFamilyHeading;
                anchors {
                    left: parent.left;
                    right: parent.right;
                }
                onClicked: {
                    // TODO : logic to load older items
                }
            }
        }
        VerticalScrollDecorator { }
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
        onClicked: { view.scrollToTop (); }
    }
}

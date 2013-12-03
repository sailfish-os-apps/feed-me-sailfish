import QtQuick 2.0;
import Sailfish.Silica 1.0;
//import "../components";

Page {
    id: page;
    allowedOrientations: (Orientation.Portrait | Orientation.Landscape);

    SilicaListView {
        id: view;
        currentIndex: -1;
        model: 0;
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
                text: Database.currentStreamId !== ''
                      ? (Database.currentStreamId.indexOf ("/category/") > -1
                         ? Database.getCategoryInfo (Database.currentStreamId) ['label']
                         : Database.getFeedInfo     (Database.currentStreamId) ['title'])
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
        delegate: BackgroundItem {

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
                text: qsTr ("Show only unread : <b>%1</b>").arg (Database.showOnlyUnread ? qsTr ("ON") : qsTr ("OFF"));
                textFormat: Text.StyledText;
                font.family: Theme.fontFamilyHeading;
                anchors {
                    left: parent.left;
                    right: parent.right;
                }
                onClicked: { Database.showOnlyUnread = !Database.showOnlyUnread; }
            }
            MenuItem {
                text: qsTr ("Check for newer items...");
                font.family: Theme.fontFamilyHeading;
                anchors {
                    left: parent.left;
                    right: parent.right;
                }
                onClicked: {

                }
            }
        }
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
}

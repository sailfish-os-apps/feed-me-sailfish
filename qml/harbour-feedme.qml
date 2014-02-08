import QtQuick 2.0;
import harbour.feedme.myQtCoreImports 5.1;
import Sailfish.Silica 1.0;
import "Ajax.js" as Ajax;
import "pages";
import "cover";

ApplicationWindow {
    id: rootItem;
    cover: CoverPage { }
    initialPage: mainPage;

    MainPage {
        id: mainPage;
    }
    StreamPage {
        id: streamPage;
    }
    ContentPage {
        id: contentPage;
    }
    DockedPanel {
        id: panelStatus;
        dock: Dock.Bottom;
        open: Feedly.isPolling;
        height: (indicatorPolling.height + indicatorPolling.anchors.margins * 2);
        anchors {
            left: parent.left;
            right: parent.right;
        }

        Rectangle {
            color: "white";
            opacity: 0.05;
            anchors.fill: parent;
        }
        BusyIndicator {
            id: indicatorPolling;
            running: Feedly.isPolling;
            visible: running;
            size: BusyIndicatorSize.Medium;
            anchors {
                left: parent.left;
                margins: Theme.paddingMedium;
                verticalCenter: parent.verticalCenter;
            }
        }
        Label {
            text: Feedly.currentStatusMsg;
            textFormat: Text.PlainText;
            font.pixelSize: Theme.fontSizeSmall;
            font.family: Theme.fontFamilyHeading;
            color: Theme.secondaryColor;
            anchors {
                left: indicatorPolling.right;
                right: parent.right;
                margins: Theme.paddingLarge;
                verticalCenter: parent.verticalCenter;
            }
        }
    }
    Formatter {
        id: formatter;
    }
}



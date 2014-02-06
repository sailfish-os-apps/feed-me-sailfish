import QtQuick 2.0;
import Sailfish.Silica 1.0;
import harbour.feedme.myQtCoreImports 5.1;

CoverBackground {
    id: background;

    Image {
        source: "../img/logo.png";
        fillMode: Image.PreserveAspectFit;
        verticalAlignment: Image.AlignTop;
        horizontalAlignment: Image.AlignLeft;
        anchors.fill: parent;
    }
    Label {
        id: lblCounter;
        text: (Feedly.isLogged ? globalCategory.counter : "");
        font.pixelSize: Theme.fontSizeLarge;
        font.family: Theme.fontFamilyHeading
        color: Theme.highlightColor;
        anchors {
            top: parent.top;
            left: parent.left;
            margins: Theme.paddingLarge;
        }

        property CategoryInfo globalCategory : Feedly.getCategoryInfo (Feedly.getStreamIdAll ());
    }
    Label {
        text: (Feedly.isLogged ? qsTr ("news") : qsTr ("Please log in"));
        fontSizeMode: Text.HorizontalFit;
        font.pixelSize: Theme.fontSizeSmall;
        font.family: Theme.fontFamilyHeading
        color: Theme.highlightColor;
        anchors {
            baseline: lblCounter.baseline;
            left: lblCounter.right;
            leftMargin: Theme.paddingSmall;
            right: parent.right;
            rightMargin: Theme.paddingLarge;
        }
    }
    Item {
        height: (parent.height / 4);
        visible: Feedly.isPolling;
        anchors {
            left: parent.left;
            right: parent.right;
            bottom: parent.bottom;
        }

        Image {
            source: "image://theme/graphic-busyindicator-medium?%1".arg (Theme.highlightColor.toString ());
            transformOrigin: Item.Center;
            anchors.centerIn: parent;

            NumberAnimation on rotation {
                from: 0;
                to: 360;
                duration: 2000;
                running: (Feedly.isPolling && !Qt.application.active);
                loops: Animation.Infinite;
            }
        }
    }
    CoverActionList {
        id: coverAction;
        enabled: (!Feedly.isOffline && Feedly.isLogged && !Feedly.isPolling);
        iconBackground: false;

        CoverAction {
            iconSource: "image://theme/icon-cover-sync";
            onTriggered: { Feedly.refreshAll (); }
        }
    }
}

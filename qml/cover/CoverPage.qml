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
        text: globalCategory.counter;
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
        text: qsTr ("news");
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
    CoverActionList {
        id: coverAction;
        iconBackground: false;

        CoverAction {
            iconSource: "image://theme/icon-cover-sync";
            onTriggered: {
                // TODO : refresh / sync all
            }
        }
    }
}



import QtQuick 2.0;
import Sailfish.Silica 1.0;

Rectangle {
    id: base;
    width: Theme.itemSizeExtraSmall;
    height: width;
    radius: (width / 2);
    antialiasing: true;
    visible: (value > 0);
    color: "transparent";
    border {
        width: 1;
        color: Theme.secondaryHighlightColor;
    }

    property int value : 0;

    Text {
        text: base.value;
        font.pixelSize: Theme.itemSizeExtraLarge;
        font.bold: false
        font.family: Theme.fontFamilyHeading;
        color: Theme.secondaryHighlightColor;
        fontSizeMode: Text.Fit;
        horizontalAlignment: Text.AlignHCenter;
        verticalAlignment: Text.AlignVCenter;
        anchors {
            fill: parent;
            margins: (parent.width / 4);
        }
    }
}

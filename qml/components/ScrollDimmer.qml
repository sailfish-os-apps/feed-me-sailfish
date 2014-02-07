import QtQuick 2.1;

Rectangle {
    gradient: Gradient {
        GradientStop { position: 0.00; color: (!flickable.atYBeginning ? "black" : "transparent"); }
        GradientStop { position: 0.15; color: "transparent"; }
        GradientStop { position: 0.85; color: "transparent"; }
        GradientStop { position: 1.00; color: (!view.atYEnd ? "black" : "transparent"); }
    }
    anchors.fill: flickable;

    property Flickable flickable : parent;
}

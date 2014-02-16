import QtQuick 2.1;

Rectangle {
    visible: (!hasPullDownMenu && !hasPushUpMenu);
    gradient: Gradient {
        GradientStop { position: 0.00; color: (!flickable.atYBeginning ? dimColor : "transparent"); }
        GradientStop { position: 0.15; color: "transparent"; }
        GradientStop { position: 0.85; color: "transparent"; }
        GradientStop { position: 1.00; color: (!flickable.atYEnd ? dimColor : "transparent"); }
    }
    anchors.fill: flickable;

    property Flickable flickable  : null;
    property color     dimColor   : Qt.rgba (0, 0, 0, 0.65);

    property bool hasPullDownMenu : (flickable && ('pullDownMenu' in flickable) && flickable.pullDownMenu && flickable.pullDownMenu.active);
    property bool hasPushUpMenu   : (flickable && ('pushUpMenu'   in flickable) && flickable.pushUpMenu   && flickable.pushUpMenu.active);
}

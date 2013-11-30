import QtQuick 2.0;
import Sailfish.Silica 1.0;
//import "../components";

Page {
    id: page;
    allowedOrientations: (Orientation.Portrait | Orientation.Landscape);

    PageHeader {
        id: header;
        title: qsTr ("Stream view");
    }



}

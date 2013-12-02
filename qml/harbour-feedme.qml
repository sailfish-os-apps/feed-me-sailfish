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


    ListModel {
        id: modelSubscriptions;

        ListElement {
            title: "Clubic";
            category: "Actus IT";
            count: 23;
        }
        ListElement {
            title: "01.net";
            category: "Actus IT";
            count: 12;
        }
        ListElement {
            title: "Engadget";
            category: "Actus IT";
            count: 127;
        }
        ListElement {
            title: "BlogGeek";
            category: "Actus IT";
            count: 0;
        }
        ListElement {
            title: "Phoronix";
            category: "Actus IT";
            count: 9;
        }
        ListElement {
            title: "Qt Blog by Digia";
            category: "Qt / QML";
            count: 4;
        }
        ListElement {
            title: "Qt Planet";
            category: "Qt / QML";
            count: 6;
        }
        ListElement {
            title: "The KDE blog";
            category: "Qt / QML";
            count: 3;
        }
    }

}



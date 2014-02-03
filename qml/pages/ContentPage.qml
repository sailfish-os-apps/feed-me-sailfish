import QtQuick 2.0;
import Sailfish.Silica 1.0;
import harbour.feedme.myQtCoreImports 5.1;
import "../components";

Page {
    id: page;
    allowedOrientations: (Orientation.Portrait | Orientation.Landscape);

    readonly property string googleFaviconWebServiceUrl : "http://www.google.com/s2/favicons?domain=%1";

    property string bodyCSS : '<style type="text/css">p { margin: 0; margin-bottom: 5px; padding: 0; } a { color: %1; } pre { margin: 10px; padding: 10px; font-family: "Ubuntu Mono, monospace" !important; white-space: pre-wrap; color: %2; text-align: left !important; } </style>'.arg (Theme.highlightColor).arg (Theme.secondaryHighlightColor);

    property ContentInfo currentNewsItem : Feedly.getContentInfo (Feedly.currentEntryId);
    property FeedInfo    currentFeedItem : null;
    onCurrentNewsItemChanged: {
        view.contentY = 0;
        currentFeedItem = null;
        repeaterImages.model = 0;
        htmlView.text = "";
        imgStreamIcon.source = "";
        labelSource.text = "";
        dialogImg.uri = "";
        if (currentNewsItem !== null) {
            currentFeedItem = Feedly.getFeedInfo (currentNewsItem.streamId);
            if (currentNewsItem.unread) {
                //FeedlyApi.updateUnreadEntry (model.entryId);
                currentNewsItem.unread = false;
            }
            htmlView.text = formatBody (currentNewsItem.content)
            repeaterImages.model = extractImages (currentNewsItem.content);
            imgStreamIcon.source = googleFaviconWebServiceUrl.arg (currentFeedItem.website);
            labelSource.text =  currentFeedItem.title + (currentNewsItem.author !== "" ? "  (<b>%1</b>)".arg (currentNewsItem.author) : "");
        }
    }

    function extractImages (str) {
        String.prototype.contains = function (str) { return (this.indexOf (str) > -1); }
        var ret = [];
        var imgList = (str.match (/<img\s+[^>]*">/gi) || []);
        for (var i = 0; i < imgList.length; i++) {
            var img = imgList [i];
            var url = ((img.match (/src="([^"]*)"/i) || []) [1] || "");
            var title = ((img.match (/alt="([^"]*)"/i) || []) [1] || "");
            if (url !== ""
                    && !url.contains ("res3.feedsportal.com")
                    && !url.contains ("feeds.feedburner.com")
                    && !url.contains ("doubleclick.net")
                    && !url.contains ("feeds.wordpress.com")
                    && !url.contains ("stats.wordpress.com")
                    && !url.contains ("/smilies/")
                    && !url.contains ("a.fsdn.com")
                    && !(url.contains ("da.feedsportal.com") && url.contains ("/rc.img"))
                    && !(img.contains ('height="1"') && img.contains ('width="1"'))
                    ) {
                console.debug ("img found:", img);
                ret.push ({ "chunk" : img, "url" : url, "title" : title });
            }
        }
        return ret;
    }
    function formatBody (str) {
        if (str) {
            var ret = str;
            var imgList = extractImages (ret);
            var placeholder = '###PLACEHOLDER###';
            // replacing img with refs
            for (var i = 0; i < imgList.length; i++) {
                var img = imgList [i];
                var title = img ['title'];
                ret = ret.replace (img ['chunk'], '<br /><br /><span style="color:transparent">###IMG#%1###</span>'.arg (i + 1) + placeholder);
            }
            // clean remaining img tags
            ret = ret.replace (/<img\s+[^>]*">/gi, '');
            // replace placeholders with imgs
            ret = ret.replace (/###PLACEHOLDER###/gi, '<br /><img src="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAPoAAAD6AQMAAACyIsh+AAAAA3NCSVQICAjb4U/gAAAACXBIWXMAAA3XAAAN1wFCKJt4AAAAGXRFWHRTb2Z0d2FyZQB3d3cuaW5rc2NhcGUub3Jnm+48GgAAAANQTFRF////p8QbyAAAAAF0Uk5TAEDm2GYAAAAeSURBVBgZ7cEBAQAAAIKg/q92SMAAAAAAAAAAgGMBIDoAAcs7EDgAAAAASUVORK5CYII=" style="background: white; border: 1px solid white; width: 250px; height: 250px;" /><br /><br />');
            return bodyCSS + ret;
        }
        else {
            return "";
        }
    }

    SilicaFlickable {
        id: view;
        contentWidth: width;
        contentHeight: (layout.height + layout.anchors.margins * 2);
        anchors.fill: parent;

        PullDownMenu {
            id: pulley;

            MenuItem {
                text: qsTr ("Share...");
                font.family: Theme.fontFamilyHeading;
                enabled: false; // TODO : when Jolla publish Sharing API
                anchors {
                    left: parent.left;
                    right: parent.right;
                }
                onClicked: { }
            }
            MenuItem {
                text: qsTr ("Mark for later");
                font.family: Theme.fontFamilyHeading;
                anchors {
                    left: parent.left;
                    right: parent.right;
                }
                onClicked: {
                    // TODO : api to change marked
                }
            }
            MenuItem {
                text: qsTr ("View original link");
                font.family: Theme.fontFamilyHeading;
                enabled: currentNewsItem;
                anchors {
                    left: parent.left;
                    right: parent.right;
                }
                onClicked: { Qt.openUrlExternally (currentNewsItem.link); }
            }
        }
        Column {
            id: layout;
            spacing: Theme.paddingLarge;
            anchors {
                top: parent.top;
                left: parent.left;
                right: parent.right;
                margins: Theme.paddingLarge;
            }

            Text {
                text: (currentNewsItem ? "\t\t\t" + currentNewsItem.title : "");
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere;
                textFormat: Text.PlainText;
                color: Theme.highlightColor;
                font.pixelSize: Theme.fontSizeMedium;
                font.family: Theme.fontFamilyHeading;
                horizontalAlignment: Text.AlignRight;
                anchors {
                    left: parent.left;
                    right: parent.right;
                }
            }
            Row {
                id: layoutInfo;
                spacing: 10;
                anchors {
                    left: parent.left;
                    right: parent.right;
                }

                Image {
                    id: imgStreamIcon;
                    width: 16;
                    height: width;
                    sourceSize.width: width;
                    sourceSize.height: height;
                    anchors.verticalCenter: parent.verticalCenter;
                }
                Text {
                    id: labelSource;
                    color: Theme.secondaryHighlightColor;
                    width: (parent.width - imgStreamIcon.width - parent.spacing);
                    elide: Text.ElideRight;
                    textFormat: Text.StyledText;
                    font {
                        pixelSize: Theme.fontSizeExtraSmall;
                        family: Theme.fontFamilyHeading;
                    }
                    anchors.verticalCenter: parent.verticalCenter;
                }
            }
            TextEdit {
                id: htmlView;
                readOnly: true;
                activeFocusOnPress: false;
                selectByMouse: false;
                textFormat: TextEdit.RichText;
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere;
                color: Theme.primaryColor;
                font {
                    pixelSize: Theme.fontSizeSmall;
                    family: Theme.fontFamilyHeading;
                }
                anchors {
                    left: parent.left;
                    right: parent.right;
                }
                onLinkActivated: {
                    Qt.openUrlExternally (link);
                }

                Repeater {
                    id: repeaterImages;
                    delegate: Item {
                        width: 250;
                        height: (width /*+ bar.height*/);
                        y: (pos ? pos.y : 0);
                        anchors.horizontalCenter: htmlView.horizontalCenter;

                        property var pos : {
                            var ret;
                            var str = "###IMG#%1###".arg (model.index +1);
                            var len = str.length;
                            if (htmlView.length > 0
                                    && htmlView.width > 0
                                    && htmlView.contentWidth > 0
                                    && htmlView.height > 0
                                    && htmlView.contentHeight > 0) {
                                for (var i = 0; i < htmlView.length - len; i++) {
                                    if (htmlView.getText (i, i + len) === str) {
                                        ret = htmlView.positionToRectangle (i);
                                        break;
                                    }
                                }
                            }
                            return ret;
                        }

                        MouseArea {
                            anchors.fill: parent;
                            onClicked: { dialogImg.uri = modelData ['url']; }
                        }
                        Rectangle {
                            color: "white";
                            radius: border.width;
                            antialiasing: true;
                            border.width: 2;
                            border.color: Theme.highlightColor;
                            anchors.fill: parent;
                            anchors.margins: -border.width;
                        }
                        Item {
                            clip: true;
                            height: width;
                            anchors {
                                top: parent.top;
                                left: parent.left;
                                right: parent.right;
                            }

                            Image {
                                id: img;
                                source: modelData ['url'];
                                cache: true;
                                asynchronous: true;
                                fillMode: Image.Pad;
                                scale: Math.min (Math.min (parent.width/width, parent.height/width), 1.0);
                                anchors.centerIn: parent;
                            }
                            Text {
                                text: qsTr ("Loading...");
                                color: "gray";
                                visible: (img.status !== Image.Ready);
                                font {
                                    family: Theme.fontFamilyHeading;
                                    pixelSize: Theme.fontSizeTiny;
                                }
                                anchors.centerIn: parent;
                            }
                        }
                        Item {
                            id: bar;
                            height: 40;
                            anchors {
                                top: parent.top;
                                left: parent.left
                                right: parent.right;
                            }

                            Rectangle {
                                color: "black";
                                opacity: 0.65;
                                anchors.fill: parent;
                            }
                            Text {
                                text: "<b>IMG %1:</b> '%2'".arg (model.index + 1).arg (modelData ['title']);
                                color: "white";
                                elide: Text.ElideRight;
                                font {
                                    family: Theme.fontFamilyHeading;
                                    pixelSize: Theme.fontSizeTiny;
                                }
                                anchors {
                                    left: parent.left;
                                    right: parent.right;
                                    margins: 5;
                                    verticalCenter: parent.verticalCenter;
                                }
                            }
                        }
                    }
                }
            }
        }
        VerticalScrollDecorator { }
    }
    Item {
        id: dialogImg;
        scale: (uri !== "" ? 1.0 : 0.0);
        visible: (scale > 0);
        enabled: visible;
        anchors.fill: parent;

        property string uri : "";

        MouseArea {
            anchors.fill: parent;
            onClicked: { }
        }
        Rectangle {
            anchors.fill: parent;
            color: "black";
            opacity: 0.85;
        }
        Flickable {
            id: flickerImg;
            clip: true;
            contentWidth: imgContainer.width;
            contentHeight: imgContainer.height;
            anchors.fill: parent;

            Item {
                id: imgContainer;
                width: Math.max (imgView.width, flickerImg.width);
                height: Math.max (imgView.height, flickerImg.height);

                Image {
                    id: imgView;
                    asynchronous: true;
                    cache: true;
                    source: dialogImg.uri;
                    fillMode: Image.Pad;
                    anchors.centerIn: parent;
                }
                MouseArea {
                    propagateComposedEvents: true;
                    anchors.fill: parent;
                    onClicked: { dialogImg.uri = ""; }
                }
            }
            VerticalScrollDecorator   { }
            HorizontalScrollDecorator { }
        }
        Behavior on scale { NumberAnimation { duration: 350; } }
    }
}

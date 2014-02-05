
#ifdef QT_QML_DEBUG
#include <QtQuick>
#endif

#include <QQuickView>
#include <QQmlContext>
#include <QQmlEngine>
#include <QGuiApplication>
#include <qqml.h>

#include <sailfishapp.h>

#include "MyDataBase.h"

int main (int argc, char * argv []){
    qmlRegisterType            <QTimer>              ("harbour.feedme.myQtCoreImports", 5, 1, "PreciseTimer");
    qmlRegisterUncreatableType <QAbstractItemModel>  ("harbour.feedme.myQtCoreImports", 5, 1, "AbstractItemModel", "");
    qmlRegisterUncreatableType <QAbstractProxyModel> ("harbour.feedme.myQtCoreImports", 5, 1, "AbstractProxyMode", "");
    qmlRegisterUncreatableType <QQmlPropertyMap>     ("harbour.feedme.myQtCoreImports", 5, 1, "QmlPropertyMap", "");
    qmlRegisterUncreatableType <MyFeedlyApi>         ("harbour.feedme.myQtCoreImports", 5, 1, "FeedlyApi", "");
    qmlRegisterType            <MyCategory>          ("harbour.feedme.myQtCoreImports", 5, 1, "CategoryInfo");
    qmlRegisterType            <MyFeed>              ("harbour.feedme.myQtCoreImports", 5, 1, "FeedInfo");
    qmlRegisterType            <MyContent>           ("harbour.feedme.myQtCoreImports", 5, 1, "ContentInfo");
    QGuiApplication * app = SailfishApp::application (argc, argv);
    app->setApplicationName ("FeedMe");
    app->setOrganizationName ("TheBootroo");
    if (!qgetenv ("HTTP_PROXY").isEmpty ()) {
        QString proxyStr = QString::fromLocal8Bit (qgetenv ("HTTP_PROXY")).toLower ().remove ("http://");
        QNetworkProxy::setApplicationProxy (QNetworkProxy (QNetworkProxy::HttpProxy, proxyStr.split (':').first (), proxyStr.split (':').last ().toInt ()));
    }
    QQuickView * view = SailfishApp::createView ();
    view->rootContext ()->setContextProperty ("Feedly", new MyFeedlyApi (view));
    view->setSource (QUrl ("qrc:/qml/harbour-feedme.qml"));
    view->show ();
    return app->exec();
}



#ifdef QT_QML_DEBUG
#include <QtQuick>
#endif

#include <QQuickView>
#include <QQmlContext>
#include <QQmlEngine>
#include <QGuiApplication>
#include <QAbstractProxyModel>
#include <QAbstractItemModel>
#include <QAbstractListModel>
#include <QTimer>
#include <qqml.h>

#include <sailfishapp.h>

int main (int argc, char * argv []){
    qmlRegisterType            <QTimer>              ("harbour.feedme.myQtCoreImports", 5, 1, "PreciseTimer");
    qmlRegisterUncreatableType <QAbstractItemModel>  ("harbour.feedme.myQtCoreImports", 5, 1, "AbstractItemModel", "");
    qmlRegisterUncreatableType <QAbstractProxyModel> ("harbour.feedme.myQtCoreImports", 5, 1, "AbstractProxyMode", "");
    QGuiApplication * app = SailfishApp::application (argc, argv);
    app->setApplicationName ("FeedMe");
    app->setOrganizationName ("TheBootroo");
    QQuickView * view = SailfishApp::createView ();
    view->setSource (QUrl ("qrc:/qml/harbour-feedme.qml"));
    view->show ();
    return app->exec();
}


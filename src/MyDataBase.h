#ifndef MYDATABASE_H
#define MYDATABASE_H

#ifdef QT_QML_DEBUG
#include <QtQuick>
#endif

#include <QQuickView>
#include <QQmlContext>
#include <QQmlEngine>
#include <QGuiApplication>
#include <qqml.h>
#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QSqlError>
#include <QSqlRecord>
#include <QDir>
#include <QUrl>
#include <QHash>
#include <QQmlPropertyMap>
#include <QStringList>
#include <QDateTime>
#include <QAbstractProxyModel>
#include <QAbstractItemModel>
#include <QAbstractListModel>
#include <QSortFilterProxyModel>
#include <QVariant>
#include <QMetaEnum>
#include <QMetaObject>
#include <QMetaProperty>
#include <QMetaMethod>
#include <QRegularExpression>
#include <QSettings>
#include <QTcpServer>
#include <QTcpSocket>
#include <QByteArray>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QNetworkProxy>
#include <QIODevice>
#include <QAbstractSocket>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonObject>
#include <QUrl>
#include <QUrlQuery>
#include <QTimer>
#include <QQueue>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QDebug>

#undef USE_SANDBOX

#ifdef USE_SANDBOX
#define apiBaseUrl       QString ("http://sandbox.feedly.com")
#define apiClientId      QString ("sandbox221")
#define apiClientSecret  QString ("AX1EKDZVWUYOSX9RR2VXR8SE")
#else
#define apiBaseUrl       QString ("http://cloud.feedly.com")
#define apiClientId      QString ("boutroue")
#define apiClientSecret  QString ("FE012EGICU4ZOBDRBEOVAJA1JZYH")
#endif

#define apiRedirectUri   QString ("http://localhost")

#define apiAuthScope     QString ("https://cloud.feedly.com/subscriptions")

// TODO : put pagination in settings ?
#define PageSize              250

#define CRLF                  QString ("\r\n")
#define CURR_MSECS            QDateTime::currentMSecsSinceEpoch ()
#define REGEXP_NUMBER         QRegularExpression ("^\\d$")

#define streamIdAll           QString ("user/-/category/global.all")
#define streamIdMarked        QString ("user/-/tag/global.saved")
#define streamIdUncategorized QString ("user/-/category/global.uncategorized")

#define QML_PUBLIC_PROPERTY(type, name) \
    protected: \
        Q_PROPERTY (type name READ get_##name WRITE set_##name NOTIFY name##Changed) \
    private: \
        type m_##name; \
    public: \
        type get_##name () const { \
            return m_##name ; \
        } \
    public Q_SLOTS: \
        bool set_##name (type name) { \
            bool ret = false; \
            if (m_##name != name) { \
                m_##name = name; \
                ret = true; \
                emit name##Changed (m_##name); \
            } \
            return ret; \
        } \
    Q_SIGNALS: \
        void name##Changed (type name);

#define QML_SETTINGS_PROPERTY(type, name) \
    protected: \
        Q_PROPERTY (type name READ get_##name WRITE set_##name NOTIFY name##Changed) \
    public: \
        type get_##name () const { \
            return  m_settings->value (#name).value<type> (); \
        } \
    public Q_SLOTS: \
        bool set_##name (type name) { \
            bool ret = false; \
            if (!m_settings->contains (#name) || m_settings->value (#name).value<type> () != name) { \
                m_settings->setValue (#name, name); \
                ret = true; \
                emit name##Changed (name); \
            } \
            return ret; \
        } \
    Q_SIGNALS: \
        void name##Changed (type name);

#define QML_READONLY_PROPERTY(type, name) \
    protected: \
        Q_PROPERTY (type name READ get_##name CONSTANT) \
    private: \
        type m_##name; \
    public: \
        type get_##name () const { \
            return m_##name ; \
        }

class VariantModel : public QAbstractListModel {
    Q_OBJECT

public:
    explicit VariantModel (QStringList roles, QObject * parent = NULL);

public:
    Q_INVOKABLE int  count   () const;
    Q_INVOKABLE int  roleId  (QString roleName) const;
    Q_INVOKABLE void append  (QVariantMap item);
    Q_INVOKABLE void prepend (QVariantMap item);
    Q_INVOKABLE void deleteAll ();
    Q_INVOKABLE QVariantMap valueAt (int idx) const;

    // QAbstractItemModel interface
    int rowCount (const QModelIndex & parent = QModelIndex ()) const;
    virtual QVariant data (const QModelIndex & index, int role = Qt::DisplayRole) const;
    QHash<int, QByteArray> roleNames () const;

private:
    QList<QVariantMap>     m_items;
    QHash<int, QByteArray> m_roles;
};

class MyCategory : public QObject {
    Q_OBJECT
    QML_PUBLIC_PROPERTY (QString, streamId)
    QML_PUBLIC_PROPERTY (QString, label)
    QML_PUBLIC_PROPERTY (int,     counter)

public: explicit MyCategory (QObject * parent = NULL) : QObject (parent) {
        m_streamId = "";
        m_label    = "";
        m_counter  = 0;
    }
};

class MyFeed : public QObject {
    Q_OBJECT
    Q_ENUMS (FeedStatus)
    QML_PUBLIC_PROPERTY (QString, streamId)
    QML_PUBLIC_PROPERTY (QString, categoryId)
    QML_PUBLIC_PROPERTY (QString, title)
    QML_PUBLIC_PROPERTY (QString, website)
    QML_PUBLIC_PROPERTY (int,     status)
    QML_PUBLIC_PROPERTY (int,     counter)

public: explicit MyFeed (QObject * parent = NULL) : QObject (parent) {
        m_categoryId = "";
        m_streamId   = "";
        m_website    = "";
        m_title      = "";
        m_counter    = 0;
        m_status     = Idle;
    }

    enum FeedStatus {
        Idle,
        Pending,
        Fetching,
        Error
    };
};

class MyContent : public QObject {
    Q_OBJECT
    QML_PUBLIC_PROPERTY (QString,   entryId)
    QML_PUBLIC_PROPERTY (QString,   streamId)
    QML_PUBLIC_PROPERTY (QString,   title)
    QML_PUBLIC_PROPERTY (QString,   author)
    QML_PUBLIC_PROPERTY (QString,   content)
    QML_PUBLIC_PROPERTY (QString,   link)
    QML_PUBLIC_PROPERTY (QString,   thumbnail)
    QML_PUBLIC_PROPERTY (bool,      unread)
    QML_PUBLIC_PROPERTY (bool,      marked)
    QML_PUBLIC_PROPERTY (quint64,   published)
    QML_PUBLIC_PROPERTY (quint64,   updated)
    QML_PUBLIC_PROPERTY (quint64,   crawled)
    QML_PUBLIC_PROPERTY (quint64,   cached)

public: explicit MyContent (QObject * parent = NULL) : QObject (parent) {
        m_entryId   = "";
        m_streamId  = "";
        m_title     = "";
        m_author    = "";
        m_content   = "";
        m_link      = "";
        m_thumbnail = "";
        m_unread    = true;
        m_marked    = false;
    }
};

class MyFeedlyApi : public QObject {
    Q_OBJECT
    QML_PUBLIC_PROPERTY   (QString,           currentStreamId)
    QML_PUBLIC_PROPERTY   (QString,           currentEntryId)
    QML_PUBLIC_PROPERTY   (QString,           currentStatusMsg)
    QML_PUBLIC_PROPERTY   (qint64,            streamMostRecentMSecs)
    QML_PUBLIC_PROPERTY   (bool,              isPolling)
    QML_PUBLIC_PROPERTY   (int,               currentPageCount)
    QML_READONLY_PROPERTY (int,               port)
    QML_READONLY_PROPERTY (int,               pageSize)
    QML_READONLY_PROPERTY (VariantModel *,    subscriptionsList)
    QML_READONLY_PROPERTY (VariantModel *,    newsStreamList)
    QML_SETTINGS_PROPERTY (QString,           apiCode)
    QML_SETTINGS_PROPERTY (QString,           apiUserId)
    QML_SETTINGS_PROPERTY (QString,           apiAccessToken)
    QML_SETTINGS_PROPERTY (QString,           apiRefreshToken)
    QML_SETTINGS_PROPERTY (qint64,            lastStart)
    QML_SETTINGS_PROPERTY (qint64,            lastPullMSecs)
    QML_SETTINGS_PROPERTY (bool,              showOnlyUnread)
    QML_SETTINGS_PROPERTY (bool,              isLogged)
    QML_SETTINGS_PROPERTY (bool,              isOffline)

public: // oop
    explicit MyFeedlyApi (QObject * parent = NULL);
    virtual ~MyFeedlyApi ();

public: // methods
    Q_INVOKABLE void         refreshAll              ();
    Q_INVOKABLE void         refreshStream           (QString streamId);
    Q_INVOKABLE void         markItemAsRead          (QString entryId);
    Q_INVOKABLE void         markCurrentStreamAsRead ();
    Q_INVOKABLE void         logoutAndSweepAll       ();

public: // getters
    Q_INVOKABLE MyFeed     * getFeedInfo             (QString feedId);
    Q_INVOKABLE MyCategory * getCategoryInfo         (QString categoryId);
    Q_INVOKABLE MyContent  * getContentInfo          (QString entryId);
    Q_INVOKABLE QString      getOAuthPageUrl         ();
    Q_INVOKABLE QString      getStreamIdAll          ();
    Q_INVOKABLE QString      getStreamIdMarked       ();

public: // setters

signals: // notifiers

signals: // events
    void dataReceived (QString data);

public slots: // methods
    void loadSubscriptions ();
    void loadUnreadCounts  ();

protected slots: // internal routines
    void initializeTables        ();
    void refreshStreamModel      ();
    void requestTokens           ();
    void requestCategories       ();
    void requestSubscriptions    ();
    void requestContents         ();
    void requestReadOperations   ();
    void syncAllFlags            ();
    void pushLocalReadOperations ();

private slots: // internal callbacks
    void onCurrentStreamIdChanged     (QString arg);
    void onShowOnlyUnreadChanged      (bool    arg);
    void onCurrentPageCountChanged    (int     arg);
    void onIsOfflineChanged           (bool    arg);
    void onIncomingConnection         ();
    void onSockReadyRead              ();
    void onRequestTokenReply          ();
    void onRequestCategoriesReply     ();
    void onRequestSubscriptionsReply  ();
    void onRequestContentsReply       ();
    void onRequestReadOperationsReply ();
    void onPushLocalOperationsReply   ();

private: // members
    QSqlDatabase                 m_database;
    QSettings                  * m_settings;
    QTimer                     * m_timerContents;
    QTimer                     * m_timerUnreadCounts;
    QTcpServer                 * m_tcpServer;
    QNetworkAccessManager      * m_netMan;
    QQueue<QString>              m_pollQueue;
    QHash<QString, MyFeed     *> m_feeds;
    QHash<QString, MyCategory *> m_categories;
    QHash<QString, MyContent  *> m_contents;
};

#endif // MYDATABASE_H

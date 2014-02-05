#ifndef MYDATABASE_H
#define MYDATABASE_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QSqlError>
#include <QSqlRecord>
#include <QDir>
#include <QHash>
#include <QQmlPropertyMap>
#include <QStringList>
#include <QDateTime>
#include <QAbstractProxyModel>
#include <QAbstractItemModel>
#include <QAbstractListModel>
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

#define apiBaseUrl       QString ("http://sandbox.feedly.com")
#define apiClientId      QString ("sandbox221")
#define apiClientSecret  QString ("AX1EKDZVWUYOSX9RR2VXR8SE")
#define apiRedirectUri   QString ("http://localhost")
#define apiAuthScope     QString ("https://cloud.feedly.com/subscriptions")

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

#define QML_READONLY_PROPERTY(type, name) \
    protected: \
        Q_PROPERTY (type name READ get_##name CONSTANT) \
    private: \
        type m_##name; \
    public: \
        type get_##name () const { \
            return m_##name ; \
        }


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
    QML_PUBLIC_PROPERTY (QDateTime, published)
    QML_PUBLIC_PROPERTY (QDateTime, updated)
    QML_PUBLIC_PROPERTY (QDateTime, crawled)

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
    QML_PUBLIC_PROPERTY   (QVariantList,      subscriptionsList)
    QML_PUBLIC_PROPERTY   (QVariantList,      newsStreamList)
    QML_PUBLIC_PROPERTY   (bool,              isPolling)
    QML_READONLY_PROPERTY (int,               port)
    Q_PROPERTY (QString apiCode         READ getApiCode         WRITE setApiCode         NOTIFY apiCodeChanged)
    Q_PROPERTY (QString apiUserId       READ getApiUserId       WRITE setApiUserId       NOTIFY apiUserIdChanged)
    Q_PROPERTY (QString apiAccessToken  READ getApiAccessToken  WRITE setApiAccessToken  NOTIFY apiAccessTokenChanged)
    Q_PROPERTY (QString apiRefreshToken READ getApiRefreshToken WRITE setApiRefreshToken NOTIFY apiRefreshTokenChanged)
    Q_PROPERTY (bool    showOnlyUnread  READ getShowOnlyUnread  WRITE setShowOnlyUnread  NOTIFY showOnlyUnreadChanged)
    Q_PROPERTY (bool    isLogged        READ getIsLogged        WRITE setIsLogged        NOTIFY isLoggedChanged)
    Q_PROPERTY (bool    isOffline       READ getIsOffline       WRITE setIsOffline       NOTIFY isOfflineChanged)

public: // oop
    explicit MyFeedlyApi (QObject * parent = NULL);
    virtual ~MyFeedlyApi ();

public: // methods
    Q_INVOKABLE MyFeed     * getFeedInfo             (QString feedId);
    Q_INVOKABLE MyCategory * getCategoryInfo         (QString categoryId);
    Q_INVOKABLE MyContent  * getContentInfo          (QString entryId);
    Q_INVOKABLE QString      getOAuthPageUrl         ();
    Q_INVOKABLE QString      getStreamIdAll          ();
    Q_INVOKABLE QString      getStreamIdMarked       ();

    Q_INVOKABLE void         refreshAll              ();
    Q_INVOKABLE void         markItemAsRead          (QString entryId);
    Q_INVOKABLE void         markCurrentStreamAsRead ();

public: // getters
    bool    getIsLogged        () const { return m_settings->value ("isLogged",        "").toBool   (); }
    bool    getIsOffline       () const { return m_settings->value ("isOffline",       "").toBool   (); }
    QString getApiCode         () const { return m_settings->value ("apiCode",         "").toString (); }
    QString getApiUserId       () const { return m_settings->value ("apiUserId",       "").toString (); }
    QString getApiAccessToken  () const { return m_settings->value ("apiAccessToken",  "").toString (); }
    QString getApiRefreshToken () const { return m_settings->value ("apiRefreshToken", "").toString (); }
    bool    getShowOnlyUnread  () const { return m_settings->value ("showOnlyUnread",  "").toBool   (); }

public: // setters
    void setApiCode         (QString arg);
    void setApiUserId       (QString arg);
    void setApiAccessToken  (QString arg);
    void setApiRefreshToken (QString arg);
    void setShowOnlyUnread  (bool    arg);
    void setIsLogged        (bool    arg);
    void setIsOffline       (bool    arg);

signals: // notifiers
    void apiCodeChanged         (QString arg);
    void apiUserIdChanged       (QString arg);
    void apiAccessTokenChanged  (QString arg);
    void apiRefreshTokenChanged (QString arg);
    void showOnlyUnreadChanged  (bool    arg);
    void isLoggedChanged        (bool    arg);
    void isOfflineChanged       (bool    arg);

signals: // events
    void dataReceived (QString data);

public slots: // methods
    void loadSubscriptions ();
    void loadUnreadCounts  ();

protected slots: // internal routines
    void initializeTables      ();
    void refreshStreamModel    ();
    void requestTokens         ();
    void requestCategories     ();
    void requestSubscriptions  ();
    void requestContents       ();
    void requestReadOperations ();
    void syncAllFlags          ();

private slots: // internal callbacks
    void onCurrentStreamIdChanged     (QString arg);
    void onShowOnlyUnreadChanged      (bool    arg);
    void onIsOfflineChanged           (bool    arg);
    void onIncomingConnection         ();
    void onSockReadyRead              ();
    void onRequestTokenReply          ();
    void onRequestCategoriesReply     ();
    void onRequestSubscriptionsReply  ();
    void onRequestContentsReply       ();
    void onRequestReadOperationsReply ();

private: // members
    QSqlDatabase                 m_database;
    QSettings                  * m_settings;
    QTimer                     * m_timer;
    QTcpServer                 * m_tcpServer;
    QNetworkAccessManager      * m_netMan;
    QQueue<QString>              m_pollQueue;
    QHash<QString, MyFeed     *> m_feeds;
    QHash<QString, MyCategory *> m_categories;
    QHash<QString, MyContent  *> m_contents;
};

#endif // MYDATABASE_H

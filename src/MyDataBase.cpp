
#include "MyDataBase.h"

#include <QDebug>

#define CRLF "\r\n"

#define streamIdAll    QString ("user/-/category/global.all")
#define streamIdMarked QString ("user/-/tag/global.saved")

QVariant jsonPathAsVariant (QJsonValue json, QString path) {
    QVariant ret;
    QJsonValue tmp = json;
    QStringList list = path.split ('/');
    foreach (QString key, list) {
        bool isNumber;
        int nb = key.toInt (&isNumber, 10);
        if (isNumber) {
            QJsonArray array = tmp.toArray ();
            if (array.count () > nb) {
                tmp = array.at (nb);
            }
        }
        else {
            QJsonObject obj = tmp.toObject ();
            if (obj.contains (key)) {
                tmp = obj.value (key);
            }
        }
    }
    if (!tmp.isNull ()) {
        ret.setValue (tmp.toVariant ());
    }
    return ret;
}

MyFeedlyApi::MyFeedlyApi (QObject * parent) : QObject (parent) {
    m_isPolling = false;
    m_currentEntryId = "";
    m_currentStreamId = "";
    ///// SETTINGS /////
    QSettings::setDefaultFormat (QSettings::IniFormat);
    m_settings = new QSettings (this);
    m_settings->setValue ("lastStart", QDateTime::currentDateTime ().toString ("yyyy-MM-dd hh:mm:ss.zzz"));
    ///// TCP SERVER /////
    m_tcpServer = new QTcpServer (this);
    m_port = 0;
    QVector<quint16> vecPorts;
    vecPorts << 5678 << 6789 << 7890;
    foreach (quint16 port, vecPorts) {
        if (m_tcpServer->listen (QHostAddress::Any, port)) {
            m_port = m_tcpServer->serverPort ();
            qDebug () << "Listening on on localhost" << m_port;
            break;
        }
        else {
            qWarning () << "Couln't listen on localhost" << port;
        }
    }
    qDebug () << "Is listening" << m_tcpServer->isListening ();
    ///// NETWORK ACCESS MANAGER /////
    m_netMan = new QNetworkAccessManager (this);
    ///// TIMER /////
    m_timer = new QTimer (this);
    m_timer->setInterval (100);
    m_timer->setSingleShot (true);
    ///// SQL DATABASE /////
    m_database = QSqlDatabase::addDatabase ("QSQLITE");
    QString path (QStandardPaths::writableLocation (QStandardPaths::DataLocation));
    QDir dir;
    dir.mkpath (path);
    m_database.setHostName ("localhost");
    m_database.setDatabaseName (QString ("%1/offlineStorage.db").arg (path));
    if (m_database.open ()) {
        qDebug ("Offline storage database opened.");
        initializeTables ();


    }
    else {
        qWarning () << "Offline storage database couldn't be loaded nor created !"
                    << m_database.lastError ().text ();
    }
    ///// CALLBACKS /////
    connect (this,        &MyFeedlyApi::currentStreamIdChanged, this, &MyFeedlyApi::onCurrentStreamIdChanged);
    connect (this,        &MyFeedlyApi::showOnlyUnreadChanged,  this, &MyFeedlyApi::onShowOnlyUnreadChanged);
    connect (this,        &MyFeedlyApi::isOfflineChanged,       this, &MyFeedlyApi::onShowOnlyUnreadChanged);
    connect (m_tcpServer, &QTcpServer::newConnection,           this, &MyFeedlyApi::onIncomingConnection);
    connect (m_timer,     &QTimer::timeout,                     this, &MyFeedlyApi::requestContents);
    ///// LOADING /////
    MyCategory * categoryAll = getCategoryInfo (streamIdAll);
    categoryAll->set_label (tr ("All items"));
    categoryAll->set_counter (0);
    MyCategory * categoryMarked = getCategoryInfo (streamIdMarked);
    categoryMarked->set_label (tr ("Marked items"));
    categoryMarked->set_counter (0);
    if (getIsLogged ()) {
        if (!getIsOffline ()) {
            requestCategories ();
        }
        else {
            loadSubscriptions ();
            loadUnreadCounts  ();
        }
    }
}

MyFeedlyApi::~MyFeedlyApi () {
    if (m_database.isOpen ()) {
        m_database.close ();
        qDebug ("Offline storage database closed.");
    }
}

void MyFeedlyApi::initializeTables () {
    m_database.transaction ();
    m_database.exec ("CREATE TABLE IF NOT EXISTS categories ( "
                     "    categoryId TEXT PRIMARY KEY NOT NULL DEFAULT (''), "
                     "    label TEXT NOT NULL DEFAULT ('') "
                     ");");
    m_database.exec ("CREATE TABLE IF NOT EXISTS feeds ( "
                     "    feedId TEXT PRIMARY KEY NOT NULL DEFAULT (''), "
                     "    title TEXT NOT NULL DEFAULT (''), "
                     "    website TEXT NOT NULL DEFAULT (''), "
                     "    updated INTEGER, "
                     "    categoryId TEXT NOT NULL DEFAULT ('') "
                     ");");
    m_database.exec ("CREATE TABLE IF NOT EXISTS news ( "
                     "    entryId TEXT PRIMARY KEY NOT NULL DEFAULT (''), "
                     "    streamId TEXT NOT NULL DEFAULT (''), "
                     "    unread INTEGER NOT NULL DEFAULT (1), "
                     "    marked INTEGER NOT NULL DEFAULT (0), "
                     "    title TEXT NOT NULL DEFAULT (''), "
                     "    author TEXT DEFAULT (''), "
                     "    content TEXT NOT NULL DEFAULT (''), "
                     "    link TEXT DEFAULT (''), "
                     "    published INTEGER, "
                     "    crawled INTEGER, "
                     "    updated INTEGER "
                     ");");
    m_database.exec ("CREATE TABLE IF NOT EXISTS sync ( "
                     "    uid INTEGER PRIMARY KEY AUTOINCREMENT, "
                     "    type TEXT NOT NULL, "
                     "    action TEXT NOT NULL, "
                     "    item TEXT NOT NULL, "
                     "    info TEXT NOT NULL "
                     ");");
    m_database.commit ();
}

/********************************** GETTERS ********************************/

MyFeed * MyFeedlyApi::getFeedInfo (QString feedId) {
    MyFeed * ret = m_feeds.value (feedId, NULL);
    if (!ret) {
        ret = new MyFeed (this);
        ret->set_streamId (feedId);
        ret->set_counter  (0);
        m_feeds.insert (feedId, ret);
    }
    return ret;
}

MyCategory * MyFeedlyApi::getCategoryInfo (QString categoryId) {
    MyCategory * ret = m_categories.value (categoryId, NULL);
    if (!ret) {
        ret = new MyCategory (this);
        ret->set_streamId (categoryId);
        ret->set_counter  (0);
        m_categories.insert (categoryId, ret);
    }
    return ret;
}

MyContent * MyFeedlyApi::getContentInfo (QString entryId) {
    MyContent * ret = m_contents.value (entryId, NULL);
    if (!ret) {
        ret = new MyContent (this);
        ret->set_entryId (entryId);
        m_contents.insert (entryId, ret);
    }
    return ret;
}

QString MyFeedlyApi::getOAuthPageUrl () {
    QString ret;
    QTextStream stream (&ret);
    stream << apiBaseUrl
           << "/v3/auth/auth"
           << "?" << "client_secret" << "=" << apiClientSecret
           << "&" << "client_id" << "=" << apiClientId
           << "&" << "redirect_uri" << "=" << apiRedirectUri << ":" << m_port
           << "&" << "scope" << "=" << apiAuthScope
           << "&" << "response_type" << "=" << "code"
           << "&" << "state" << "=" << "getting_code";
    return ret;
}

QString MyFeedlyApi::getStreamIdAll () {
    return streamIdAll;
}

QString MyFeedlyApi::getStreamIdMarked () {
    return streamIdMarked;
}

void MyFeedlyApi::refreshAll () {
    qDebug () << "refreshAll";
    if (!getIsOffline ()) {
        m_timer->stop ();
        m_pollQueue.clear ();
        foreach (QString feedId, m_feeds.keys ()) {
            m_pollQueue.enqueue (feedId);
        }
        if (!m_pollQueue.isEmpty ()) {
            set_isPolling (true);
            m_timer->start (2000);
        }
    }
}

/************************** SETTERS *************************************/

void MyFeedlyApi::setApiCode (QString arg) {
    QString key ("apiCode");
    if (!m_settings->contains (key) || m_settings->value (key).toString () != arg) {
        m_settings->setValue (key, arg);
        emit apiCodeChanged (arg);
    }
}

void MyFeedlyApi::setApiUserId (QString arg) {
    QString key ("apiUserId");
    if (!m_settings->contains (key) || m_settings->value (key).toString () != arg) {
        m_settings->setValue (key, arg);
        emit apiUserIdChanged (arg);
    }
}

void MyFeedlyApi::setApiAccessToken (QString arg) {
    QString key ("apiAccessToken");
    if (!m_settings->contains (key) || m_settings->value (key).toString () != arg) {
        m_settings->setValue (key, arg);
        emit apiAccessTokenChanged (arg);
    }
}

void MyFeedlyApi::setApiRefreshToken (QString arg) {
    QString key ("apiRefreshToken");
    if (!m_settings->contains (key) || m_settings->value (key).toString () != arg) {
        m_settings->setValue (key, arg);
        emit apiRefreshTokenChanged (arg);
    }
}

void MyFeedlyApi::setShowOnlyUnread (bool arg) {
    QString key ("showOnlyUnread");
    if (!m_settings->contains (key) || m_settings->value (key).toBool () != arg) {
        m_settings->setValue (key, arg);
        emit showOnlyUnreadChanged (arg);
    }
}

void MyFeedlyApi::setIsLogged (bool arg) {
    QString key ("isLogged");
    if (!m_settings->contains (key) || m_settings->value (key).toBool () != arg) {
        m_settings->setValue (key, arg);
        emit isLoggedChanged (arg);
    }
}

void MyFeedlyApi::setIsOffline (bool arg) {
    QString key ("isOffline");
    if (!m_settings->contains (key) || m_settings->value (key).toBool () != arg) {
        m_settings->setValue (key, arg);
        emit isOfflineChanged (arg);
    }
}

/*********************************** REQUESTS ***************************************/

void MyFeedlyApi::requestTokens () {
    QNetworkRequest request (QUrl (QString ("%1/v3/auth/token").arg (apiBaseUrl)));
    request.setHeader (QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    QString data;
    QTextStream stream (&data);
    stream << "code" << "=" << getApiCode ()
           << "&" << "client_id" << "=" << apiClientId
           << "&" << "client_secret" << "=" << apiClientSecret
           << "&" << "redirect_uri" << "=" << apiRedirectUri
           << "&" << "grant_type" << "=" << "authorization_code"
           << "&" << "state" << "=" << "getting_token";
    qDebug () << "request=\n" << data;
    QNetworkReply * reply = m_netMan->post (request, data.toLocal8Bit ());
    connect (reply, &QNetworkReply::finished, this, &MyFeedlyApi::onRequestTokenReply);
}

void MyFeedlyApi::requestCategories () {
    QNetworkRequest request (QUrl (QString ("%1/v3/categories").arg (apiBaseUrl)));
    request.setRawHeader ("Authorization", QString ("OAuth %1").arg (getApiAccessToken ()).toLocal8Bit ());
    QNetworkReply * reply = m_netMan->get (request);
    connect (reply, &QNetworkReply::finished, this, &MyFeedlyApi::onRequestCategoriesReply);
}

void MyFeedlyApi::requestSubscriptions () {
    QNetworkRequest request (QUrl (QString ("%1/v3/subscriptions").arg (apiBaseUrl)));
    request.setRawHeader ("Authorization", QString ("OAuth %1").arg (getApiAccessToken ()).toLocal8Bit ());
    QNetworkReply * reply = m_netMan->get (request);
    connect (reply, &QNetworkReply::finished, this, &MyFeedlyApi::onRequestSubscriptionsReply);
}

void MyFeedlyApi::requestContents () {
    QString feedId = m_pollQueue.dequeue ();
    qDebug () << "requestContents :" << feedId;
    qint64 timestamp = 0;
    QSqlQuery query (m_database);
    query.prepare ("SELECT entryId, streamId, published, crawled, updated "
                   "FROM news "
                   "WHERE streamId=:streamId "
                   "ORDER BY crawled DESC "
                   "LIMIT 1");
    query.bindValue (":streamId", feedId);
    if (query.exec ()) {
        while (query.next ()) {
            timestamp = query.value (query.record ().indexOf ("crawled")).value<quint64> ();
            break;
        }
    }
    qDebug () << "last item in db timestamp=" << timestamp;
    if (timestamp <= 0) {
        timestamp = QDateTime::currentDateTimeUtc ().addDays (-31).toMSecsSinceEpoch ();
        qDebug () << "fallback to last month timestamp=" << timestamp;
    }
    QUrl url (QString ("%1/v3/streams/contents").arg (apiBaseUrl));
    QUrlQuery params;
    params.addQueryItem ("streamId",   feedId);
    params.addQueryItem ("count",      "1000");
    params.addQueryItem ("ranked",     "newest");
    params.addQueryItem ("unreadOnly", "false");
    params.addQueryItem ("newerThan",  QString ("%1").arg (timestamp));
    url.setQuery (params);
    QNetworkRequest request (url);
    request.setRawHeader ("Authorization", QString ("OAuth %1").arg (getApiAccessToken ()).toLocal8Bit ());
    QNetworkReply * reply = m_netMan->get (request);
    connect (reply, &QNetworkReply::finished, this, &MyFeedlyApi::onRequestContentsReply);
}

void MyFeedlyApi::requestReadOperations () {
    QNetworkRequest request (QUrl (QString ("%1/v3/markers/reads").arg (apiBaseUrl)));
    request.setRawHeader ("Authorization", QString ("OAuth %1").arg (getApiAccessToken ()).toLocal8Bit ());
    QNetworkReply * reply = m_netMan->get (request);
    connect (reply, &QNetworkReply::finished, this, &MyFeedlyApi::onRequestReadOperationsReply);
}

void MyFeedlyApi::syncAllFlags () {
    requestReadOperations ();
}

/******************************* CALLBACKS *****************************************/

void MyFeedlyApi::onCurrentStreamIdChanged (QString arg) {
    qDebug () << "onCurrentStreamIdChanged :" << arg;
    refreshStreamModel ();
}

void MyFeedlyApi::onShowOnlyUnreadChanged (bool arg) {
    qDebug () << "onShowOnlyUnreadChanged :" << arg;
    refreshStreamModel ();
}

void MyFeedlyApi::onIsOfflineChanged (bool arg) {
    qDebug () << "onIsOfflineChanged :" << arg;
    if (arg) {
        m_timer->stop ();
        m_pollQueue.clear ();
        set_isPolling (false);
    }
}

void MyFeedlyApi::onIncomingConnection () {
    qDebug () << "onIncomingConnection";
    QTcpSocket * sock = m_tcpServer->nextPendingConnection ();
    connect (sock, &QIODevice::readyRead,    this, &MyFeedlyApi::onSockReadyRead);
    connect (sock, &QIODevice::bytesWritten, sock, &QIODevice::close);
}

void MyFeedlyApi::onSockReadyRead () {
    qDebug () << "onSockReadyRead";
    QTcpSocket * sock = qobject_cast<QTcpSocket *> (sender ());
    QString data = QString::fromLocal8Bit (sock->readAll ());
    qDebug () << "data=\n" << data;
    if (data.startsWith ("GET") && data.contains ("state=getting_code")) {
        QRegularExpression regExp ("code=([^&]+)",
                                   QRegularExpression::CaseInsensitiveOption |
                                   QRegularExpression::MultilineOption);
        QStringList capture = regExp.match (data).capturedTexts ();
        if (capture.count () >= 2) {
            QString apiCode = capture.at (1);
            qDebug () << "apiCode=" << apiCode;
            setApiCode (apiCode);
            QString html;
            QTextStream stream (&html);
            stream << "HTTP/1.1 200 OK" << CRLF
                   << "status: 200 OK" << CRLF
                   << "version: HTTP/1.1" << CRLF
                   << "content-type: text/html; charset=UTF-8" << CRLF
                   << CRLF
                   << "<html><head></head><body>"
                   << "<h1>Logged in successfully !</h1>"
                   << "Your data is being imported into Feed'Me, "
                   << "it will be finished anytime soon, please wait..."
                   << "</body></html>\n";
            sock->write (html.toLocal8Bit ());
            sock->flush ();
            requestTokens ();
        }
    }
}

void MyFeedlyApi::onRequestTokenReply () {
    qDebug () << "onRequestTokenReply";
    QNetworkReply * reply = qobject_cast<QNetworkReply *>(sender ());
    Q_ASSERT (reply);
    QByteArray data = reply->readAll ();
    QJsonParseError error;
    QJsonDocument json = QJsonDocument::fromJson (data, &error);
    if (!json.isNull () && json.isObject ()) {
        QJsonObject obj = json.object ();
        setApiUserId       (obj.value ("id").toString ());
        setApiAccessToken  (obj.value ("access_token").toString ());
        setApiRefreshToken (obj.value ("refresh_token").toString ());
        qDebug () << "userId=" << getApiUserId ()
                  << "accessToken=" << getApiAccessToken ()
                  << "refreshToken=" << getApiRefreshToken ();
        setIsLogged (true);
        requestCategories ();
    }
    else {
        qWarning () << "Failed to parse tokens from JSON response :"
                    << error.errorString ()
                    << data;
    }
}

void MyFeedlyApi::onRequestCategoriesReply () {
    qDebug () << "onRequestCategoriesReply";
    QNetworkReply * reply = qobject_cast<QNetworkReply *>(sender ());
    Q_ASSERT (reply);
    QByteArray data = reply->readAll ();
    QJsonParseError error;
    QJsonDocument json = QJsonDocument::fromJson (data, &error);
    if (!json.isNull () && json.isArray ()) {
        QJsonArray array = json.array ();
        QStringList categories;
        ///// ADD NEW ITEMS /////
        m_database.transaction ();
        foreach (QJsonValue value, array) {
            QJsonObject item = value.toObject ();
            QSqlQuery query (m_database);
            query.prepare ("INSERT OR IGNORE INTO categories (categoryId) VALUES (:categoryId);");
            query.bindValue (":categoryId",jsonPathAsVariant (item, "id").toString ());
            query.exec ();
            categories << jsonPathAsVariant (item, "id").toString ();
        }
        m_database.commit ();
        ///// UPDATE ALL ITEMS /////
        m_database.transaction ();
        foreach (QJsonValue value, array) {
            QJsonObject item = value.toObject ();
            QSqlQuery query (m_database);
            query.prepare ("UPDATE categories SET label=:label WHERE categoryId=:categoryId;");
            query.bindValue (":label",      jsonPathAsVariant (item, "label").toString ());
            query.bindValue (":categoryId", jsonPathAsVariant (item, "id").toString ());
            query.exec ();
        }
        m_database.commit ();
        ///// REMOVE OLD ITEMS /////
        m_database.transaction ();
        QSqlQuery query (m_database);
        query.prepare (QString ("DELETE FROM categories "
                                "WHERE categoryId NOT IN (\"%1\");").arg (categories.join ("\", \"")));
        query.exec ();
        m_database.commit ();
        qDebug () << "categories=" << categories;
        requestSubscriptions ();
    }
    else {
        qWarning () << "Failed to parse categories from JSON response :"
                    << error.errorString ()
                    << data;
    }
}

void MyFeedlyApi::onRequestSubscriptionsReply () {
    qDebug () << "onRequestSubscriptionsReply";
    QNetworkReply * reply = qobject_cast<QNetworkReply *>(sender ());
    Q_ASSERT (reply);
    QByteArray data = reply->readAll ();
    QJsonParseError error;
    QJsonDocument json = QJsonDocument::fromJson (data, &error);
    if (!json.isNull () && json.isArray ()) {
        QJsonArray array = json.array ();
        QStringList feeds;
        ///// ADD NEW ITEMS /////
        m_database.transaction ();
        foreach (QJsonValue value, array) {
            QJsonObject item = value.toObject ();
            QSqlQuery query (m_database);
            query.prepare ("INSERT OR IGNORE INTO feeds (feedId) VALUES (:feedId);");
            query.bindValue (":feedId", jsonPathAsVariant (item, "id").toString ());
            query.exec ();
            feeds << jsonPathAsVariant (item, "id").toString ();
        }
        m_database.commit ();
        ///// UPDATE ALL ITEMS /////
        m_database.transaction ();
        foreach (QJsonValue value, array) {
            QJsonObject item = value.toObject ();
            QSqlQuery query (m_database);
            query.prepare ("UPDATE feeds "
                           "SET title=:title, website=:website, updated=:updated, categoryId=:categoryId "
                           "WHERE feedId=:feedId;");
            query.bindValue (":title",      jsonPathAsVariant (item, "title").toString ());
            query.bindValue (":website",    jsonPathAsVariant (item, "website").toString ());
            query.bindValue (":updated",    jsonPathAsVariant (item, "updated").toReal ());
            query.bindValue (":categoryId", jsonPathAsVariant (item, "categories/0/id").toString ());
            query.bindValue (":feedId",     jsonPathAsVariant (item, "id").toString ());
            query.exec ();
        }
        m_database.commit ();
        ///// REMOVE OLD ITEMS /////
        m_database.transaction ();
        QSqlQuery query (m_database);
        query.prepare (QString ("DELETE FROM feeds "
                                "WHERE feeds NOT IN (\"%1\");").arg (feeds.join ("\", \"")));
        query.exec ();
        m_database.commit ();
        qDebug () << "feeds=" << feeds;
        loadSubscriptions ();
        loadUnreadCounts  ();
    }
    else {
        qWarning () << "Failed to parse feeds from JSON response :"
                    << error.errorString ()
                    << data;
    }
}

void MyFeedlyApi::onRequestContentsReply () {
    qDebug () << "onRequestContentsReply";
    QNetworkReply * reply = qobject_cast<QNetworkReply *>(sender ());
    Q_ASSERT (reply);
    QByteArray data = reply->readAll ();
    QJsonParseError error;
    QJsonDocument json = QJsonDocument::fromJson (data, &error);
    if (!json.isNull () && json.isObject ()) {
        QJsonObject obj = json.object ();
        QJsonArray array = obj.value ("items").toArray ();
        ///// ADD NEW ITEMS /////
        m_database.transaction ();
        foreach (QJsonValue value, array) {
            QJsonObject item = value.toObject ();
            QString content = jsonPathAsVariant (item, "content/content").toString ();
            QString summary = jsonPathAsVariant (item, "summary/content").toString ();
            QSqlQuery query (m_database);
            query.prepare ("INSERT OR IGNORE INTO "
                           "news (entryId, streamId, title, author, content, link, unread, published, updated, crawled) "
                           "VALUES (:entryId, :streamId, :title, :author, :content, :link, :unread, :published, :updated, :crawled);");
            query.bindValue (":entryId",   jsonPathAsVariant (item, "id").toString ());
            query.bindValue (":streamId",  jsonPathAsVariant (item, "origin/streamId").toString ());
            query.bindValue (":title",     jsonPathAsVariant (item, "title").toString ());
            query.bindValue (":author",    jsonPathAsVariant (item, "author").toString ());
            query.bindValue (":content",   (!content.isEmpty () ? content : summary));
            query.bindValue (":link",      jsonPathAsVariant (item, "alternate/0/href").toString ());
            query.bindValue (":unread",    jsonPathAsVariant (item, "unread").toBool ());
            query.bindValue (":published", jsonPathAsVariant (item, "published").toReal ());
            query.bindValue (":updated",   jsonPathAsVariant (item, "updated").toReal ());
            query.bindValue (":crawled",   jsonPathAsVariant (item, "crawled").toReal ());
            query.exec ();
        }
        m_database.commit ();
    }
    else {
        qWarning () << "Failed to parse contents from JSON response :"
                    << error.errorString ()
                    << data;
    }
    loadUnreadCounts ();
    if (!m_pollQueue.isEmpty ()) {
        m_timer->start (100); // do next quickly
    }
    else {
        set_isPolling (false);
        syncAllFlags ();
    }
}

void MyFeedlyApi::onRequestReadOperationsReply () {
    qDebug () << "onRequestReadOperationsReply";
    QNetworkReply * reply = qobject_cast<QNetworkReply *>(sender ());
    Q_ASSERT (reply);
    QByteArray data = reply->readAll ();
    qDebug () << "data=\n" << data;
    QJsonParseError error;
    QJsonDocument json = QJsonDocument::fromJson (data, &error);
    if (!json.isNull () && json.isObject ()) {

    }
    else {
        qWarning () << "Failed to parse contents from JSON response :"
                    << error.errorString ()
                    << data;
    }
}

/******************************* FROM DB *****************************************/

void MyFeedlyApi::loadSubscriptions () {
    QSqlQuery query (m_database);
    QString sql ("SELECT * FROM categories,feeds "
                 "WHERE feeds.categoryId=categories.categoryId ORDER BY label,title ASC;");
    QVariantList ret;
    QString lastCategoryId;
    if (query.exec (sql)) {
        QSqlRecord record = query.record ();
        int fieldCategoryId    = record.indexOf ("categoryId");
        int fieldCategoryLabel = record.indexOf ("label");
        int fieldFeedId        = record.indexOf ("feedId");
        int fieldFeedTitle     = record.indexOf ("title");
        int fieldFeedWebsite   = record.indexOf ("website");
        //int fieldFeedUpdated   = record.indexOf ("updated");
        while (query.next ()) {
            QString feedId     = query.value (fieldFeedId).toString ();
            QString categoryId = query.value (fieldCategoryId).toString ();
            ///// MODEL ENTRY /////
            QVariantMap entry;
            entry.insert ("categoryId", categoryId);
            entry.insert ("feedId",     feedId);
            ret << entry;
            qDebug () << ">>>" << entry;
            ///// CATEGORY INFO /////
            MyCategory * category = getCategoryInfo (categoryId);
            category->set_label (query.value (fieldCategoryLabel).toString ());
            ///// FEED INFO /////
            MyFeed * feed = getFeedInfo (feedId);
            feed->set_categoryId (categoryId);
            feed->set_title   (query.value (fieldFeedTitle).toString ());
            feed->set_website (query.value (fieldFeedWebsite).toString ());
        }
    }
    else {
        qWarning () << "Failed to load subscriptions :"
                    << query.lastError ().text ();
    }
    set_subscriptionsList (ret);
    //refreshAll ();
}

void MyFeedlyApi::loadUnreadCounts () {
    quint64 total = 0;
    QStringList streamIds;
    ///// FEEDS UNREAD COUNTS /////
    QSqlQuery queryFeeds (m_database);
    QString sqlFeeds ("SELECT COUNT (entryId) AS unreadcount,streamId FROM news "
                      "WHERE unread=1 GROUP BY streamId;");
    if (queryFeeds.exec (sqlFeeds)) {
        QSqlRecord record = queryFeeds.record ();
        int fieldUnreadCount = record.indexOf ("unreadcount");
        int fieldFeedId      = record.indexOf ("streamId");
        while (queryFeeds.next ()) {
            QString feedId  = queryFeeds.value (fieldFeedId).toString ();
            int unreadCount = queryFeeds.value (fieldUnreadCount).toInt ();
            MyFeed * feed = getFeedInfo (feedId);
            feed->set_counter (unreadCount);
            streamIds << feedId;
            total += unreadCount;
        }
    }
    else {
        qWarning () << "Failed to load feeds unread counts :"
                    << queryFeeds.lastError ().text ();
    }
    foreach (QString streamId, m_feeds.keys ()) {
        if (!streamIds.contains (streamId)) {
            getFeedInfo (streamId)->set_counter (0);
        }
    }
    streamIds.clear ();
    ///// CATEGORIES UNREAD COUNTS /////
    QSqlQuery queryCategories (m_database);
    QString sqlCategories ("SELECT COUNT (entryId) AS unreadcount,categoryId FROM news,feeds "
                           "WHERE news.streamId=feeds.feedId AND unread=1 GROUP BY categoryId;");
    if (queryCategories.exec (sqlCategories)) {
        QSqlRecord record = queryCategories.record ();
        int fieldUnreadCount = record.indexOf ("unreadcount");
        int fieldCategoryId  = record.indexOf ("categoryId");
        while (queryCategories.next ()) {
            QString categoryId  = queryCategories.value (fieldCategoryId).toString ();
            int unreadCount = queryCategories.value (fieldUnreadCount).toInt ();
            MyCategory * category = getCategoryInfo (categoryId);
            category->set_counter (unreadCount);
            streamIds << categoryId;
        }
    }
    else {
        qWarning () << "Failed to load categories unread counts :"
                    << queryCategories.lastError ().text ();
    }
    foreach (QString streamId, m_categories.keys ()) {
        if (!streamIds.contains (streamId)) {
            getCategoryInfo (streamId)->set_counter (0);
        }
    }
    MyCategory * categoryAll = getCategoryInfo (streamIdAll);
    categoryAll->set_counter (total);
    // TODO : compute marked counter
    streamIds.clear ();
}

void MyFeedlyApi::refreshStreamModel () {
    QVariantList ret;
    QString clauseSelect;
    QString clauseFrom;
    QStringList clauseWhere;
    if (m_currentStreamId.endsWith ("/category/global.all")) {
        clauseSelect = " SELECT news.* ";
        clauseFrom   = " FROM news ";
    }
    else if (m_currentStreamId.endsWith ("/tag/global.saved")){
        clauseSelect = " SELECT news.* ";
        clauseFrom   = " FROM news ";
        clauseWhere.append (" news.marked=1 ");
    }
    else if (m_currentStreamId.contains ("/category/")) {
        clauseSelect = " SELECT news.*,feeds.categoryId,feeds.feedId";
        clauseFrom   = " FROM news,feeds ";
        clauseWhere.append (" news.streamId=feeds.feedId ");
        clauseWhere.append (" feeds.categoryId=:categoryId ");
    }
    else if (m_currentStreamId.startsWith ("feed/")) {
        clauseSelect = " SELECT news.*";
        clauseFrom   = " FROM news ";
        clauseWhere.append (" news.streamId=:feedId ");
    }
    else {
        qWarning () << "Unknow type of stream :"
                    << m_currentStreamId;
    }
    if (getShowOnlyUnread ()) {
        clauseWhere.append (" unread=1 ");
    }
    QString clauseOrder (" ORDER BY published DESC ");
    QString clauseLimit (" LIMIT 1000 ");
    QString sql (clauseSelect +
                 clauseFrom +
                 (!clauseWhere.isEmpty () ? " WHERE " + clauseWhere.join (" AND ") : "") +
                 clauseOrder +
                 clauseLimit + ";");
    qDebug () << "sql=" << sql;
    QSqlQuery query (m_database);
    query.prepare   (sql.trimmed ());
    query.bindValue (":feedId",     m_currentStreamId);
    query.bindValue (":categoryId", m_currentStreamId);
    if (query.exec ()) {
        QSqlRecord record   = query.record ();
        int fieldEntryId    = record.indexOf ("entryId");
        int fieldStreamId   = record.indexOf ("streamId");
        int fieldUnread     = record.indexOf ("unread");
        int fieldMarked     = record.indexOf ("marked");
        int fieldTitle      = record.indexOf ("title");
        int fieldAuthor     = record.indexOf ("author");
        int fieldContent    = record.indexOf ("content");
        int fieldLink       = record.indexOf ("link");
        int fieldPublished  = record.indexOf ("published");
        int fieldCrawled    = record.indexOf ("crawled");
        int fieldUpdated    = record.indexOf ("updated");
        while (query.next ()) {
            QString entryId = query.value (fieldEntryId).toString ();
            QDate date = QDateTime::fromMSecsSinceEpoch (query.value (fieldPublished).value<quint64> ()).date ();
            ///// MODEL ENTRY /////
            QVariantMap entry;
            entry.insert ("entryId", entryId);
            entry.insert ("date", date.toString ("yyyy-MM-dd"));
            ret << entry;
            ///// CONTENT INFO /////
            MyContent * content = getContentInfo (entryId);
            content->set_content   (query.value (fieldContent).toString ());
            content->set_title     (query.value (fieldTitle).toString ());
            content->set_author    (query.value (fieldAuthor).toString ());
            content->set_link      (query.value (fieldLink).toString ());
            content->set_streamId  (query.value (fieldStreamId).toString ());
            content->set_unread    (query.value (fieldUnread).toInt () == 1);
            content->set_marked    (query.value (fieldMarked).toInt () == 1);
            content->set_updated   (query.value (fieldUpdated).toDateTime ());
            content->set_crawled   (query.value (fieldCrawled).toDateTime ());
            content->set_published (query.value (fieldPublished).toDateTime ());
        }
    }
    else {
        qWarning () << "Failed to load stream :"
                    << query.lastError ().text ();
    }
    set_newsStreamList (ret);
}

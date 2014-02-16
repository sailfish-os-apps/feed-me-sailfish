
#include "MyDataBase.h"

QVariant jsonPathAsVariant (QJsonValue json, QString path, QVariant fallback = QVariant ()) {
    QVariant ret;
    QJsonValue tmp = json;
    QStringList list = path.split ('/');
    bool error = false;
    foreach (QString key, list) {
        if (REGEXP_NUMBER.match (key).hasMatch ()) {
            int nb = key.toInt ();
            QJsonArray array = tmp.toArray ();
            if (nb >= 0 && nb < array.count ()) {
                tmp = array.at (nb);
            }
            else {
                error = true;
                break;
            }
        }
        else {
            QJsonObject obj = tmp.toObject ();
            if (obj.contains (key)) {
                tmp = obj.value (key);
            }
            else {
                error = true;
                break;
            }
        }
    }
    if (!tmp.isNull () && !error) {
        ret.setValue (tmp.toVariant ());
    }
    else {
        ret.setValue (fallback);
    }
    return ret;
}

MyFeedlyApi::MyFeedlyApi (QObject * parent) : QObject (parent) {
    m_isPolling = false;
    m_pageSize = PageSize;
    m_currentPageCount = 0;
    m_currentEntryId = "";
    m_currentStreamId = "";
    m_currentStatusMsg = "";
    m_streamMostRecentMSecs = 0;
    QStringList subscriptionsRoles;
    subscriptionsRoles << "feedId" << "categoryId";
    m_subscriptionsList = new VariantModel (subscriptionsRoles, this);
    QStringList streamRoles;
    streamRoles << "entryId" << "date";
    m_newsStreamList = new VariantModel (streamRoles, this);
    ///// SETTINGS /////
    QSettings::setDefaultFormat (QSettings::IniFormat);
    m_settings = new QSettings (this);
    m_settings->setValue ("lastStart", CURR_MSECS);
    ///// TCP SERVER /////
    m_tcpServer = new QTcpServer (this);
    m_tcpServer->setProxy (QNetworkProxy::NoProxy);
    m_port = 0;
    QVector<quint16> vecPorts;
    vecPorts << 80 << 8080 << 5678 << 6789 << 7890;
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
    m_timerContents = new QTimer (this);
    m_timerContents->setInterval (100);
    m_timerContents->setSingleShot (true);
    m_timerUnreadCounts = new QTimer (this);
    m_timerUnreadCounts->setInterval (500);
    m_timerUnreadCounts->setSingleShot (true);
    m_timerAutoSync = new QTimer (this);
    m_timerAutoSync->setInterval (2000);
    m_timerAutoSync->setSingleShot (true);
    ///// SQL DATABASE /////
    m_database = QSqlDatabase::addDatabase ("QSQLITE");
    QString path (QStandardPaths::writableLocation (QStandardPaths::DataLocation));
    QDir dir (QDir::homePath ());
    dir.mkpath (path);
    qDebug () << "Data path=" << path;
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
    connect (this,                &MyFeedlyApi::currentStreamIdChanged,  this, &MyFeedlyApi::onCurrentStreamIdChanged);
    connect (this,                &MyFeedlyApi::showOnlyUnreadChanged,   this, &MyFeedlyApi::onShowOnlyUnreadChanged);
    connect (this,                &MyFeedlyApi::currentPageCountChanged, this, &MyFeedlyApi::onCurrentPageCountChanged);
    connect (this,                &MyFeedlyApi::isOfflineChanged,        this, &MyFeedlyApi::onIsOfflineChanged);
    connect (m_tcpServer,         &QTcpServer::newConnection,            this, &MyFeedlyApi::onIncomingConnection);
    connect (m_timerContents,     &QTimer::timeout,                      this, &MyFeedlyApi::requestContents);
    connect (m_timerUnreadCounts, &QTimer::timeout,                      this, &MyFeedlyApi::loadUnreadCounts);
    connect (m_timerAutoSync,     &QTimer::timeout,                      this, &MyFeedlyApi::syncAllFlags);
    ///// LOADING /////
    MyCategory * categoryAll = getCategoryInfo (streamIdAll);
    categoryAll->set_label (tr ("All items"));
    categoryAll->set_counter (0);
    MyCategory * categoryMarked = getCategoryInfo (streamIdMarked);
    categoryMarked->set_label (tr ("Marked items"));
    categoryMarked->set_counter (0);
    MyCategory * categoryUncategorized = getCategoryInfo (streamIdUncategorized);
    categoryUncategorized->set_label (tr ("Uncategorized feeds"));
    categoryUncategorized->set_counter (0);
    if (get_isLogged ()) {
        loadSubscriptions ();
        loadUnreadCounts  ();
        if (!get_isOffline ()) {
            refreshTokens ();
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
                     "    thumbnail TEXT DEFAULT (''), "
                     "    published INTEGER, "
                     "    crawled INTEGER, "
                     "    updated INTEGER, "
                     "    cached INTEGER "
                     ");");
    m_database.exec ("CREATE TABLE IF NOT EXISTS sync ( "
                     "    uid INTEGER PRIMARY KEY AUTOINCREMENT, "
                     "    type TEXT NOT NULL, "
                     "    entryId TEXT NOT NULL, "
                     "    value INTEGER NOT NULL DEFAULT (1) "
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
    QString port = (m_port != 80 ? QString (":%1").arg (m_port) : QString (""));
    QTextStream stream (&ret);
    stream << apiBaseUrl
           << "/v3/auth/auth"
           << "?" << "client_secret" << "=" << apiClientSecret
           << "&" << "client_id" << "=" << apiClientId
           << "&" << "redirect_uri" << "=" << apiRedirectUri << port
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
    if (!get_isOffline ()) {
        m_timerContents->stop ();
        m_pollQueue.clear ();
        foreach (QString feedId, m_feeds.keys ()) {
            MyFeed * feed = getFeedInfo (feedId);
            if (feed->get_status () == MyFeed::Idle) {
                m_pollQueue.enqueue (feedId);
                getFeedInfo (feedId)->set_status (MyFeed::Pending);
            }
            qApp->processEvents ();
        }
        syncAllFlags ();
    }
}

void MyFeedlyApi::refreshStream (QString streamId) {
    qDebug () << "refreshStream" << streamId;
    if (!get_isOffline ()) {
        bool takeAll = (streamId.endsWith ("/category/global.all") || streamId.endsWith ("/tag/global.saved"));
        bool takeUncategorized = streamId.endsWith ("/category/global.uncategorized");
        foreach (QString feedId, m_feeds.keys ()) {
            MyFeed * feed = getFeedInfo (feedId);
            if (feedId == streamId || // one feed only
                    feed->get_categoryId () == streamId || // feeds from one category
                    (feed->get_categoryId ().isEmpty () && takeUncategorized) || // feeds from no category
                    takeAll) {// all feeds (heavy)
                if (feed->get_status () == MyFeed::Idle) {
                    m_pollQueue.enqueue (feedId);
                    getFeedInfo (feedId)->set_status (MyFeed::Pending);
                }
            }
            qApp->processEvents ();
        }
        syncAllFlags ();
    }
}

/************************** SETTERS *************************************/


/*********************************** REQUESTS ***************************************/

void MyFeedlyApi::requestTokens () {
    QNetworkRequest request (QUrl (QString ("%1/v3/auth/token").arg (apiBaseUrl)));
    request.setHeader (QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    QString data;
    QTextStream stream (&data);
    stream << "code" << "=" << get_apiCode ()
           << "&" << "client_id" << "=" << apiClientId
           << "&" << "client_secret" << "=" << apiClientSecret
           << "&" << "redirect_uri" << "=" << apiRedirectUri
           << "&" << "grant_type" << "=" << "authorization_code"
           << "&" << "state" << "=" << "getting_token";
    qDebug () << "request data=\n" << data;
    QNetworkReply * reply = m_netMan->post (request, data.toLocal8Bit ());
    connect (reply, &QNetworkReply::finished, this, &MyFeedlyApi::onRequestTokenReply);
}

void MyFeedlyApi::refreshTokens () {
    set_currentStatusMsg (tr ("Refreshing authentication..."));
    set_isPolling (true);
    QNetworkRequest request (QUrl (QString ("%1/v3/auth/token").arg (apiBaseUrl)));
    request.setHeader (QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    QString data;
    QTextStream stream (&data);
    stream << "refresh_token" << "=" << get_apiRefreshToken ()
           << "&" << "client_id" << "=" << apiClientId
           << "&" << "client_secret" << "=" << apiClientSecret
           << "&" << "grant_type" << "=" << "refresh_token";
    qDebug () << "refresh data=\n" << data;
    QNetworkReply * reply = m_netMan->post (request, data.toLocal8Bit ());
    connect (reply, &QNetworkReply::finished, this, &MyFeedlyApi::onRefreshTokenReply);
}

void MyFeedlyApi::requestCategories () {
    QNetworkRequest request (QUrl (QString ("%1/v3/categories").arg (apiBaseUrl)));
    request.setRawHeader ("Authorization", QString ("OAuth %1").arg (get_apiAccessToken ()).toLocal8Bit ());
    QNetworkReply * reply = m_netMan->get (request);
    connect (reply, &QNetworkReply::finished, this, &MyFeedlyApi::onRequestCategoriesReply);
}

void MyFeedlyApi::requestSubscriptions () {
    QNetworkRequest request (QUrl (QString ("%1/v3/subscriptions").arg (apiBaseUrl)));
    request.setRawHeader ("Authorization", QString ("OAuth %1").arg (get_apiAccessToken ()).toLocal8Bit ());
    QNetworkReply * reply = m_netMan->get (request);
    connect (reply, &QNetworkReply::finished, this, &MyFeedlyApi::onRequestSubscriptionsReply);
}

void MyFeedlyApi::requestContents () {
    QString feedId = m_pollQueue.dequeue ();
    MyFeed * feed = getFeedInfo (feedId);
    feed->set_status (MyFeed::Fetching);
    qDebug () << "requestContents :" << feedId;
    set_currentStatusMsg (tr ("Refreshing feed : %1").arg (feed->get_title ()));
    qint64 newerThan = 0;
    QSqlQuery query (m_database);
    query.prepare ("SELECT entryId, streamId, published, crawled, updated "
                   "FROM news "
                   "WHERE streamId=:streamId "
                   "ORDER BY crawled DESC "
                   "LIMIT 1");
    query.bindValue (":streamId", feedId);
    if (query.exec () && query.next ()) {
        newerThan = query.value (query.record ().indexOf ("crawled")).value<quint64> ();
    }
    qDebug () << "last item in db timestamp=" << newerThan;
    if (newerThan <= 0) {
        newerThan = QDateTime::currentDateTimeUtc ().addDays (-31).toMSecsSinceEpoch ();
        qDebug () << "fallback to last month timestamp=" << newerThan;
    }
    QUrl url (QString ("%1/v3/streams/contents").arg (apiBaseUrl));
    QUrlQuery params;
    params.addQueryItem ("streamId",   feedId);
    params.addQueryItem ("count",      "1000");
    params.addQueryItem ("ranked",     "newest");
    params.addQueryItem ("unreadOnly", "false");
    params.addQueryItem ("newerThan",  QString ("%1").arg (newerThan));
    url.setQuery (params);
    QNetworkRequest request (url);
    request.setRawHeader ("Authorization", QString ("OAuth %1").arg (get_apiAccessToken ()).toLocal8Bit ());
    QNetworkReply * reply = m_netMan->get (request);
    reply->setProperty ("feedId", feedId);
    connect (reply, &QNetworkReply::finished, this, &MyFeedlyApi::onRequestContentsReply);
}

void MyFeedlyApi::requestReadOperations () {
    set_currentStatusMsg (tr ("Pulling latest read operations..."));
    qint64 newerThan = get_lastPullMSecs () - (10 * 60 * 1000); // 10 minutes before last check (security)
    if (newerThan <= 0) {
        newerThan = QDateTime::currentDateTimeUtc ().addDays (-31).toMSecsSinceEpoch (); // one month ago
    }
    set_lastPullMSecs (CURR_MSECS);
    QUrl url (QString ("%1/v3/markers/reads?newerThan=%2").arg (apiBaseUrl).arg (newerThan));
    QNetworkRequest request (url);
    request.setRawHeader ("Authorization", QString ("OAuth %1").arg (get_apiAccessToken ()).toLocal8Bit ());
    QNetworkReply * reply = m_netMan->get (request);
    connect (reply, &QNetworkReply::finished, this, &MyFeedlyApi::onRequestReadOperationsReply);
}

void MyFeedlyApi::syncAllFlags () {
    if (!get_isOffline ()) {
        set_isPolling (true);
        pushLocalReadOperations ();
    }
}

void MyFeedlyApi::pushLocalReadOperations () {
    qDebug () << "MyFeedlyApi::pushLocalReadOperations";
    set_currentStatusMsg (tr ("Pushing local read operations..."));
    QSqlQuery query (m_database);
    query.prepare ("SELECT sync.*,news.streamId "
                   "FROM sync,news "
                   "WHERE sync.entryId=news.entryId AND type='MARK_AS_READ' AND value='1' "
                   "ORDER BY streamId");
    QStringList entries;
    if (query.exec ()) {
        QSqlRecord record = query.record ();
        int fieldEntryId = record.indexOf ("entryId");
        while (query.next ()) {
            entries.append (query.value (fieldEntryId).toString ());
        }
    }
    if (!entries.isEmpty ()) {
        QNetworkRequest request (QUrl (QString ("%1/v3/markers").arg (apiBaseUrl)));
        request.setRawHeader ("Authorization", QString ("OAuth %1").arg (get_apiAccessToken ()).toLocal8Bit ());
        request.setHeader (QNetworkRequest::ContentTypeHeader, QString ("application/json"));
        QString data = QString ("{\"action\":\"markAsRead\",\"type\":\"entries\",\"entryIds\":[\"%1\"]}").arg (entries.join ("\",\""));
        //qDebug () << "POST DATA=" << data;
        QNetworkReply * reply = m_netMan->post (request, data.toLocal8Bit ());
        reply->setProperty ("entries", entries);
        connect (reply, &QNetworkReply::finished, this, &MyFeedlyApi::onPushLocalOperationsReply);
    }
    else {
        requestReadOperations ();
    }
}

void MyFeedlyApi::markItemAsRead (QString entryId) {
    qDebug () << "MyFeedlyApi::markItemAsRead :" << entryId;
    if (!entryId.isEmpty ()) {
        MyContent * entry = getContentInfo (entryId);
        if (entry) {
            if (entry->get_unread ()) {
                QSqlQuery querySave (m_database);
                querySave.prepare ("INSERT INTO sync (entryId, type) VALUES (:entryId, :type)");
                querySave.bindValue (":entryId", entryId);
                querySave.bindValue (":type", "MARK_AS_READ");
                querySave.exec ();
                QSqlQuery queryUpdate (m_database);
                queryUpdate.prepare ("UPDATE news SET unread=0 WHERE entryId=:entryId");
                queryUpdate.bindValue (":entryId", entryId);
                queryUpdate.exec ();
                entry->set_unread (false);
                m_timerUnreadCounts->start ();
                m_timerAutoSync->start ();
            }
        }
    }
}

void MyFeedlyApi::markCurrentStreamAsRead () {
    qDebug () << "MyFeedlyApi::markCurrentStreamAsRead";
    m_database.transaction ();
    for (int idx = 0; idx < m_newsStreamList->count (); idx++) {
        QVariantMap item = m_newsStreamList->valueAt (idx);
        markItemAsRead (item.value ("entryId").toString ());
    }
    m_database.commit ();
    qApp->processEvents ();
}

void MyFeedlyApi::logoutAndSweepAll () {
    qDebug () << "MyFeedlyApi::logoutAndSweepAll";
    set_currentStatusMsg   (tr ("Dropping local database..."));
    set_isPolling          (true);
    set_isOffline          (true);
    set_currentStreamId    ("");
    set_currentEntryId     ("");
    m_newsStreamList->deleteAll    ();
    m_subscriptionsList->deleteAll ();
    qApp->processEvents    ();
    m_database.transaction ();
    m_database.exec        ("DROP TABLE sync");
    m_database.exec        ("DROP TABLE news");
    m_database.exec        ("DROP TABLE feeds");
    m_database.exec        ("DROP TABLE categories");
    m_database.commit      ();
    qApp->processEvents    ();
    m_database.exec        ("VACUUM");
    qApp->processEvents    ();
    initializeTables       ();
    qApp->processEvents    ();
    set_currentStatusMsg   (tr ("Cleaning personal info..."));
    set_apiAccessToken     ("");
    set_apiCode            ("");
    set_apiRefreshToken    ("");
    set_apiUserId          ("");
    foreach (QString feedId, m_feeds.keys ()) {
        MyFeed * feed = getFeedInfo (feedId);
        if (feed) {
            feed->set_counter (0);
        }
        m_feeds.remove (feedId);
        delete feed;
        feed = NULL;
    }
    qApp->processEvents    ();
    foreach (QString categoryId, m_categories.keys ()) {
        MyCategory * category = getCategoryInfo (categoryId);
        if (category) {
            category->set_counter (0);
        }
        if (categoryId != streamIdAll && categoryId != streamIdMarked && categoryId != streamIdUncategorized) {
            m_categories.remove (categoryId);
            delete category;
            category = NULL;
        }
    }
    qApp->processEvents    ();
    set_isOffline        (false);
    set_isPolling        (false);
    set_isLogged         (false);
}

/******************************* CALLBACKS *****************************************/

void MyFeedlyApi::onCurrentStreamIdChanged (QString arg) {
    qDebug () << "onCurrentStreamIdChanged :" << arg;
    set_currentPageCount (0);
    set_streamMostRecentMSecs (CURR_MSECS);
    m_newsStreamList->deleteAll ();
    qApp->processEvents ();
    refreshStreamModel ();
}

void MyFeedlyApi::onShowOnlyUnreadChanged (bool arg) {
    qDebug () << "onShowOnlyUnreadChanged :" << arg;
    set_currentPageCount (0);
    set_streamMostRecentMSecs (CURR_MSECS);
    m_newsStreamList->deleteAll ();
    refreshStreamModel ();
}

void MyFeedlyApi::onCurrentPageCountChanged (int arg) {
    qDebug () << "onCurrentPageCountChanged :" << arg;
    refreshStreamModel ();
}

void MyFeedlyApi::onIsOfflineChanged (bool arg) {
    qDebug () << "onIsOfflineChanged :" << arg;
    if (arg) {
        m_timerContents->stop ();
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
            set_apiCode (apiCode);
            QString html;
            QTextStream stream (&html);
            stream << "HTTP/1.1 200 OK" << CRLF
                   << "status: 200 OK" << CRLF
                   << "version: HTTP/1.1" << CRLF
                   << "content-type: text/html; charset=UTF-8" << CRLF
                   << CRLF
                   << "<html><head></head><body style='color: black;'>"
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
    if (reply->error () == QNetworkReply::NoError) {
        QByteArray data = reply->readAll ();
        QJsonParseError error;
        QJsonDocument json = QJsonDocument::fromJson (data, &error);
        if (!json.isNull () && json.isObject ()) {
            QJsonObject obj = json.object ();
            set_apiUserId       (obj.value ("id").toString ());
            set_apiAccessToken  (obj.value ("access_token").toString ());
            set_apiRefreshToken (obj.value ("refresh_token").toString ());
            qDebug () << "userId=" << get_apiUserId ()
                      << "accessToken=" << get_apiAccessToken ()
                      << "refreshToken=" << get_apiRefreshToken ();
            set_isLogged (true);
            requestCategories ();
        }
        else {
            qWarning () << "Failed to parse tokens from JSON response :"
                        << error.errorString ()
                        << data;
        }
    }
    else {
        qWarning () << "Network error on token request :"
                    << reply->errorString ();
    }
}

void MyFeedlyApi::onRefreshTokenReply () {
    qDebug () << "onRefreshTokenReply";
    set_currentStatusMsg (tr ("Idle."));
    set_isPolling (false);
    QNetworkReply * reply = qobject_cast<QNetworkReply *>(sender ());
    Q_ASSERT (reply);
    if (reply->error () == QNetworkReply::NoError) {
        QByteArray data = reply->readAll ();
        QJsonParseError error;
        QJsonDocument json = QJsonDocument::fromJson (data, &error);
        if (!json.isNull () && json.isObject ()) {
            QJsonObject obj = json.object ();
            set_apiAccessToken  (obj.value ("access_token").toString ());
            qDebug () << "accessToken=" << get_apiAccessToken ();
            set_isLogged (true);
            requestCategories ();
        }
        else {
            qWarning () << "Failed to parse tokens from JSON response :"
                        << error.errorString ()
                        << data;
        }
    }
    else {
        qWarning () << "Network error on token refresh :"
                    << reply->errorString ();
    }
}

void MyFeedlyApi::onRequestCategoriesReply () {
    qDebug () << "onRequestCategoriesReply";
    QNetworkReply * reply = qobject_cast<QNetworkReply *>(sender ());
    Q_ASSERT (reply);
    if (reply->error () == QNetworkReply::NoError) {
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
                qApp->processEvents ();
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
                qApp->processEvents ();
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
    else {
        qWarning () << "Network error on categories request :"
                    << reply->errorString ();
    }
}

void MyFeedlyApi::onRequestSubscriptionsReply () {
    qDebug () << "onRequestSubscriptionsReply";
    QNetworkReply * reply = qobject_cast<QNetworkReply *>(sender ());
    Q_ASSERT (reply);
    if (reply->error () == QNetworkReply::NoError) {
        QByteArray data = reply->readAll ();
        //qDebug () << data;
        QJsonParseError error;
        QJsonDocument json = QJsonDocument::fromJson (data, &error);
        if (!json.isNull () && json.isArray ()) {
            QJsonArray array = json.array ();
            QStringList feeds;
            ///// ADD NEW ITEMS /////
            m_database.transaction ();
            foreach (QJsonValue value, array) {
                QJsonObject item = value.toObject ();
                QString feedId = jsonPathAsVariant (item, "id", "").toString ();
                QSqlQuery query (m_database);
                query.prepare ("INSERT OR IGNORE INTO feeds (feedId) VALUES (:feedId);");
                query.bindValue (":feedId", feedId);
                query.exec ();
                feeds << feedId;
            }
            m_database.commit ();
            qApp->processEvents ();
            ///// UPDATE ALL ITEMS /////
            m_database.transaction ();
            foreach (QJsonValue value, array) {
                QJsonObject item = value.toObject ();
                QSqlQuery query (m_database);
                query.prepare ("UPDATE feeds "
                               "SET title=:title, website=:website, updated=:updated, categoryId=:categoryId "
                               "WHERE feedId=:feedId;");
                query.bindValue (":title",      jsonPathAsVariant (item, "title", "").toString ());
                query.bindValue (":website",    jsonPathAsVariant (item, "website", "").toString ());
                query.bindValue (":updated",    jsonPathAsVariant (item, "updated", 0).toReal ());
                query.bindValue (":categoryId", jsonPathAsVariant (item, "categories/0/id", "").toString ());
                query.bindValue (":feedId",     jsonPathAsVariant (item, "id", "").toString ());
                query.exec ();
            }
            m_database.commit ();
            qApp->processEvents ();
            ///// REMOVE OLD ITEMS /////
            m_database.transaction ();
            m_database.exec (QString ("DELETE FROM news  WHERE streamId NOT IN (\"%1\");").arg (feeds.join ("\", \"")));
            m_database.exec (QString ("DELETE FROM feeds WHERE feedId   NOT IN (\"%1\");").arg (feeds.join ("\", \"")));
            m_database.commit ();
            //qDebug () << "feeds=" << feeds;
            loadSubscriptions ();
            m_timerUnreadCounts->start ();
        }
        else {
            qWarning () << "Failed to parse feeds from JSON response :"
                        << error.errorString ()
                        << data;
        }
    }
    else {
        qWarning () << "Network error on feeds request :"
                    << reply->errorString ();
    }
}

void MyFeedlyApi::onRequestContentsReply () {
    qDebug () << "onRequestContentsReply";
    QNetworkReply * reply = qobject_cast<QNetworkReply *>(sender ());
    Q_ASSERT (reply);
    if (!m_pollQueue.isEmpty ()) {
        m_timerContents->start (10); // do next quickly
    }
    else {
        set_currentStatusMsg (tr ("Idle."));
        set_isPolling (false);
    }
    if (reply->error () == QNetworkReply::NoError) {
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
                QString content   = jsonPathAsVariant (item, "content/content", "").toString ();
                QString summary   = jsonPathAsVariant (item, "summary/content", "").toString ();
                QString thumbnail = jsonPathAsVariant (item, "visual/url", "").toString ();
                QSqlQuery query (m_database);
                query.prepare ("INSERT OR IGNORE INTO "
                               "news (entryId, streamId, title, author, content, link, thumbnail, unread, published, updated, crawled, cached) "
                               "VALUES (:entryId, :streamId, :title, :author, :content, :link, :thumbnail, :unread, :published, :updated, :crawled, :cached);");
                query.bindValue (":entryId",   jsonPathAsVariant (item, "id", "").toString ());
                query.bindValue (":streamId",  jsonPathAsVariant (item, "origin/streamId", "").toString ());
                query.bindValue (":title",     jsonPathAsVariant (item, "title", "").toString ().trimmed ());
                query.bindValue (":author",    jsonPathAsVariant (item, "author", "").toString ().trimmed ());
                query.bindValue (":content",   (content.size () >= summary.size () ? content : summary));
                query.bindValue (":link",      jsonPathAsVariant (item, "alternate/0/href", "").toString ().trimmed ());
                query.bindValue (":thumbnail", (thumbnail != "none" ? thumbnail : ""));
                query.bindValue (":unread",    jsonPathAsVariant (item, "unread", true).toBool ());
                query.bindValue (":published", jsonPathAsVariant (item, "published", 0).toReal ());
                query.bindValue (":updated",   jsonPathAsVariant (item, "updated", 0).toReal ());
                query.bindValue (":crawled",   jsonPathAsVariant (item, "crawled", 0).toReal ());
                query.bindValue (":cached",    CURR_MSECS);
                query.exec ();
            }
            m_database.commit ();
            qApp->processEvents ();
        }
        else {
            qWarning () << "Failed to parse contents from JSON response :"
                        << error.errorString ()
                        << data;
        }
        getFeedInfo (reply->property("feedId").toString ())->set_status (MyFeed::Idle);
        m_timerUnreadCounts->start ();
    }
    else {
        qWarning () << "Network error on contents request :"
                    << reply->errorString ();
    }
}

void MyFeedlyApi::onRequestReadOperationsReply () {
    qDebug () << "onRequestReadOperationsReply";
    QNetworkReply * reply = qobject_cast<QNetworkReply *>(sender ());
    Q_ASSERT (reply);
    if (reply->error () == QNetworkReply::NoError) {
        QByteArray data = reply->readAll ();
        //qDebug () << "data=\n" << data;
        QJsonParseError error;
        QJsonDocument json = QJsonDocument::fromJson (data, &error);
        if (!json.isNull () && json.isObject ()) {
            QJsonObject item = json.object ();
            QJsonArray feeds = item.value ("feeds").toArray ();
            m_database.transaction ();
            foreach (QJsonValue feed, feeds) {
                QJsonObject feedObj = feed.toObject ();
                QSqlQuery feedQuery (m_database);
                feedQuery.prepare ("UPDATE news SET unread=0 WHERE streamId=:streamId AND crawled<:asOf");
                feedQuery.bindValue (":streamId", feedObj.value ("id").toString ());
                feedQuery.bindValue (":asOf", feedObj.value ("asOf").toDouble ());
                feedQuery.exec ();
            }
            m_database.commit ();
            qApp->processEvents ();
            QJsonArray entries = item.value ("entries").toArray ();
            m_database.transaction ();
            foreach (QJsonValue entry, entries) {
                QSqlQuery entryQuery (m_database);
                entryQuery.prepare ("UPDATE news SET unread=0 WHERE entryId=:entryId");
                entryQuery.bindValue (":entryId", entry.toString ());
                entryQuery.exec ();
            }
            m_database.commit ();
            qApp->processEvents ();
            if (!m_pollQueue.isEmpty ()) {
                set_isPolling (true);
                set_currentStatusMsg (tr ("Refreshing feeds..."));
                m_timerContents->start (1200);
            }
            else {
                set_currentStatusMsg (tr ("Idle."));
                set_isPolling (false);
            }
        }
        else {
            qWarning () << "Failed to parse contents from JSON response :"
                        << error.errorString ()
                        << data;
        }
    }
    else {
        qWarning () << "Network error on read operations request :"
                    << reply->errorString ();
    }
    m_timerUnreadCounts->start ();
}

void MyFeedlyApi::onPushLocalOperationsReply () {
    qDebug () << "onPushLocalOperationsReply";
    QNetworkReply * reply = qobject_cast<QNetworkReply *>(sender ());
    Q_ASSERT (reply);
    QStringList entries = reply->property ("entries").toStringList ();
    //qDebug () << "entries=" << entries;
    if (reply->error () == QNetworkReply::NoError) {
        m_database.transaction ();
        foreach (QString entryId, entries) {
            QSqlQuery query (m_database);
            query.prepare ("DELETE FROM sync WHERE entryId=:entryId AND type='MARK_AS_READ' AND value=1 ");
            query.bindValue (":entryId", entryId);
            query.exec ();
        }
        m_database.commit ();
        qApp->processEvents ();
        requestReadOperations ();
    }
    else {
        qWarning () << "Network error on push local operations :"
                    << reply->errorString ();
    }
}

/******************************* FROM DB *****************************************/

void MyFeedlyApi::loadSubscriptions () {
    QStringList categories;
    m_subscriptionsList->deleteAll ();
    QSqlQuery query (m_database);
    QString sql ("SELECT feeds.*,categories.label "
                 "FROM feeds "
                 "LEFT JOIN categories ON feeds.categoryId=categories.categoryId "
                 "ORDER BY label,title ASC;");
    if (query.exec (sql)) {
        QSqlRecord record = query.record ();
        int fieldCategoryId    = record.indexOf ("categoryId");
        int fieldCategoryLabel = record.indexOf ("label");
        int fieldFeedId        = record.indexOf ("feedId");
        int fieldFeedTitle     = record.indexOf ("title");
        int fieldFeedWebsite   = record.indexOf ("website");
        while (query.next ()) {
            QString feedId     = query.value (fieldFeedId).toString ();
            QString categoryId = query.value (fieldCategoryId).toString ();
            if (categoryId.isNull () || categoryId.isEmpty ()) {
                categoryId = streamIdUncategorized;
            }
            //qDebug () << "categoryId >>>" << categoryId;
            ///// MODEL ENTRY /////
            QVariantMap entry;
            entry.insert ("categoryId", categoryId);
            if (!categories.contains (categoryId)) {
                entry.insert ("feedId", "");
                m_subscriptionsList->append (entry);
                categories.append (categoryId);
            }
            entry.insert ("feedId", feedId);
            m_subscriptionsList->append (entry);
            //qDebug () << "entry >>>" << entry;
            ///// CATEGORY INFO /////
            if (categoryId != streamIdUncategorized) {
                MyCategory * category = getCategoryInfo (categoryId);
                category->set_label (query.value (fieldCategoryLabel).toString ());
            }
            ///// FEED INFO /////
            MyFeed * feed = getFeedInfo (feedId);
            feed->set_categoryId (categoryId);
            feed->set_title   (query.value (fieldFeedTitle).toString ());
            feed->set_website (query.value (fieldFeedWebsite).toString ());
        }
        qApp->processEvents ();
        //qDebug () << "categories list model=" << categories;
    }
    else {
        qWarning () << "Failed to load subscriptions :"
                    << query.lastError ().text ();
    }
}

void MyFeedlyApi::loadUnreadCounts () {
    quint64 total = 0;
    QStringList streamIds;
    ///// FEEDS UNREAD COUNTS /////
    QSqlQuery queryFeeds (m_database);
    QString sqlFeeds ("SELECT COUNT (entryId) AS unreadcount,streamId FROM news WHERE unread=1 GROUP BY streamId;");
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
        qApp->processEvents ();
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
            if (categoryId.isEmpty ()) {
                categoryId = streamIdUncategorized;
            }
            int unreadCount = queryCategories.value (fieldUnreadCount).toInt ();
            MyCategory * category = getCategoryInfo (categoryId);
            category->set_counter (unreadCount);
            streamIds << categoryId;
        }
        qApp->processEvents ();
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
    qApp->processEvents ();
    //////////////////// GLOBAL UNREAD COUNT /////////////////////////
    MyCategory * categoryAll = getCategoryInfo (streamIdAll);
    categoryAll->set_counter (total);
    //////////////////// MARKED UNREAD COUNT ////////////////////////
    QSqlQuery queryMarked (m_database);
    QString sqlMarked ("SELECT COUNT (entryId) AS unreadcount FROM news WHERE marked=1 AND unread=1");
    if (queryMarked.exec (sqlMarked) && queryMarked.next ()) {
        int unreadCount = queryMarked.value (queryMarked.record ().indexOf ("unreadcount")).toInt ();
        MyCategory * categoryMarked = getCategoryInfo (streamIdMarked);
        categoryMarked->set_counter (unreadCount);
    }
    else {
        qWarning () << "Failed to load marked unread counts :"
                    << queryMarked.lastError ().text ();
    }
    streamIds.clear ();
}

void MyFeedlyApi::refreshStreamModel () {
    if (!m_currentStreamId.isEmpty ()) {
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
        else if (m_currentStreamId.endsWith ("/category/global.uncategorized")){
            clauseSelect = " SELECT news.*,feeds.categoryId,feeds.feedId ";
            clauseFrom   = " FROM news,feeds ";
            clauseWhere.append (" news.streamId=feeds.feedId ");
            clauseWhere.append (" feeds.categoryId='' ");
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
        if (get_showOnlyUnread ()) {
            clauseWhere.append (" unread=1 ");
        }
        clauseWhere.append (" cached<=:olderThan ");
        QString clauseOrder (" ORDER BY published DESC ");
        QString clauseLimit = QString (" LIMIT %1,%2 ").arg (m_currentPageCount * PageSize).arg (PageSize);
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
        query.bindValue (":olderThan",  m_streamMostRecentMSecs);
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
            int fieldThumbnail  = record.indexOf ("thumbnail");
            while (query.next ()) {
                QString entryId = query.value (fieldEntryId).toString ();
                QDate date = QDateTime::fromMSecsSinceEpoch (query.value (fieldPublished).value<quint64> ()).date ();
                ///// MODEL ENTRY /////
                QVariantMap entry;
                entry.insert ("entryId", entryId);
                entry.insert ("date", date.toString ("yyyy-MM-dd"));
                m_newsStreamList->append (entry);
                ///// CONTENT INFO /////
                MyContent * content = getContentInfo (entryId);
                content->set_content   (query.value (fieldContent).toString ());
                content->set_title     (query.value (fieldTitle).toString ());
                content->set_author    (query.value (fieldAuthor).toString ());
                content->set_link      (query.value (fieldLink).toString ());
                content->set_thumbnail (query.value (fieldThumbnail).toString ());
                content->set_streamId  (query.value (fieldStreamId).toString ());
                content->set_unread    (query.value (fieldUnread).toInt () == 1);
                content->set_marked    (query.value (fieldMarked).toInt () == 1);
                content->set_updated   (query.value (fieldUpdated).value<quint64> ());
                content->set_crawled   (query.value (fieldCrawled).value<quint64> ());
                content->set_published (query.value (fieldPublished).value<quint64> ());
            }
            qApp->processEvents ();
        }
        else {
            qWarning () << "Failed to load stream :"
                        << query.lastError ().text ();
        }
    }
}

/************************ VARIANT LIST MODEL ****************************/

VariantModel::VariantModel (QStringList roles, QObject * parent) : QAbstractListModel (parent) {
    foreach (QString role, roles) {
        m_roles.insert (m_roles.count (), role.toLocal8Bit ());
    }
}

int VariantModel::count () const {
    return m_items.size ();
}

int VariantModel::roleId (QString roleName) const {
    return m_roles.key (roleName.toLocal8Bit ());
}

void VariantModel::prepend (QVariantMap item) {
    beginInsertRows (QModelIndex (), 0, 0);
    m_items.prepend (item);
    endInsertRows ();
}

void VariantModel::append (QVariantMap item) {
    beginInsertRows (QModelIndex (), count (), count ());
    m_items.append (item);
    endInsertRows ();
}

void VariantModel::deleteAll () {
    beginResetModel ();
    m_items.clear ();
    endResetModel ();
}

QVariantMap VariantModel::valueAt (int idx) const {
    QVariantMap ret;
    if (idx >= 0 && idx < count ()) {
        ret = m_items.at (idx);
    }
    return ret;
}

int VariantModel::rowCount (const QModelIndex & parent) const {
    Q_UNUSED (parent);
    return count ();
}

QVariant VariantModel::data (const QModelIndex & index, int role) const {
    return valueAt (index.row ()).value (QString::fromLocal8Bit (m_roles.value (role)));
}

QHash<int, QByteArray> VariantModel::roleNames() const {
    return m_roles;
}

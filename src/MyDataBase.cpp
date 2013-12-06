
#include "MyDataBase.h"

#include <QDebug>

MyDataBase::MyDataBase (QObject * parent) : QObject (parent) {
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
    connect (this, &MyDataBase::currentStreamIdChanged, this, &MyDataBase::onCurrentStreamIdChanged);
    connect (this, &MyDataBase::showOnlyUnreadChanged,  this, &MyDataBase::onShowOnlyUnreadChanged);
}

MyDataBase::~MyDataBase () {
    if (m_database.isOpen ()) {
        m_database.close ();
        qDebug ("Offline storage database closed.");
    }
}

MyFeed * MyDataBase::getFeedInfo (QString feedId) {
    MyFeed * ret = m_feeds.value (feedId, NULL);
    if (!ret) {
        ret = new MyFeed (this);
        ret->set_streamId (feedId);
        ret->set_counter  (0);
        m_feeds.insert (feedId, ret);
    }
    return ret;
}

MyCategory * MyDataBase::getCategoryInfo (QString categoryId) {
    MyCategory * ret = m_categories.value (categoryId, NULL);
    if (!ret) {
        ret = new MyCategory (this);
        ret->set_streamId (categoryId);
        ret->set_counter  (0);
        m_categories.insert (categoryId, ret);
    }
    return ret;
}

MyContent * MyDataBase::getContentInfo (QString entryId) {
    MyContent * ret = m_contents.value (entryId, NULL);
    if (!ret) {
        ret = new MyContent (this);
        ret->set_entryId (entryId);
        m_contents.insert (entryId, ret);
    }
    return ret;
}

void MyDataBase::initializeTables () {
    m_database.transaction ();
    m_database.exec ("CREATE TABLE IF NOT EXISTS categories ( "
                     "    categoryId TEXT PRIMARY KEY NOT NULL DEFAULT (''), "
                     "    label TEXT NOT NULL DEFAULT ('') "
                     ")");
    m_database.exec ("CREATE TABLE IF NOT EXISTS feeds ( "
                     "    feedId TEXT PRIMARY KEY NOT NULL DEFAULT (''), "
                     "    title TEXT NOT NULL DEFAULT (''), "
                     "    website TEXT NOT NULL DEFAULT (''), "
                     "    updated INTEGER, "
                     "    categoryId TEXT NOT NULL DEFAULT ('') "
                     ")");
    m_database.exec ("CREATE TABLE IF NOT EXISTS news ( "
                     "    entryId TEXT PRIMARY KEY NOT NULL DEFAULT (''), "
                     "    streamId TEXT NOT NULL DEFAULT (''), "
                     "    unread INTEGER NOT NULL DEFAULT (1), "
                     "    title TEXT NOT NULL DEFAULT (''), "
                     "    author TEXT DEFAULT (''), "
                     "    content TEXT NOT NULL DEFAULT (''), "
                     "    link TEXT DEFAULT (''), "
                     "    published INTEGER, "
                     "    crawled INTEGER, "
                     "    updated INTEGER  "
                     ")");
    m_database.commit ();
}

void MyDataBase::refreshStreamModel () {
    QVariantList ret;
    QString clauseSelect;
    QString clauseFrom;
    QStringList clauseWhere;
    if (m_currentStreamId.endsWith ("/global.all")) {
        clauseSelect = " SELECT * ";
        clauseFrom   = " FROM news ";
    }
    else {
        clauseSelect = " SELECT news.*,feeds.categoryId AS categoryId,feeds.feedId AS feedId ";
        clauseFrom   = " FROM news,feeds ";
        clauseWhere.append (" (news.streamId=feeds.feedId) ");
        clauseWhere.append (" (feeds.feedId=:feedId OR feeds.categoryId=:categoryId) ");
    }
    if (m_showOnlyUnread) {
        clauseWhere.append (" unread=1 ");
    }
    QString clauseOrder (" ORDER BY published DESC ");
    QString clauseLimit (" LIMIT 1000 ");
    QString sql (clauseSelect + clauseFrom + (!clauseWhere.isEmpty () ? " WHERE " + clauseWhere.join (" AND ") : "") + clauseOrder + clauseLimit + ";");
    qDebug () << "sql=" << sql;
    QSqlQuery query (m_database);
    query.prepare   (sql.trimmed ());
    query.bindValue (":feedId",     m_currentStreamId);
    query.bindValue (":categoryId", m_currentStreamId);
    if (query.exec ()) {
        QSqlRecord record  = query.record ();
        int fieldEntryId = record.indexOf ("entryId");
        int fieldStreamId = record.indexOf ("streamId");
        int fieldUnread = record.indexOf ("unread");
        int fieldTitle  = record.indexOf ("title");
        int fieldAuthor =  record.indexOf ("author");
        int fieldContent = record.indexOf ("content");
        int fieldLink = record.indexOf ("link");
        int fieldPublished = record.indexOf ("published");
        int fieldCrawled = record.indexOf ("crawled");
        int fieldUpdated = record.indexOf ("updated");
        int fieldCategoryId = record.indexOf ("categoryId");
        int fieldFeedId = record.indexOf ("feedId");
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
            content->set_marked    (false); // FIXME
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

void MyDataBase::onCurrentStreamIdChanged (QString arg) {
    qDebug () << "onCurrentStreamIdChanged :" << arg;
    refreshStreamModel ();
}

void MyDataBase::onShowOnlyUnreadChanged (bool arg) {
    qDebug () << "onShowOnlyUnreadChanged :" << arg;
    refreshStreamModel ();
}

void MyDataBase::loadSubscriptions () {
    QSqlQuery query (m_database);
    QString sql ("SELECT * FROM categories,feeds "
                 "WHERE feeds.categoryId=categories.categoryId ORDER BY label,title ASC;");
    QVariantList ret;
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
}

void MyDataBase::loadUnreadCounts () {
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
    streamIds.clear ();
}

#include "MyDataBase.h"

#include <QDebug>

MyDataBase::MyDataBase (QObject * parent) : QObject (parent) {
    m_categoryInfo = new QQmlPropertyMap (this);
    m_feedInfo = new QQmlPropertyMap (this);
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
}

MyDataBase::~MyDataBase () {
    if (m_database.isOpen ()) {
        m_database.close ();
        qDebug ("Offline storage database closed.");
    }
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

void MyDataBase::loadSubscriptions () {
    QSqlQuery query (m_database);
    QString sql ("SELECT * FROM categories,feeds WHERE feeds.categoryId=categories.categoryId ORDER BY label,title ASC;");
    QVariantList ret;
    if (query.exec (sql)) {
        QSqlRecord record = query.record ();
        int fieldCategoryId    = record.indexOf ("categoryId");
        int fieldCategoryLabel = record.indexOf ("label");
        int fieldFeedId        = record.indexOf ("feedId");
        int fieldFeedTitle     = record.indexOf ("title");
        int fieldFeedWebsite   = record.indexOf ("website");
        int fieldFeedUpdated   = record.indexOf ("updated");
        while (query.next ()) {
            QVariantMap entry;
            QString feedId     = query.value (fieldFeedId).toString ();
            QString categoryId = query.value (fieldCategoryId).toString ();
            ///// MODEL ENTRY /////
            entry.insert ("categoryId", categoryId);
            entry.insert ("feedId",     feedId);
            ret << entry;
            qDebug () << ">>>" << entry;
            ///// CATEGORY INFO /////
            MyCategory * category = m_categoryInfo->value (categoryId).value<MyCategory *> ();
            if (!category) {
                category = new MyCategory (this);
                category->set_streamId (categoryId);
                category->set_counter  (0);
                m_categoryInfo->insert (categoryId, QVariant::fromValue (category));
            }
            category->set_label (query.value (fieldCategoryLabel).toString ());
            ///// FEED INFO /////
            MyFeed * feed = m_feedInfo->value (feedId).value<MyFeed *> ();
            if (!feed) {
                feed = new MyFeed (this);
                feed->set_streamId (feedId);
                feed->set_counter  (0);
                m_feedInfo->insert (feedId, QVariant::fromValue (feed));
            }
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

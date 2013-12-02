#ifndef MYDATABASE_H
#define MYDATABASE_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QSqlError>
#include <QSqlRecord>
#include <QDir>
#include <QQmlPropertyMap>

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

public: explicit MyCategory (QObject * parent = NULL) : QObject (parent) { }
};

class MyFeed : public QObject {
    Q_OBJECT
    QML_PUBLIC_PROPERTY (QString, streamId)
    QML_PUBLIC_PROPERTY (QString, title)
    QML_PUBLIC_PROPERTY (QString, website)
    QML_PUBLIC_PROPERTY (int,     counter)

public: explicit MyFeed (QObject * parent = NULL) : QObject (parent) { }
};

class MyDataBase : public QObject {
    Q_OBJECT
    QML_PUBLIC_PROPERTY (QVariantList, subscriptionsList)
    QML_READONLY_PROPERTY (QQmlPropertyMap *, categoryInfo)
    QML_READONLY_PROPERTY (QQmlPropertyMap *, feedInfo)

public:
    explicit MyDataBase (QObject * parent = NULL);
    virtual ~MyDataBase ();


signals:


public slots:
    void loadSubscriptions ();


protected:
    void initializeTables  ();

private:
    QSqlDatabase m_database;

};

#endif // MYDATABASE_H

#include "databasemangager.h"

DataBaseMangager &DataBaseMangager::instance()
{
    static DataBaseMangager instance;
    return instance;
}

bool DataBaseMangager::openDatabase(const QString &path)
{
    db.setDatabaseName(path);
    if(!db.isOpen())
    {
        qDebug() << "无法打开数据库：" << db.lastError().text();
        return false;
    }
    return true;
}

void DataBaseMangager::closeDatabase()
{
    if(db.isOpen())
    {
        db.close();
    }
}

QString DataBaseMangager::getDatabasePath() const
{
    return dbPath;
}

void DataBaseMangager::setDatabasePath(const QString &path)
{
    if(path != dbPath)
    {
        dbPath = path;
        closeDatabase();
        openDatabase(path);
    }

}

DataBaseMangager::~DataBaseMangager()
{
    closeDatabase();
}

DataBaseMangager::DataBaseMangager(QObject *parent)
    : QObject{parent}
{
    db =QSqlDatabase::addDatabase("QSQLITE");
    openDatabase(dbPath);
}



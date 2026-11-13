#ifndef DATABASEMANGAGER_H
#define DATABASEMANGAGER_H

#include <QObject>
#include<QSqlDatabase>
#include <QDebug>
#include <QSqlError>
//数据库设计为单例模式，因为多处会调用
class DataBaseMangager : public QObject
{
    Q_OBJECT
public:
    static DataBaseMangager& instance();
    bool openDatabase(const QString& path);
    void closeDatabase();
    QString getDatabasePath() const;
    void setDatabasePath(const QString& path);
    ~DataBaseMangager();
private:
    explicit DataBaseMangager(QObject *parent = nullptr);
    QSqlDatabase db;
    QString dbPath = "E:\\Qt_Program\\Qt_DoctorMangerSystem\\DoctorMangerSystem\\DoctorMangerSys.db";
signals:
};

#endif // DATABASEMANGAGER_H

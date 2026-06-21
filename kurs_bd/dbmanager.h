#ifndef DBMANAGER_H
#define DBMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlQueryModel>
#include <QList>
#include "models.h"

class DBManager : public QObject {
    Q_OBJECT
public:
    // Получение инстанса (Singleton)
    static DBManager& instance();

    // Подключение к БД
    bool openConnection(const QString& host, int port, const QString& dbName, 
                        const QString& user, const QString& password);
    void closeConnection();
    bool isOpen() const;
    QString lastError() const;

    // Авторизация и пользователи
    User authenticateUser(const QString& login, const QString& password, bool& ok);
    QStringList getUserRoles(int id_user);
    bool registerUser(const QString& login, const QString& password, const QStringList& roleNames, int& newUserId);
    bool updateUserPassword(int id_user, const QString& newPassword);
    bool updateUserRoles(int id_user, const QStringList& roleNames);
    QList<User> getAllUsers();
    bool deleteUser(int id_user);
    QList<Role> getAllRoles();

    // Управление водителями (Drivers)
    QList<Driver> getAllDrivers();
    Driver getDriverById(int id_driver);
    Driver getDriverByUserId(int id_user);
    QList<User> getAvailableUsersForDrivers(int currentDriverUserId = 0);
    bool insertDriver(const Driver& driver, int& newDriverId);
    bool updateDriver(const Driver& driver);
    bool deleteDriver(int id_driver);

    // Управление маршрутами (Routes)
    QList<Route> getAllRoutes();
    Route getRouteById(int id_route);
    bool insertRoute(const Route& route, int& newRouteId);
    bool updateRoute(const Route& route);
    bool deleteRoute(int id_route);

    // Управление перевозками (Transportations)
    QList<Transportation> getAllTransportations();
    Transportation getTransportationById(int id_transportation);
    bool insertTransportation(const Transportation& trans, int& newTransId);
    bool updateTransportation(const Transportation& trans);
    bool deleteTransportation(int id_transportation);

    // Управление назначениями водителей (Driver_Transportations)
    QList<DriverTransportation> getAssignmentsForTransportation(int id_transportation);
    QList<DriverTransportation> getAssignmentsForDriver(int id_driver);
    bool assignDriverToTransportation(int id_driver, int id_transportation, double payment = -1.0); // -1.0 = NULL в БД (авторасчет)
    bool updateAssignmentBonus(int id_driver, int id_transportation, double bonus, const QString& reason);
    bool removeDriverFromTransportation(int id_driver, int id_transportation);

    // Аналитические отчеты (Views)
    QSqlQueryModel* getSalaryStatementModel(QObject* parent = nullptr);
    QSqlQueryModel* getTransportationSummaryModel(QObject* parent = nullptr);

private:
    DBManager();
    ~DBManager();
    DBManager(const DBManager&) = delete;
    DBManager& operator=(const DBManager&) = delete;

    QSqlDatabase m_db;
    QString m_lastError;
};

#endif // DBMANAGER_H

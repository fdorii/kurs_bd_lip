#include "dbmanager.h"
#include <QVariant>
#include <QDebug>

DBManager::DBManager() {
    m_db = QSqlDatabase::addDatabase("QPSQL");
}

DBManager::~DBManager() {
    closeConnection();
}

DBManager& DBManager::instance() {
    static DBManager inst;
    return inst;
}

bool DBManager::openConnection(const QString& host, int port, const QString& dbName, 
                              const QString& user, const QString& password) {
    m_db.setHostName(host);
    m_db.setPort(port);
    m_db.setDatabaseName(dbName);
    m_db.setUserName(user);
    m_db.setPassword(password);

    if (!m_db.open()) {
        m_lastError = m_db.lastError().text();
        return false;
    }
    m_lastError = "";
    return true;
}

void DBManager::closeConnection() {
    if (m_db.isOpen()) {
        m_db.close();
    }
}

bool DBManager::isOpen() const {
    return m_db.isOpen();
}

QString DBManager::lastError() const {
    return m_lastError;
}

// ==========================================
// АВТОРИЗАЦИЯ И ПОЛЬЗОВАТЕЛИ
// ==========================================

User DBManager::authenticateUser(const QString& login, const QString& password, bool& ok) {
    ok = false;
    User user;

    QSqlQuery query(m_db);
    query.prepare("SELECT id_user, login, password FROM Users WHERE login = :login AND password = :password");
    query.bindValue(":login", login);
    query.bindValue(":password", password);

    if (query.exec() && query.next()) {
        user.id_user = query.value("id_user").toInt();
        user.login = query.value("login").toString();
        user.password = query.value("password").toString();
        ok = true;
    } else {
        m_lastError = query.lastError().text();
    }
    return user;
}

QStringList DBManager::getUserRoles(int id_user) {
    QStringList roles;
    QSqlQuery query(m_db);
    query.prepare("SELECT r.role_name FROM User_Roles ur "
                  "JOIN Roles r ON ur.id_role = r.id_role "
                  "WHERE ur.id_user = :id_user");
    query.bindValue(":id_user", id_user);

    if (query.exec()) {
        while (query.next()) {
            roles.append(query.value(0).toString());
        }
    } else {
        m_lastError = query.lastError().text();
    }
    return roles;
}

bool DBManager::registerUser(const QString& login, const QString& password, const QStringList& roleNames, int& newUserId) {
    // 1. Вставка в Users
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO Users (login, password) VALUES (:login, :password) RETURNING id_user");
    query.bindValue(":login", login);
    query.bindValue(":password", password);

    if (!query.exec() || !query.next()) {
        m_lastError = query.lastError().text();
        return false;
    }

    newUserId = query.value(0).toInt();

    // 2. Связывание с ролями
    for (const QString& roleName : roleNames) {
        // Находим id роли
        QSqlQuery roleQuery(m_db);
        roleQuery.prepare("SELECT id_role FROM Roles WHERE role_name = :name");
        roleQuery.bindValue(":name", roleName);
        if (!roleQuery.exec() || !roleQuery.next()) {
            m_lastError = roleQuery.lastError().text();
            return false;
        }
        int id_role = roleQuery.value(0).toInt();

        // Связываем пользователя с ролью
        QSqlQuery linkQuery(m_db);
        linkQuery.prepare("INSERT INTO User_Roles (id_user, id_role) VALUES (:id_user, :id_role)");
        linkQuery.bindValue(":id_user", newUserId);
        linkQuery.bindValue(":id_role", id_role);
        if (!linkQuery.exec()) {
            m_lastError = linkQuery.lastError().text();
            return false;
        }
    }
    return true;
}

bool DBManager::updateUserPassword(int id_user, const QString& newPassword) {
    QSqlQuery query(m_db);
    query.prepare("UPDATE Users SET password = :password WHERE id_user = :id_user");
    query.bindValue(":password", newPassword);
    query.bindValue(":id_user", id_user);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }
    return true;
}

bool DBManager::updateUserRoles(int id_user, const QStringList& roleNames) {
    // Удаляем старые роли
    QSqlQuery delQuery(m_db);
    delQuery.prepare("DELETE FROM User_Roles WHERE id_user = :id_user");
    delQuery.bindValue(":id_user", id_user);
    if (!delQuery.exec()) {
        m_lastError = delQuery.lastError().text();
        return false;
    }

    // Добавляем новые
    for (const QString& rName : roleNames) {
        QSqlQuery roleQuery(m_db);
        roleQuery.prepare("SELECT id_role FROM Roles WHERE role_name = :name");
        roleQuery.bindValue(":name", rName);
        if (!roleQuery.exec() || !roleQuery.next()) {
            m_lastError = roleQuery.lastError().text();
            return false;
        }
        int id_role = roleQuery.value(0).toInt();

        QSqlQuery insQuery(m_db);
        insQuery.prepare("INSERT INTO User_Roles (id_user, id_role) VALUES (:id_user, :id_role)");
        insQuery.bindValue(":id_user", id_user);
        insQuery.bindValue(":id_role", id_role);
        if (!insQuery.exec()) {
            m_lastError = insQuery.lastError().text();
            return false;
        }
    }
    return true;
}

QList<User> DBManager::getAllUsers() {
    QList<User> list;
    QSqlQuery query("SELECT id_user, login, password FROM Users", m_db);
    while (query.next()) {
        list.append(User(
            query.value("id_user").toInt(),
            query.value("login").toString(),
            query.value("password").toString()
        ));
    }
    return list;
}

bool DBManager::deleteUser(int id_user) {
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM Users WHERE id_user = :id_user");
    query.bindValue(":id_user", id_user);
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }
    return true;
}

QList<Role> DBManager::getAllRoles() {
    QList<Role> list;
    QSqlQuery query("SELECT id_role, role_name FROM Roles", m_db);
    while (query.next()) {
        list.append(Role(
            query.value("id_role").toInt(),
            query.value("role_name").toString()
        ));
    }
    return list;
}

// ==========================================
// УПРАВЛЕНИЕ ВОДИТЕЛЯМИ
// ==========================================

QList<Driver> DBManager::getAllDrivers() {
    QList<Driver> list;
    QSqlQuery query("SELECT id_driver, id_user, last_name, first_name, middle_name, experience FROM Drivers", m_db);
    while (query.next()) {
        list.append(Driver(
            query.value("id_driver").toInt(),
            query.value("id_user").toInt(),
            query.value("last_name").toString(),
            query.value("first_name").toString(),
            query.value("middle_name").toString(),
            query.value("experience").toInt()
        ));
    }
    return list;
}

Driver DBManager::getDriverById(int id_driver) {
    Driver d;
    QSqlQuery query(m_db);
    query.prepare("SELECT id_driver, id_user, last_name, first_name, middle_name, experience "
                  "FROM Drivers WHERE id_driver = :id");
    query.bindValue(":id", id_driver);

    if (query.exec() && query.next()) {
        d = Driver(
            query.value("id_driver").toInt(),
            query.value("id_user").toInt(),
            query.value("last_name").toString(),
            query.value("first_name").toString(),
            query.value("middle_name").toString(),
            query.value("experience").toInt()
        );
    }
    return d;
}

Driver DBManager::getDriverByUserId(int id_user) {
    Driver d;
    QSqlQuery query(m_db);
    query.prepare("SELECT id_driver, id_user, last_name, first_name, middle_name, experience "
                  "FROM Drivers WHERE id_user = :id_user");
    query.bindValue(":id_user", id_user);

    if (query.exec() && query.next()) {
        d = Driver(
            query.value("id_driver").toInt(),
            query.value("id_user").toInt(),
            query.value("last_name").toString(),
            query.value("first_name").toString(),
            query.value("middle_name").toString(),
            query.value("experience").toInt()
        );
    }
    return d;
}

QList<User> DBManager::getAvailableUsersForDrivers(int currentDriverUserId) {
    QList<User> list;
    QSqlQuery query(m_db);
    query.prepare("SELECT DISTINCT u.id_user, u.login FROM Users u "
                  "JOIN User_Roles ur ON u.id_user = ur.id_user "
                  "JOIN Roles r ON ur.id_role = r.id_role "
                  "WHERE r.role_name = 'Водитель' AND "
                  "(u.id_user NOT IN (SELECT id_user FROM Drivers WHERE id_user IS NOT NULL) OR u.id_user = :current_id)");
    query.bindValue(":current_id", currentDriverUserId);
    
    if (query.exec()) {
        while (query.next()) {
            list.append(User(
                query.value("id_user").toInt(),
                query.value("login").toString(),
                ""
            ));
        }
    } else {
        m_lastError = query.lastError().text();
    }
    return list;
}


bool DBManager::insertDriver(const Driver& driver, int& newDriverId) {
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO Drivers (id_user, last_name, first_name, middle_name, experience) "
                  "VALUES (:id_user, :last_name, :first_name, :middle_name, :experience) RETURNING id_driver");
    
    if (driver.id_user > 0) {
        query.bindValue(":id_user", driver.id_user);
    } else {
        query.bindValue(":id_user", QVariant(QMetaType(QMetaType::Int))); // NULL
    }
    query.bindValue(":last_name", driver.last_name);
    query.bindValue(":first_name", driver.first_name);
    query.bindValue(":middle_name", driver.middle_name);
    query.bindValue(":experience", driver.experience);

    if (!query.exec() || !query.next()) {
        m_lastError = query.lastError().text();
        return false;
    }
    newDriverId = query.value(0).toInt();
    return true;
}

bool DBManager::updateDriver(const Driver& driver) {
    QSqlQuery query(m_db);
    query.prepare("UPDATE Drivers SET id_user = :id_user, last_name = :last_name, "
                  "first_name = :first_name, middle_name = :middle_name, experience = :experience "
                  "WHERE id_driver = :id");
    if (driver.id_user > 0) {
        query.bindValue(":id_user", driver.id_user);
    } else {
        query.bindValue(":id_user", QVariant(QMetaType(QMetaType::Int)));
    }
    query.bindValue(":last_name", driver.last_name);
    query.bindValue(":first_name", driver.first_name);
    query.bindValue(":middle_name", driver.middle_name);
    query.bindValue(":experience", driver.experience);
    query.bindValue(":id", driver.id_driver);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }
    return true;
}

bool DBManager::deleteDriver(int id_driver) {
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM Drivers WHERE id_driver = :id");
    query.bindValue(":id", id_driver);
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }
    return true;
}

// ==========================================
// УПРАВЛЕНИЕ МАРШРУТАМИ
// ==========================================

QList<Route> DBManager::getAllRoutes() {
    QList<Route> list;
    QSqlQuery query("SELECT id_route, route_name, distance, base_payment FROM Routes", m_db);
    while (query.next()) {
        list.append(Route(
            query.value("id_route").toInt(),
            query.value("route_name").toString(),
            query.value("distance").toDouble(),
            query.value("base_payment").toDouble()
        ));
    }
    return list;
}

Route DBManager::getRouteById(int id_route) {
    Route r;
    QSqlQuery query(m_db);
    query.prepare("SELECT id_route, route_name, distance, base_payment FROM Routes WHERE id_route = :id");
    query.bindValue(":id", id_route);
    if (query.exec() && query.next()) {
        r = Route(
            query.value("id_route").toInt(),
            query.value("route_name").toString(),
            query.value("distance").toDouble(),
            query.value("base_payment").toDouble()
        );
    }
    return r;
}

bool DBManager::insertRoute(const Route& route, int& newRouteId) {
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO Routes (route_name, distance, base_payment) "
                  "VALUES (:name, :dist, :base) RETURNING id_route");
    query.bindValue(":name", route.route_name);
    query.bindValue(":dist", route.distance);
    query.bindValue(":base", route.base_payment);

    if (!query.exec() || !query.next()) {
        m_lastError = query.lastError().text();
        return false;
    }
    newRouteId = query.value(0).toInt();
    return true;
}

bool DBManager::updateRoute(const Route& route) {
    QSqlQuery query(m_db);
    query.prepare("UPDATE Routes SET route_name = :name, distance = :dist, base_payment = :base WHERE id_route = :id");
    query.bindValue(":name", route.route_name);
    query.bindValue(":dist", route.distance);
    query.bindValue(":base", route.base_payment);
    query.bindValue(":id", route.id_route);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }
    return true;
}

bool DBManager::deleteRoute(int id_route) {
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM Routes WHERE id_route = :id");
    query.bindValue(":id", id_route);
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }
    return true;
}

// ==========================================
// УПРАВЛЕНИЕ ПЕРЕВОЗКАМИ
// ==========================================

QList<Transportation> DBManager::getAllTransportations() {
    QList<Transportation> list;
    QSqlQuery query("SELECT id_transportation, id_route, departure_date, arrival_date FROM Transportations", m_db);
    while (query.next()) {
        list.append(Transportation(
            query.value("id_transportation").toInt(),
            query.value("id_route").toInt(),
            query.value("departure_date").toDateTime(),
            query.value("arrival_date").toDateTime()
        ));
    }
    return list;
}

Transportation DBManager::getTransportationById(int id_transportation) {
    Transportation t;
    QSqlQuery query(m_db);
    query.prepare("SELECT id_transportation, id_route, departure_date, arrival_date "
                  "FROM Transportations WHERE id_transportation = :id");
    query.bindValue(":id", id_transportation);

    if (query.exec() && query.next()) {
        t = Transportation(
            query.value("id_transportation").toInt(),
            query.value("id_route").toInt(),
            query.value("departure_date").toDateTime(),
            query.value("arrival_date").toDateTime()
        );
    }
    return t;
}

bool DBManager::insertTransportation(const Transportation& trans, int& newTransId) {
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO Transportations (id_route, departure_date, arrival_date) "
                  "VALUES (:rId, :dep, :arr) RETURNING id_transportation");
    query.bindValue(":rId", trans.id_route);
    query.bindValue(":dep", trans.departure_date);
    
    if (trans.arrival_date.isValid()) {
        query.bindValue(":arr", trans.arrival_date);
    } else {
        query.bindValue(":arr", QVariant(QMetaType(QMetaType::QDateTime)));
    }

    if (!query.exec() || !query.next()) {
        m_lastError = query.lastError().text();
        return false;
    }
    newTransId = query.value(0).toInt();
    return true;
}

bool DBManager::updateTransportation(const Transportation& trans) {
    QSqlQuery query(m_db);
    query.prepare("UPDATE Transportations SET id_route = :rId, departure_date = :dep, arrival_date = :arr "
                  "WHERE id_transportation = :id");
    query.bindValue(":rId", trans.id_route);
    query.bindValue(":dep", trans.departure_date);
    if (trans.arrival_date.isValid()) {
        query.bindValue(":arr", trans.arrival_date);
    } else {
        query.bindValue(":arr", QVariant(QMetaType(QMetaType::QDateTime)));
    }
    query.bindValue(":id", trans.id_transportation);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }
    return true;
}

bool DBManager::deleteTransportation(int id_transportation) {
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM Transportations WHERE id_transportation = :id");
    query.bindValue(":id", id_transportation);
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }
    return true;
}

// ==========================================
// УПРАВЛЕНИЕ НАЗНАЧЕНИЯМИ (Driver_Transportations)
// ==========================================

QList<DriverTransportation> DBManager::getAssignmentsForTransportation(int id_transportation) {
    QList<DriverTransportation> list;
    QSqlQuery query(m_db);
    query.prepare("SELECT id_driver, id_transportation, payment, bonus_amount, bonus_reason "
                  "FROM Driver_Transportations WHERE id_transportation = :tId");
    query.bindValue(":tId", id_transportation);

    if (query.exec()) {
        while (query.next()) {
            list.append(DriverTransportation(
                query.value("id_driver").toInt(),
                query.value("id_transportation").toInt(),
                query.value("payment").toDouble(),
                query.value("bonus_amount").toDouble(),
                query.value("bonus_reason").toString()
            ));
        }
    }
    return list;
}

QList<DriverTransportation> DBManager::getAssignmentsForDriver(int id_driver) {
    QList<DriverTransportation> list;
    QSqlQuery query(m_db);
    query.prepare("SELECT id_driver, id_transportation, payment, bonus_amount, bonus_reason "
                  "FROM Driver_Transportations WHERE id_driver = :dId");
    query.bindValue(":dId", id_driver);

    if (query.exec()) {
        while (query.next()) {
            list.append(DriverTransportation(
                query.value("id_driver").toInt(),
                query.value("id_transportation").toInt(),
                query.value("payment").toDouble(),
                query.value("bonus_amount").toDouble(),
                query.value("bonus_reason").toString()
            ));
        }
    }
    return list;
}

bool DBManager::assignDriverToTransportation(int id_driver, int id_transportation, double payment) {
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO Driver_Transportations (id_driver, id_transportation, payment) "
                  "VALUES (:dId, :tId, :pay)");
    query.bindValue(":dId", id_driver);
    query.bindValue(":tId", id_transportation);
    
    if (payment >= 0.0) {
        query.bindValue(":pay", payment);
    } else {
        query.bindValue(":pay", QVariant(QMetaType(QMetaType::Double))); // Вставка NULL запустит триггер авторасчета!
    }

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }
    return true;
}

bool DBManager::updateAssignmentBonus(int id_driver, int id_transportation, double bonus, const QString& reason) {
    QSqlQuery query(m_db);
    query.prepare("UPDATE Driver_Transportations SET bonus_amount = :bonus, bonus_reason = :reason "
                  "WHERE id_driver = :dId AND id_transportation = :tId");
    query.bindValue(":bonus", bonus);
    query.bindValue(":reason", reason);
    query.bindValue(":dId", id_driver);
    query.bindValue(":tId", id_transportation);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }
    return true;
}

bool DBManager::removeDriverFromTransportation(int id_driver, int id_transportation) {
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM Driver_Transportations WHERE id_driver = :dId AND id_transportation = :tId");
    query.bindValue(":dId", id_driver);
    query.bindValue(":tId", id_transportation);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }
    return true;
}

// ==========================================
// АНАЛИТИЧЕСКИЕ ПРЕДСТАВЛЕНИЯ
// ==========================================

QSqlQueryModel* DBManager::getSalaryStatementModel(QObject* parent) {
    QSqlQueryModel* model = new QSqlQueryModel(parent);
    model->setQuery("SELECT id_driver AS \"ID Водителя\", "
                    "driver_full_name AS \"ФИО Водителя\", "
                    "experience_years AS \"Стаж (лет)\", "
                    "completed_trips AS \"Завершенные рейсы\", "
                    "trips_in_progress AS \"Рейсы в пути\", "
                    "total_payment AS \"Начислено оплаты (руб)\", "
                    "total_bonuses AS \"Начислено премий (руб)\", "
                    "total_earned AS \"Итого выплачено (руб)\" "
                    "FROM Salary_Statement", m_db);
    if (model->lastError().isValid()) {
        m_lastError = model->lastError().text();
    }
    return model;
}

QSqlQueryModel* DBManager::getTransportationSummaryModel(QObject* parent) {
    QSqlQueryModel* model = new QSqlQueryModel(parent);
    model->setQuery("SELECT id_transportation AS \"ID Рейса\", "
                    "route_name AS \"Название маршрута\", "
                    "distance_km AS \"Расстояние (км)\", "
                    "departure_date AS \"Дата отправления\", "
                    "arrival_date AS \"Дата прибытия\", "
                    "drivers AS \"Водители\", "
                    "total_drivers_payment AS \"Зарплаты водителей (руб)\", "
                    "total_drivers_bonuses AS \"Премии водителей (руб)\", "
                    "total_transportation_cost AS \"Общая себестоимость (руб)\" "
                    "FROM Transportation_Summary", m_db);
    if (model->lastError().isValid()) {
        m_lastError = model->lastError().text();
    }
    return model;
}

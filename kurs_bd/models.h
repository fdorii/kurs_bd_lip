#ifndef MODELS_H
#define MODELS_H

#include <QString>
#include <QDateTime>

// Модель роли пользователя
struct Role {
    int id_role;
    QString role_name;

    Role() : id_role(0) {}
    Role(int id, const QString& name) : id_role(id), role_name(name) {}
};

// Модель пользователя
struct User {
    int id_user;
    QString login;
    QString password; // Содержит пароль (или его хэш)

    User() : id_user(0) {}
    User(int id, const QString& log, const QString& pass) 
        : id_user(id), login(log), password(pass) {}
};

// Модель роли, привязанной к пользователю
struct UserRole {
    int id_user;
    int id_role;

    UserRole() : id_user(0), id_role(0) {}
    UserRole(int uId, int rId) : id_user(uId), id_role(rId) {}
};

// Модель водителя
struct Driver {
    int id_driver;
    int id_user; // Может быть NULL (0 в C++), если учетная запись удалена
    QString last_name;
    QString first_name;
    QString middle_name;
    int experience;

    Driver() : id_driver(0), id_user(0), experience(0) {}
    Driver(int id, int uId, const QString& last, const QString& first, const QString& middle, int exp)
        : id_driver(id), id_user(uId), last_name(last), first_name(first), middle_name(middle), experience(exp) {}

    QString getFullName() const {
        return QString("%1 %2 %3").arg(last_name, first_name, middle_name).trimmed();
    }
};

// Модель маршрута
struct Route {
    int id_route;
    QString route_name;
    double distance;
    double base_payment;

    Route() : id_route(0), distance(0.0), base_payment(0.0) {}
    Route(int id, const QString& name, double dist, double basePay)
        : id_route(id), route_name(name), distance(dist), base_payment(basePay) {}
};

// Модель перевозки
struct Transportation {
    int id_transportation;
    int id_route;
    QDateTime departure_date;
    QDateTime arrival_date; // Может быть невалидной (NULL в БД), если рейс в пути

    Transportation() : id_transportation(0), id_route(0) {}
    Transportation(int id, int rId, const QDateTime& dep, const QDateTime& arr = QDateTime())
        : id_transportation(id), id_route(rId), departure_date(dep), arrival_date(arr) {}

    bool isInProgress() const {
        return !arrival_date.isValid();
    }
};

// Модель связи водителя и перевозки (назначение на рейс)
struct DriverTransportation {
    int id_driver;
    int id_transportation;
    double payment;       // Вычисленная триггером оплата
    double bonus_amount;  // Размер премии
    QString bonus_reason; // Причина начисления премии

    DriverTransportation() : id_driver(0), id_transportation(0), payment(0.0), bonus_amount(0.0) {}
    DriverTransportation(int dId, int tId, double pay, double bonus = 0.0, const QString& reason = "")
        : id_driver(dId), id_transportation(tId), payment(pay), bonus_amount(bonus), bonus_reason(reason) {}
};

#endif // MODELS_H

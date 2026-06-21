#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QTableWidget>
#include <QTableView>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QDateTimeEdit>
#include <QSqlQueryModel>
#include <QSqlTableModel>
#include "models.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(int id_user, const QString& username, const QStringList& roles, QWidget* parent = nullptr);
    ~MainWindow();

    bool isLogoutRequested() const { return m_logoutRequested; }

private slots:
    // Действия профиля
    void onChangePasswordClicked();
    void onSaveProfileDetailsClicked();

    // Вкладки администратора
    void onAdminRefreshUsers();
    void onAdminDeleteUser();
    void onAdminSaveUserRoles();
    void onAdminCreateUser();
    void onAdminDriverRoleToggled(bool checked);

    // Вкладки диспетчера / администратора (CRUD Drivers, Routes, Transportations)
    void onRefreshDrivers();
    void onAddDriver();
    void onEditDriver();
    void onDeleteDriver();

    void onRefreshRoutes();
    void onAddRoute();
    void onEditRoute();
    void onDeleteRoute();

    void onRefreshTransportations();
    void onAddTransportation();
    void onEditTransportation();
    void onDeleteTransportation();

    // Вкладки назначения и премий (Диспетчер)
    void onRefreshAssignments();
    void onAssignDriver();
    void onRemoveAssignment();
    void onAddBonus();

    // Отчеты
    void onRefreshReports();

private:
    void setupUI();
    void setupAdminUI();
    void setupDispatcherUI();
    void setupDriverUI();
    void setupMechanicUI();
    void setupProfileTab();

    // Данные сессии
    int m_currentUserId;
    QString m_currentUsername;
    QStringList m_currentRoles;
    Driver m_currentDriverProfile; // Заполняется, если роль Водитель
    bool m_logoutRequested;

    // Общие элементы GUI
    QTabWidget* m_tabWidget;
    
    // Вкладка: Настройки Профиля
    QWidget* m_profileTab;
    QLineEdit* m_newPasswordEdit;
    QLineEdit* m_confirmNewPasswordEdit;
    QLineEdit* m_profLastNameEdit;
    QLineEdit* m_profFirstNameEdit;
    QLineEdit* m_profMiddleNameEdit;
    QSpinBox* m_profExperienceSpin;
    QWidget* m_profDriverDetailsWidget;

    // --- GUI Администратора ---
    // Управление пользователями
    QTableWidget* m_usersTable;
    QComboBox* m_userRolesCombo;
    QPushButton* m_saveUserRolesBtn;
    QPushButton* m_deleteUserBtn;

    // Регистрация нового пользователя администратором
    QLineEdit* m_adminNewUserLoginEdit;
    QLineEdit* m_adminNewUserPassEdit;
    QLineEdit* m_adminNewUserConfirmPassEdit;
    QCheckBox* m_adminNewUserAdminCheck;
    QCheckBox* m_adminNewUserDispCheck;
    QCheckBox* m_adminNewUserDriverCheck;
    QCheckBox* m_adminNewUserMechCheck;
    
    QWidget* m_adminNewUserDriverWidget;
    QLineEdit* m_adminNewUserLastNameEdit;
    QLineEdit* m_adminNewUserFirstNameEdit;
    QLineEdit* m_adminNewUserMiddleNameEdit;
    QSpinBox* m_adminNewUserExpSpin;

    bool validatePassword(const QString& password, QString& errorMessage);

    // --- GUI Диспетчера / Администратора ---
    // Таблица водителей (Drivers)
    QTableWidget* m_driversTable;
    QLineEdit* m_driverLastNameEdit;
    QLineEdit* m_driverFirstNameEdit;
    QLineEdit* m_driverMiddleNameEdit;
    QSpinBox* m_driverExperienceSpin;
    QComboBox* m_driverUserCombo; // привязка к логину

    // Таблица маршрутов (Routes)
    QTableWidget* m_routesTable;
    QLineEdit* m_routeNameEdit;
    QDoubleSpinBox* m_routeDistanceSpin;
    QDoubleSpinBox* m_routeBasePaySpin;

    // Таблица перевозок (Transportations)
    QTableWidget* m_transTable;
    QComboBox* m_transRouteCombo;
    QDateTimeEdit* m_transDepEdit;
    QDateTimeEdit* m_transArrEdit;
    QCheckBox* m_transCompletedCheck;

    // Управление назначениями
    QTableWidget* m_assignTable;
    QComboBox* m_assignDriverCombo;
    QComboBox* m_assignTransCombo;
    QDoubleSpinBox* m_bonusAmountSpin;
    QLineEdit* m_bonusReasonEdit;

    // --- GUI Водителя ---
    QTableWidget* m_myTripsTable;

    // --- GUI Механика ---
    QTableWidget* m_currentTripsTable;

    // --- Вкладка Отчетов ---
    QTableView* m_reportSalaryTable;
    QTableView* m_reportTransTable;
    QSqlQueryModel* m_reportSalaryModel;
    QSqlQueryModel* m_reportTransModel;
};

#endif // MAINWINDOW_H

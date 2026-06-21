#include "mainwindow.h"
#include "dbmanager.h"
#include <QMessageBox>
#include <QHeaderView>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRegularExpression>

MainWindow::MainWindow(int id_user, const QString& username, const QStringList& roles, QWidget* parent)
    : QMainWindow(parent), m_currentUserId(id_user), m_currentUsername(username), m_currentRoles(roles) {
    setWindowTitle(QString("ИС Грузоперевозки — Вошел как: %1 (%2)").arg(username, roles.join(", ")));
    resize(900, 600);

    // Инициализация указателей
    m_logoutRequested = false;
    m_reportSalaryModel = nullptr;
    m_reportTransModel = nullptr;

    setupUI();
}

MainWindow::~MainWindow() {}

void MainWindow::setupUI() {
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    m_tabWidget = new QTabWidget(this);
    mainLayout->addWidget(m_tabWidget);

    // 1. Вкладка настроек профиля (Доступна ВСЕМ)
    setupProfileTab();

    // 2. Вкладки в зависимости от ролей
    if (m_currentRoles.contains("Администратор")) {
        setupAdminUI();
    }
    
    if (m_currentRoles.contains("Администратор") || m_currentRoles.contains("Диспетчер")) {
        setupDispatcherUI();
    }

    if (m_currentRoles.contains("Водитель")) {
        m_currentDriverProfile = DBManager::instance().getDriverByUserId(m_currentUserId);
        setupDriverUI();
    }

    if (m_currentRoles.contains("Механик")) {
        setupMechanicUI();
    }

    // 3. Отчеты (Для Администратора и Диспетчера)
    if (m_currentRoles.contains("Администратор") || m_currentRoles.contains("Диспетчер")) {
        // Вкладка: Отчеты
        QWidget* reportsTab = new QWidget(this);
        QVBoxLayout* repLayout = new QVBoxLayout(reportsTab);
        
        repLayout->addWidget(new QLabel("<b>Зарплатная ведомость водителей (salary_statement)</b>", this));
        m_reportSalaryTable = new QTableView(this);
        m_reportSalaryTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        repLayout->addWidget(m_reportSalaryTable);

        repLayout->addWidget(new QLabel("<b>Сводный отчет по перевозкам (transportation_summary)</b>", this));
        m_reportTransTable = new QTableView(this);
        m_reportTransTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        repLayout->addWidget(m_reportTransTable);

        QPushButton* refreshRepBtn = new QPushButton("Обновить отчеты", this);
        connect(refreshRepBtn, &QPushButton::clicked, this, &MainWindow::onRefreshReports);
        repLayout->addWidget(refreshRepBtn);

        m_tabWidget->addTab(reportsTab, "Отчеты и Аналитика");
        
        onRefreshReports(); // начальная загрузка отчетов
    }
}

// Настройка вкладки профиля
void MainWindow::setupProfileTab() {
    m_profileTab = new QWidget(this);
    QVBoxLayout* profLayout = new QVBoxLayout(m_profileTab);

    // Блок изменения пароля
    QFormLayout* pwdForm = new QFormLayout();
    pwdForm->addRow(new QLabel("<h3>Смена пароля аккаунта</h3>", this));
    
    m_newPasswordEdit = new QLineEdit(this);
    m_newPasswordEdit->setEchoMode(QLineEdit::Password);
    pwdForm->addRow("Новый пароль:", m_newPasswordEdit);
    
    m_confirmNewPasswordEdit = new QLineEdit(this);
    m_confirmNewPasswordEdit->setEchoMode(QLineEdit::Password);
    pwdForm->addRow("Подтвердите пароль:", m_confirmNewPasswordEdit);

    QPushButton* changePwdBtn = new QPushButton("Сменить пароль", this);
    changePwdBtn->setObjectName("primaryButton");
    connect(changePwdBtn, &QPushButton::clicked, this, &MainWindow::onChangePasswordClicked);
    pwdForm->addRow("", changePwdBtn);
    profLayout->addLayout(pwdForm);

    profLayout->addWidget(new QLabel("<hr>", this));

    // Блок изменения данных водителя (доступен, если текущий пользователь является водителем)
    m_profDriverDetailsWidget = new QWidget(this);
    QFormLayout* drvForm = new QFormLayout(m_profDriverDetailsWidget);
    drvForm->setContentsMargins(0, 0, 0, 0);
    drvForm->addRow(new QLabel("<h3>Регистрационные данные водителя</h3>", this));

    m_profLastNameEdit = new QLineEdit(m_profDriverDetailsWidget);
    m_profFirstNameEdit = new QLineEdit(m_profDriverDetailsWidget);
    m_profMiddleNameEdit = new QLineEdit(m_profDriverDetailsWidget);
    m_profExperienceSpin = new QSpinBox(m_profDriverDetailsWidget);
    m_profExperienceSpin->setRange(0, 80);

    drvForm->addRow("Фамилия:", m_profLastNameEdit);
    drvForm->addRow("Имя:", m_profFirstNameEdit);
    drvForm->addRow("Отчество:", m_profMiddleNameEdit);
    drvForm->addRow("Стаж (лет):", m_profExperienceSpin);

    QPushButton* saveDetailsBtn = new QPushButton("Сохранить ФИО и стаж", this);
    saveDetailsBtn->setObjectName("primaryButton");
    connect(saveDetailsBtn, &QPushButton::clicked, this, &MainWindow::onSaveProfileDetailsClicked);
    drvForm->addRow("", saveDetailsBtn);

    profLayout->addWidget(m_profDriverDetailsWidget);

    if (m_currentRoles.contains("Водитель")) {
        // Подгружаем текущие данные водителя
        m_currentDriverProfile = DBManager::instance().getDriverByUserId(m_currentUserId);
        if (m_currentDriverProfile.id_driver > 0) {
            m_profLastNameEdit->setText(m_currentDriverProfile.last_name);
            m_profFirstNameEdit->setText(m_currentDriverProfile.first_name);
            m_profMiddleNameEdit->setText(m_currentDriverProfile.middle_name);
            m_profExperienceSpin->setValue(m_currentDriverProfile.experience);
        }
    } else {
        m_profDriverDetailsWidget->setVisible(false);
    }

    // Разделитель перед кнопкой выхода
    QFrame* separator = new QFrame(this);
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    profLayout->addWidget(separator);

    QPushButton* logoutBtn = new QPushButton("Выйти из аккаунта", this);
    logoutBtn->setObjectName("secondaryButton");
    logoutBtn->setStyleSheet("background-color: #fef9c3; color: #854d0e; font-weight: bold; padding: 10px;");
    connect(logoutBtn, &QPushButton::clicked, this, [this]() {
        if (QMessageBox::question(this, "Выход", "Вы действительно хотите выйти из аккаунта?", QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
            m_logoutRequested = true;
            this->close();
        }
    });
    profLayout->addWidget(logoutBtn);

    profLayout->addStretch();
    m_tabWidget->addTab(m_profileTab, "Мой профиль");
}

// Настройка интерфейса Администратора
void MainWindow::setupAdminUI() {
    QWidget* adminTab = new QWidget(this);
    QVBoxLayout* adminLayout = new QVBoxLayout(adminTab);

    adminLayout->addWidget(new QLabel("<h3>Управление пользователями и ролями</h3>", this));

    m_usersTable = new QTableWidget(this);
    m_usersTable->setColumnCount(3);
    m_usersTable->setHorizontalHeaderLabels(QStringList() << "ID" << "Логин" << "Роли");
    m_usersTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_usersTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_usersTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_usersTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    adminLayout->addWidget(m_usersTable);

    QFormLayout* roleForm = new QFormLayout();
    m_userRolesCombo = new QComboBox(this);
    // Добавим варианты ролей для изменения
    m_userRolesCombo->addItem("Администратор");
    m_userRolesCombo->addItem("Диспетчер");
    m_userRolesCombo->addItem("Водитель");
    m_userRolesCombo->addItem("Механик");
    roleForm->addRow("Назначить роль:", m_userRolesCombo);

    QHBoxLayout* adminBtnLayout = new QHBoxLayout();
    m_saveUserRolesBtn = new QPushButton("Сохранить роль", this);
    connect(m_saveUserRolesBtn, &QPushButton::clicked, this, &MainWindow::onAdminSaveUserRoles);
    adminBtnLayout->addWidget(m_saveUserRolesBtn);

    m_deleteUserBtn = new QPushButton("Удалить пользователя", this);
    m_deleteUserBtn->setStyleSheet("background-color: #ffcccc; color: #cc0000;");
    connect(m_deleteUserBtn, &QPushButton::clicked, this, &MainWindow::onAdminDeleteUser);
    adminBtnLayout->addWidget(m_deleteUserBtn);

    QPushButton* refreshUsersBtn = new QPushButton("Обновить список пользователей", this);
    connect(refreshUsersBtn, &QPushButton::clicked, this, &MainWindow::onAdminRefreshUsers);
    adminBtnLayout->addWidget(refreshUsersBtn);

    adminLayout->addLayout(roleForm);
    adminLayout->addLayout(adminBtnLayout);

    // Разделитель
    QFrame* separator = new QFrame(this);
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    adminLayout->addWidget(separator);

    adminLayout->addWidget(new QLabel("<h3>Создание нового пользователя</h3>", this));

    QFormLayout* regForm = new QFormLayout();
    m_adminNewUserLoginEdit = new QLineEdit(this);
    m_adminNewUserLoginEdit->setPlaceholderText("Введите логин");
    regForm->addRow("Логин:", m_adminNewUserLoginEdit);

    m_adminNewUserPassEdit = new QLineEdit(this);
    m_adminNewUserPassEdit->setEchoMode(QLineEdit::Password);
    m_adminNewUserPassEdit->setPlaceholderText("Введите пароль");
    regForm->addRow("Пароль:", m_adminNewUserPassEdit);

    m_adminNewUserConfirmPassEdit = new QLineEdit(this);
    m_adminNewUserConfirmPassEdit->setEchoMode(QLineEdit::Password);
    m_adminNewUserConfirmPassEdit->setPlaceholderText("Повторите пароль");
    regForm->addRow("Подтверждение:", m_adminNewUserConfirmPassEdit);
    adminLayout->addLayout(regForm);

    QHBoxLayout* rolesLayout = new QHBoxLayout();
    m_adminNewUserAdminCheck = new QCheckBox("Администратор", this);
    m_adminNewUserDispCheck = new QCheckBox("Диспетчер", this);
    m_adminNewUserDriverCheck = new QCheckBox("Водитель", this);
    m_adminNewUserMechCheck = new QCheckBox("Механик", this);
    rolesLayout->addWidget(m_adminNewUserAdminCheck);
    rolesLayout->addWidget(m_adminNewUserDispCheck);
    rolesLayout->addWidget(m_adminNewUserDriverCheck);
    rolesLayout->addWidget(m_adminNewUserMechCheck);
    adminLayout->addLayout(rolesLayout);

    // Дополнительные поля водителя
    m_adminNewUserDriverWidget = new QWidget(this);
    QFormLayout* drvRegForm = new QFormLayout(m_adminNewUserDriverWidget);
    drvRegForm->setContentsMargins(0, 0, 0, 0);
    
    m_adminNewUserLastNameEdit = new QLineEdit(m_adminNewUserDriverWidget);
    m_adminNewUserFirstNameEdit = new QLineEdit(m_adminNewUserDriverWidget);
    m_adminNewUserMiddleNameEdit = new QLineEdit(m_adminNewUserDriverWidget);
    m_adminNewUserExpSpin = new QSpinBox(m_adminNewUserDriverWidget);
    m_adminNewUserExpSpin->setRange(0, 80);

    drvRegForm->addRow("Фамилия водителя:", m_adminNewUserLastNameEdit);
    drvRegForm->addRow("Имя водителя:", m_adminNewUserFirstNameEdit);
    drvRegForm->addRow("Отчество водителя:", m_adminNewUserMiddleNameEdit);
    drvRegForm->addRow("Стаж работы (лет):", m_adminNewUserExpSpin);
    
    m_adminNewUserDriverWidget->setVisible(false);
    adminLayout->addWidget(m_adminNewUserDriverWidget);

    QPushButton* createUserBtn = new QPushButton("Создать пользователя", this);
    createUserBtn->setObjectName("primaryButton");
    adminLayout->addWidget(createUserBtn);

    connect(m_adminNewUserDriverCheck, &QCheckBox::toggled, this, &MainWindow::onAdminDriverRoleToggled);
    connect(createUserBtn, &QPushButton::clicked, this, &MainWindow::onAdminCreateUser);

    m_tabWidget->addTab(adminTab, "Администрирование");

    onAdminRefreshUsers(); // Начальная загрузка пользователей
}

// Настройка интерфейса Диспетчера (и Администратора)
void MainWindow::setupDispatcherUI() {
    // Вкладка: Водители
    QWidget* drvTab = new QWidget(this);
    QVBoxLayout* drvLayout = new QVBoxLayout(drvTab);
    m_driversTable = new QTableWidget(this);
    m_driversTable->setColumnCount(5);
    m_driversTable->setHorizontalHeaderLabels(QStringList() << "ID" << "ФИО" << "Стаж" << "User ID" << "Аккаунт");
    m_driversTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_driversTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_driversTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_driversTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    drvLayout->addWidget(m_driversTable);

    QFormLayout* drvForm = new QFormLayout();
    m_driverLastNameEdit = new QLineEdit(this);
    m_driverFirstNameEdit = new QLineEdit(this);
    m_driverMiddleNameEdit = new QLineEdit(this);
    m_driverExperienceSpin = new QSpinBox(this);
    m_driverExperienceSpin->setRange(0, 80);
    m_driverUserCombo = new QComboBox(this);
    
    drvForm->addRow("Фамилия:", m_driverLastNameEdit);
    drvForm->addRow("Имя:", m_driverFirstNameEdit);
    drvForm->addRow("Отчество:", m_driverMiddleNameEdit);
    drvForm->addRow("Стаж работы:", m_driverExperienceSpin);
    drvForm->addRow("Учетная запись:", m_driverUserCombo);
    drvLayout->addLayout(drvForm);

    QHBoxLayout* drvBtns = new QHBoxLayout();
    QPushButton* addDrvBtn = new QPushButton("Добавить водителя", this);
    QPushButton* editDrvBtn = new QPushButton("Изменить водителя", this);
    QPushButton* delDrvBtn = new QPushButton("Удалить водителя", this);
    QPushButton* refDrvBtn = new QPushButton("Обновить", this);
    drvBtns->addWidget(addDrvBtn);
    drvBtns->addWidget(editDrvBtn);
    drvBtns->addWidget(delDrvBtn);
    drvBtns->addWidget(refDrvBtn);
    drvLayout->addLayout(drvBtns);

    if (!m_currentRoles.contains("Администратор")) {
        delDrvBtn->setVisible(false);
    }

    connect(addDrvBtn, &QPushButton::clicked, this, &MainWindow::onAddDriver);
    connect(editDrvBtn, &QPushButton::clicked, this, &MainWindow::onEditDriver);
    connect(delDrvBtn, &QPushButton::clicked, this, &MainWindow::onDeleteDriver);
    connect(refDrvBtn, &QPushButton::clicked, this, &MainWindow::onRefreshDrivers);
    m_tabWidget->addTab(drvTab, "Справочник Водителей");

    // Вкладка: Маршруты
    QWidget* routesTab = new QWidget(this);
    QVBoxLayout* rtLayout = new QVBoxLayout(routesTab);
    m_routesTable = new QTableWidget(this);
    m_routesTable->setColumnCount(4);
    m_routesTable->setHorizontalHeaderLabels(QStringList() << "ID" << "Название маршрута" << "Расстояние (км)" << "Базовый тариф (руб)");
    m_routesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_routesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_routesTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_routesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    rtLayout->addWidget(m_routesTable);

    QFormLayout* rtForm = new QFormLayout();
    m_routeNameEdit = new QLineEdit(this);
    m_routeDistanceSpin = new QDoubleSpinBox(this);
    m_routeDistanceSpin->setRange(1.0, 20000.0);
    m_routeBasePaySpin = new QDoubleSpinBox(this);
    m_routeBasePaySpin->setRange(0.0, 1000000.0);
    rtForm->addRow("Название:", m_routeNameEdit);
    rtForm->addRow("Расстояние (км):", m_routeDistanceSpin);
    rtForm->addRow("Базовая оплата водителя:", m_routeBasePaySpin);
    rtLayout->addLayout(rtForm);

    QHBoxLayout* rtBtns = new QHBoxLayout();
    QPushButton* addRtBtn = new QPushButton("Добавить маршрут", this);
    QPushButton* editRtBtn = new QPushButton("Изменить маршрут", this);
    QPushButton* delRtBtn = new QPushButton("Удалить маршрут", this);
    QPushButton* refRtBtn = new QPushButton("Обновить", this);
    rtBtns->addWidget(addRtBtn);
    rtBtns->addWidget(editRtBtn);
    rtBtns->addWidget(delRtBtn);
    rtBtns->addWidget(refRtBtn);
    rtLayout->addLayout(rtBtns);

    connect(addRtBtn, &QPushButton::clicked, this, &MainWindow::onAddRoute);
    connect(editRtBtn, &QPushButton::clicked, this, &MainWindow::onEditRoute);
    connect(delRtBtn, &QPushButton::clicked, this, &MainWindow::onDeleteRoute);
    connect(refRtBtn, &QPushButton::clicked, this, &MainWindow::onRefreshRoutes);
    m_tabWidget->addTab(routesTab, "Справочник Маршрутов");

    // Вкладка: Перевозки
    QWidget* transTab = new QWidget(this);
    QVBoxLayout* trLayout = new QVBoxLayout(transTab);
    m_transTable = new QTableWidget(this);
    m_transTable->setColumnCount(4);
    m_transTable->setHorizontalHeaderLabels(QStringList() << "ID перевозки" << "Маршрут" << "Отправление" << "Прибытие");
    m_transTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_transTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_transTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_transTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    trLayout->addWidget(m_transTable);

    QFormLayout* trForm = new QFormLayout();
    m_transRouteCombo = new QComboBox(this);
    m_transDepEdit = new QDateTimeEdit(QDateTime::currentDateTime(), this);
    m_transArrEdit = new QDateTimeEdit(QDateTime::currentDateTime(), this);
    m_transCompletedCheck = new QCheckBox("Рейс завершен (установить дату прибытия)", this);
    trForm->addRow("Маршрут:", m_transRouteCombo);
    trForm->addRow("Дата отправления:", m_transDepEdit);
    trForm->addRow("Дата прибытия:", m_transArrEdit);
    trForm->addRow("", m_transCompletedCheck);
    trLayout->addLayout(trForm);

    QHBoxLayout* trBtns = new QHBoxLayout();
    QPushButton* addTrBtn = new QPushButton("Создать рейс", this);
    QPushButton* editTrBtn = new QPushButton("Изменить рейс", this);
    QPushButton* delTrBtn = new QPushButton("Удалить рейс", this);
    QPushButton* refTrBtn = new QPushButton("Обновить", this);
    trBtns->addWidget(addTrBtn);
    trBtns->addWidget(editTrBtn);
    trBtns->addWidget(delTrBtn);
    trBtns->addWidget(refTrBtn);
    trLayout->addLayout(trBtns);

    if (!m_currentRoles.contains("Администратор")) {
        delTrBtn->setVisible(false);
    }

    connect(addTrBtn, &QPushButton::clicked, this, &MainWindow::onAddTransportation);
    connect(editTrBtn, &QPushButton::clicked, this, &MainWindow::onEditTransportation);
    connect(delTrBtn, &QPushButton::clicked, this, &MainWindow::onDeleteTransportation);
    connect(refTrBtn, &QPushButton::clicked, this, &MainWindow::onRefreshTransportations);
    m_tabWidget->addTab(transTab, "Рейсы (Перевозки)");

    // Вкладка: Назначение водителей и премии
    QWidget* assignTab = new QWidget(this);
    QVBoxLayout* asLayout = new QVBoxLayout(assignTab);
    
    asLayout->addWidget(new QLabel("<h3>Текущие назначения водителей на рейсы</h3>", this));
    m_assignTable = new QTableWidget(this);
    m_assignTable->setColumnCount(5);
    m_assignTable->setHorizontalHeaderLabels(QStringList() << "ID Водителя" << "Водитель" << "ID Рейса" << "Оплата" << "Премия");
    m_assignTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_assignTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_assignTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_assignTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    asLayout->addWidget(m_assignTable);

    QFormLayout* asForm = new QFormLayout();
    m_assignDriverCombo = new QComboBox(this);
    m_assignTransCombo = new QComboBox(this);
    asForm->addRow("Назначить водителя:", m_assignDriverCombo);
    asForm->addRow("На рейс ID:", m_assignTransCombo);

    QPushButton* assignBtn = new QPushButton("Назначить (оплата рассчитается триггером по стажу)", this);
    connect(assignBtn, &QPushButton::clicked, this, &MainWindow::onAssignDriver);
    asForm->addRow("", assignBtn);

    asForm->addRow(new QLabel("<h3>Начисление премии водителям</h3>", this));
    m_bonusAmountSpin = new QDoubleSpinBox(this);
    m_bonusAmountSpin->setRange(0.0, 500000.0);
    m_bonusReasonEdit = new QLineEdit(this);
    asForm->addRow("Размер премии (руб):", m_bonusAmountSpin);
    asForm->addRow("Причина премии:", m_bonusReasonEdit);
    
    QPushButton* addBonusBtn = new QPushButton("Выплатить премию выбранному назначению", this);
    connect(addBonusBtn, &QPushButton::clicked, this, &MainWindow::onAddBonus);
    asForm->addRow("", addBonusBtn);

    QPushButton* removeAssignBtn = new QPushButton("Снять водителя с рейса", this);
    removeAssignBtn->setStyleSheet("background-color: #ffcccc; color: #cc0000;");
    connect(removeAssignBtn, &QPushButton::clicked, this, &MainWindow::onRemoveAssignment);
    asForm->addRow("", removeAssignBtn);

    asLayout->addLayout(asForm);
    m_tabWidget->addTab(assignTab, "Экипаж и Премии");

    // Автозаполнение полей ввода при выборе строки в таблицах (для удобства и простоты интерфейса)
    connect(m_driversTable, &QTableWidget::itemSelectionChanged, this, [this]() {
        int row = m_driversTable->currentRow();
        if (row < 0) return;
        int id_driver = m_driversTable->item(row, 0)->text().toInt();
        Driver d = DBManager::instance().getDriverById(id_driver);
        m_driverLastNameEdit->setText(d.last_name);
        m_driverFirstNameEdit->setText(d.first_name);
        m_driverMiddleNameEdit->setText(d.middle_name);
        m_driverExperienceSpin->setValue(d.experience);
        
        // Временно отключаем сигналы, чтобы не зациклить
        m_driverUserCombo->blockSignals(true);
        m_driverUserCombo->clear();
        QList<User> users = DBManager::instance().getAvailableUsersForDrivers(d.id_user);
        for (const auto& u : users) {
            m_driverUserCombo->addItem(u.login, u.id_user);
        }
        int index = m_driverUserCombo->findData(d.id_user);
        if (index >= 0) m_driverUserCombo->setCurrentIndex(index);
        else m_driverUserCombo->setCurrentIndex(-1);
        m_driverUserCombo->blockSignals(false);
    });

    connect(m_routesTable, &QTableWidget::itemSelectionChanged, this, [this]() {
        int row = m_routesTable->currentRow();
        if (row < 0) return;
        int id_route = m_routesTable->item(row, 0)->text().toInt();
        Route r = DBManager::instance().getRouteById(id_route);
        m_routeNameEdit->setText(r.route_name);
        m_routeDistanceSpin->setValue(r.distance);
        m_routeBasePaySpin->setValue(r.base_payment);
    });

    connect(m_transTable, &QTableWidget::itemSelectionChanged, this, [this]() {
        int row = m_transTable->currentRow();
        if (row < 0) return;
        int id_trans = m_transTable->item(row, 0)->text().toInt();
        Transportation t = DBManager::instance().getTransportationById(id_trans);
        int idx = m_transRouteCombo->findData(t.id_route);
        if (idx >= 0) m_transRouteCombo->setCurrentIndex(idx);
        m_transDepEdit->setDateTime(t.departure_date);
        if (t.arrival_date.isValid()) {
            m_transArrEdit->setDateTime(t.arrival_date);
            m_transCompletedCheck->setChecked(true);
            m_transArrEdit->setEnabled(true);
        } else {
            m_transCompletedCheck->setChecked(false);
            m_transArrEdit->setEnabled(false);
        }
    });

    connect(m_assignTable, &QTableWidget::itemSelectionChanged, this, [this]() {
        int row = m_assignTable->currentRow();
        if (row < 0) return;
        int id_driver = m_assignTable->item(row, 0)->text().toInt();
        int id_trans = m_assignTable->item(row, 2)->text().toInt();
        
        int dIdx = m_assignDriverCombo->findData(id_driver);
        if (dIdx >= 0) m_assignDriverCombo->setCurrentIndex(dIdx);
        
        int tIdx = m_assignTransCombo->findData(id_trans);
        if (tIdx >= 0) m_assignTransCombo->setCurrentIndex(tIdx);
    });

    connect(m_tabWidget, &QTabWidget::currentChanged, this, [this](int index) {
        // Автообновление списков при переходе на вкладки
        if (index == m_tabWidget->indexOf(m_driversTable->parentWidget())) onRefreshDrivers();
        if (index == m_tabWidget->indexOf(m_routesTable->parentWidget())) onRefreshRoutes();
        if (index == m_tabWidget->indexOf(m_transTable->parentWidget())) onRefreshTransportations();
        if (index == m_tabWidget->indexOf(m_assignTable->parentWidget())) onRefreshAssignments();
    });

    onRefreshDrivers();
    onRefreshRoutes();
    onRefreshTransportations();
    onRefreshAssignments();
}

// Настройка интерфейса Водителя
void MainWindow::setupDriverUI() {
    QWidget* drvTab = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(drvTab);

    layout->addWidget(new QLabel("<h3>Список моих рейсов</h3>", this));

    m_myTripsTable = new QTableWidget(this);
    m_myTripsTable->setColumnCount(6);
    m_myTripsTable->setHorizontalHeaderLabels(QStringList() << "Маршрут" << "Отправление" << "Прибытие" << "Оплата" << "Премия" << "Причина премии");
    m_myTripsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_myTripsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout->addWidget(m_myTripsTable);

    // Кнопка обновления
    QPushButton* refreshBtn = new QPushButton("Обновить список моих рейсов", this);
    layout->addWidget(refreshBtn);

    m_tabWidget->addTab(drvTab, "Мои рейсы");

    auto refreshMyTrips = [this]() {
        m_myTripsTable->setRowCount(0);
        if (m_currentDriverProfile.id_driver == 0) return;

        QList<DriverTransportation> myAssigns = DBManager::instance().getAssignmentsForDriver(m_currentDriverProfile.id_driver);
        for (const auto& a : myAssigns) {
            Transportation t = DBManager::instance().getTransportationById(a.id_transportation);
            Route r = DBManager::instance().getRouteById(t.id_route);

            int row = m_myTripsTable->rowCount();
            m_myTripsTable->insertRow(row);

            m_myTripsTable->setItem(row, 0, new QTableWidgetItem(r.route_name));
            m_myTripsTable->setItem(row, 1, new QTableWidgetItem(t.departure_date.toString("dd.MM.yyyy hh:mm")));
            m_myTripsTable->setItem(row, 2, new QTableWidgetItem(t.arrival_date.isValid() ? t.arrival_date.toString("dd.MM.yyyy hh:mm") : "В пути"));
            m_myTripsTable->setItem(row, 3, new QTableWidgetItem(QString::number(a.payment, 'f', 2)));
            m_myTripsTable->setItem(row, 4, new QTableWidgetItem(QString::number(a.bonus_amount, 'f', 2)));
            m_myTripsTable->setItem(row, 5, new QTableWidgetItem(a.bonus_reason));
        }
    };

    connect(refreshBtn, &QPushButton::clicked, this, refreshMyTrips);
    refreshMyTrips();
}

// Настройка интерфейса Механика
void MainWindow::setupMechanicUI() {
    QWidget* mechTab = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(mechTab);

    layout->addWidget(new QLabel("<h3>Рейсы, находящиеся в пути (для механика)</h3>", this));

    m_currentTripsTable = new QTableWidget(this);
    m_currentTripsTable->setColumnCount(4);
    m_currentTripsTable->setHorizontalHeaderLabels(QStringList() << "ID Рейса" << "Маршрут" << "Отправление" << "Назначенные водители");
    m_currentTripsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_currentTripsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout->addWidget(m_currentTripsTable);

    QPushButton* refreshBtn = new QPushButton("Обновить список текущих рейсов", this);
    layout->addWidget(refreshBtn);

    m_tabWidget->addTab(mechTab, "Контроль транспорта");

    auto refreshCurrentTrips = [this]() {
        m_currentTripsTable->setRowCount(0);
        QList<Transportation> all = DBManager::instance().getAllTransportations();
        for (const auto& t : all) {
            if (t.isInProgress()) {
                Route r = DBManager::instance().getRouteById(t.id_route);
                QList<DriverTransportation> assigns = DBManager::instance().getAssignmentsForTransportation(t.id_transportation);
                QStringList driverNames;
                for (const auto& a : assigns) {
                    Driver d = DBManager::instance().getDriverById(a.id_driver);
                    driverNames.append(d.getFullName());
                }

                int row = m_currentTripsTable->rowCount();
                m_currentTripsTable->insertRow(row);

                m_currentTripsTable->setItem(row, 0, new QTableWidgetItem(QString::number(t.id_transportation)));
                m_currentTripsTable->setItem(row, 1, new QTableWidgetItem(r.route_name));
                m_currentTripsTable->setItem(row, 2, new QTableWidgetItem(t.departure_date.toString("dd.MM.yyyy hh:mm")));
                m_currentTripsTable->setItem(row, 3, new QTableWidgetItem(driverNames.isEmpty() ? "Не назначены" : driverNames.join(", ")));
            }
        }
    };

    connect(refreshBtn, &QPushButton::clicked, this, refreshCurrentTrips);
    refreshCurrentTrips();
}

// ==========================================
// СЛОТЫ: МОЙ ПРОФИЛЬ
// ==========================================

void MainWindow::onChangePasswordClicked() {
    QString pwd = m_newPasswordEdit->text();
    QString conf = m_confirmNewPasswordEdit->text();

    if (pwd.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Пароль не может быть пустым!");
        return;
    }

    if (pwd != conf) {
        QMessageBox::warning(this, "Ошибка", "Пароли не совпадают!");
        return;
    }

    // Регулярные выражения на клиенте соответствуют CHECK-ограничению БД
    QRegularExpression rxDigit("[0-9]");
    QRegularExpression rxUpper("[A-ZА-Я]");
    QRegularExpression rxSpecial("[^a-zA-Z0-9а-яА-Я\\s]");

    if (pwd.length() < 8 || !pwd.contains(rxDigit) || !pwd.contains(rxUpper) || !pwd.contains(rxSpecial)) {
        QMessageBox::warning(this, "Ошибка", "Пароль не удовлетворяет требованиям сложности!\n"
                                             "Требования: длина от 8 символов, заглавная буква, цифра и спецсимвол.");
        return;
    }

    if (DBManager::instance().updateUserPassword(m_currentUserId, pwd)) {
        QMessageBox::information(this, "Успех", "Пароль успешно изменен!");
        m_newPasswordEdit->clear();
        m_confirmNewPasswordEdit->clear();
    } else {
        QMessageBox::critical(this, "Ошибка СУБД", "Не удалось обновить пароль: " + DBManager::instance().lastError());
    }
}

void MainWindow::onSaveProfileDetailsClicked() {
    if (m_currentDriverProfile.id_driver == 0) return;

    m_currentDriverProfile.last_name = m_profLastNameEdit->text().trimmed();
    m_currentDriverProfile.first_name = m_profFirstNameEdit->text().trimmed();
    m_currentDriverProfile.middle_name = m_profMiddleNameEdit->text().trimmed();
    m_currentDriverProfile.experience = m_profExperienceSpin->value();

    if (m_currentDriverProfile.last_name.isEmpty() || m_currentDriverProfile.first_name.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Фамилия и имя водителя должны быть заполнены!");
        return;
    }

    if (DBManager::instance().updateDriver(m_currentDriverProfile)) {
        QMessageBox::information(this, "Успех", "Данные профиля водителя сохранены!");
    } else {
        QMessageBox::critical(this, "Ошибка СУБД", "Не удалось сохранить данные профиля: " + DBManager::instance().lastError());
    }
}

// ==========================================
// СЛОТЫ: АДМИНИСТРАТОР
// ==========================================

void MainWindow::onAdminRefreshUsers() {
    m_usersTable->setRowCount(0);
    QList<User> list = DBManager::instance().getAllUsers();
    for (const auto& u : list) {
        int row = m_usersTable->rowCount();
        m_usersTable->insertRow(row);

        m_usersTable->setItem(row, 0, new QTableWidgetItem(QString::number(u.id_user)));
        m_usersTable->setItem(row, 1, new QTableWidgetItem(u.login));
        m_usersTable->setItem(row, 2, new QTableWidgetItem(DBManager::instance().getUserRoles(u.id_user).join(", ")));
    }
}

void MainWindow::onAdminDeleteUser() {
    int row = m_usersTable->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "Внимание", "Выберите пользователя для удаления!");
        return;
    }
    int id_user = m_usersTable->item(row, 0)->text().toInt();
    if (id_user == m_currentUserId) {
        QMessageBox::warning(this, "Внимание", "Вы не можете удалить свою собственную учетную запись!");
        return;
    }

    if (QMessageBox::question(this, "Подтверждение", "Удалить пользователя? Это также удалит/очистит его связи с ролями и водителями.", QMessageBox::Yes|QMessageBox::No) == QMessageBox::Yes) {
        if (DBManager::instance().deleteUser(id_user)) {
            onAdminRefreshUsers();
            onRefreshDrivers(); // обновление привязок водителей
        } else {
            QMessageBox::critical(this, "Ошибка СУБД", "Ошибка при удалении: " + DBManager::instance().lastError());
        }
    }
}

void MainWindow::onAdminSaveUserRoles() {
    int row = m_usersTable->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "Внимание", "Выберите пользователя!");
        return;
    }
    int id_user = m_usersTable->item(row, 0)->text().toInt();
    QStringList newRoles;
    newRoles.append(m_userRolesCombo->currentText());

    if (DBManager::instance().updateUserRoles(id_user, newRoles)) {
        QMessageBox::information(this, "Успех", "Роль пользователя успешно обновлена!");
        onAdminRefreshUsers();
    } else {
        QMessageBox::critical(this, "Ошибка СУБД", "Ошибка: " + DBManager::instance().lastError());
    }
}

// ==========================================
// СЛОТЫ: ДИСПЕТЧЕР (CRUD ВОДИТЕЛЕЙ)
// ==========================================

void MainWindow::onRefreshDrivers() {
    m_driversTable->setRowCount(0);
    m_driverUserCombo->clear();
    // Загрузка комбобокса пользователей (только свободные пользователи с ролью "Водитель")
    QList<User> users = DBManager::instance().getAvailableUsersForDrivers(0);
    for (const auto& u : users) {
        m_driverUserCombo->addItem(u.login, u.id_user);
    }

    QList<User> allUsers = DBManager::instance().getAllUsers();
    QList<Driver> list = DBManager::instance().getAllDrivers();
    for (const auto& d : list) {
        int row = m_driversTable->rowCount();
        m_driversTable->insertRow(row);

        m_driversTable->setItem(row, 0, new QTableWidgetItem(QString::number(d.id_driver)));
        m_driversTable->setItem(row, 1, new QTableWidgetItem(d.getFullName()));
        m_driversTable->setItem(row, 2, new QTableWidgetItem(QString::number(d.experience)));
        m_driversTable->setItem(row, 3, new QTableWidgetItem(d.id_user > 0 ? QString::number(d.id_user) : "Нет"));

        // Логин аккаунта
        QString account = "Нет";
        if (d.id_user > 0) {
            for (const auto& u : allUsers) {
                if (u.id_user == d.id_user) {
                    account = u.login;
                    break;
                }
            }
        }
        m_driversTable->setItem(row, 4, new QTableWidgetItem(account));
    }
}

void MainWindow::onAddDriver() {
    Driver d;
    d.last_name = m_driverLastNameEdit->text().trimmed();
    d.first_name = m_driverFirstNameEdit->text().trimmed();
    d.middle_name = m_driverMiddleNameEdit->text().trimmed();
    d.experience = m_driverExperienceSpin->value();
    d.id_user = m_driverUserCombo->currentData().toInt();

    if (d.last_name.isEmpty() || d.first_name.isEmpty()) {
        QMessageBox::warning(this, "Внимание", "Заполните Фамилию и Имя водителя!");
        return;
    }

    if (d.id_user <= 0) {
        QMessageBox::warning(this, "Внимание", "Необходимо связать водителя с учетной записью пользователя!");
        return;
    }

    int dummy = 0;
    if (DBManager::instance().insertDriver(d, dummy)) {
        onRefreshDrivers();
        m_driverLastNameEdit->clear();
        m_driverFirstNameEdit->clear();
        m_driverMiddleNameEdit->clear();
        m_driverExperienceSpin->setValue(0);
    } else {
        QMessageBox::critical(this, "Ошибка СУБД", "Не удалось добавить водителя: " + DBManager::instance().lastError());
    }
}

void MainWindow::onEditDriver() {
    int row = m_driversTable->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "Внимание", "Выберите водителя для редактирования!");
        return;
    }
    Driver d;
    d.id_driver = m_driversTable->item(row, 0)->text().toInt();
    d.last_name = m_driverLastNameEdit->text().trimmed();
    d.first_name = m_driverFirstNameEdit->text().trimmed();
    d.middle_name = m_driverMiddleNameEdit->text().trimmed();
    d.experience = m_driverExperienceSpin->value();
    d.id_user = m_driverUserCombo->currentData().toInt();

    if (d.last_name.isEmpty() || d.first_name.isEmpty()) {
        QMessageBox::warning(this, "Внимание", "Заполните Фамилию и Имя!");
        return;
    }

    if (d.id_user <= 0) {
        QMessageBox::warning(this, "Внимание", "Необходимо связать водителя с учетной записью пользователя!");
        return;
    }

    if (DBManager::instance().updateDriver(d)) {
        onRefreshDrivers();
    } else {
        QMessageBox::critical(this, "Ошибка СУБД", "Ошибка обновления: " + DBManager::instance().lastError());
    }
}

void MainWindow::onDeleteDriver() {
    int row = m_driversTable->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "Внимание", "Выберите водителя для удаления!");
        return;
    }
    int id_driver = m_driversTable->item(row, 0)->text().toInt();

    if (QMessageBox::question(this, "Подтверждение", "Удалить выбранного водителя?", QMessageBox::Yes|QMessageBox::No) == QMessageBox::Yes) {
        if (DBManager::instance().deleteDriver(id_driver)) {
            onRefreshDrivers();
        } else {
            QMessageBox::critical(this, "Ошибка СУБД", "Ошибка удаления: " + DBManager::instance().lastError());
        }
    }
}

// ==========================================
// СЛОТЫ: ДИСПЕТЧЕР (CRUD МАРШРУТОВ)
// ==========================================

void MainWindow::onRefreshRoutes() {
    m_routesTable->setRowCount(0);
    m_transRouteCombo->clear();

    QList<Route> list = DBManager::instance().getAllRoutes();
    for (const auto& r : list) {
        int row = m_routesTable->rowCount();
        m_routesTable->insertRow(row);

        m_routesTable->setItem(row, 0, new QTableWidgetItem(QString::number(r.id_route)));
        m_routesTable->setItem(row, 1, new QTableWidgetItem(r.route_name));
        m_routesTable->setItem(row, 2, new QTableWidgetItem(QString::number(r.distance, 'f', 2)));
        m_routesTable->setItem(row, 3, new QTableWidgetItem(QString::number(r.base_payment, 'f', 2)));

        m_transRouteCombo->addItem(r.route_name, r.id_route);
    }
}

void MainWindow::onAddRoute() {
    Route r;
    r.route_name = m_routeNameEdit->text().trimmed();
    r.distance = m_routeDistanceSpin->value();
    r.base_payment = m_routeBasePaySpin->value();

    if (r.route_name.isEmpty()) {
        QMessageBox::warning(this, "Внимание", "Заполните название маршрута!");
        return;
    }

    int dummy = 0;
    if (DBManager::instance().insertRoute(r, dummy)) {
        onRefreshRoutes();
        m_routeNameEdit->clear();
    } else {
        QMessageBox::critical(this, "Ошибка СУБД", "Не удалось добавить маршрут: " + DBManager::instance().lastError());
    }
}

void MainWindow::onEditRoute() {
    int row = m_routesTable->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "Внимание", "Выберите маршрут для редактирования!");
        return;
    }
    Route r;
    r.id_route = m_routesTable->item(row, 0)->text().toInt();
    r.route_name = m_routeNameEdit->text().trimmed();
    r.distance = m_routeDistanceSpin->value();
    r.base_payment = m_routeBasePaySpin->value();

    if (r.route_name.isEmpty()) {
        QMessageBox::warning(this, "Внимание", "Название не может быть пустым!");
        return;
    }

    if (DBManager::instance().updateRoute(r)) {
        onRefreshRoutes();
    } else {
        QMessageBox::critical(this, "Ошибка СУБД", "Ошибка обновления: " + DBManager::instance().lastError());
    }
}

void MainWindow::onDeleteRoute() {
    int row = m_routesTable->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "Внимание", "Выберите маршрут для удаления!");
        return;
    }
    int id_route = m_routesTable->item(row, 0)->text().toInt();

    if (QMessageBox::question(this, "Подтверждение", "Удалить маршрут? Это может быть запрещено, если на него назначены рейсы.", QMessageBox::Yes|QMessageBox::No) == QMessageBox::Yes) {
        if (DBManager::instance().deleteRoute(id_route)) {
            onRefreshRoutes();
        } else {
            QMessageBox::critical(this, "Ошибка СУБД", "Не удалось удалить маршрут. Убедитесь, что нет зависимых перевозок.\nКод ошибки: " + DBManager::instance().lastError());
        }
    }
}

// ==========================================
// СЛОТЫ: ДИСПЕТЧЕР (CRUD ПЕРЕВОЗОК)
// ==========================================

void MainWindow::onRefreshTransportations() {
    m_transTable->setRowCount(0);
    m_assignTransCombo->clear();

    QList<Transportation> list = DBManager::instance().getAllTransportations();
    for (const auto& t : list) {
        Route r = DBManager::instance().getRouteById(t.id_route);

        int row = m_transTable->rowCount();
        m_transTable->insertRow(row);

        m_transTable->setItem(row, 0, new QTableWidgetItem(QString::number(t.id_transportation)));
        m_transTable->setItem(row, 1, new QTableWidgetItem(r.route_name));
        m_transTable->setItem(row, 2, new QTableWidgetItem(t.departure_date.toString("dd.MM.yyyy hh:mm")));
        m_transTable->setItem(row, 3, new QTableWidgetItem(t.arrival_date.isValid() ? t.arrival_date.toString("dd.MM.yyyy hh:mm") : "В пути"));

        m_assignTransCombo->addItem(QString("Рейс %1: %2").arg(QString::number(t.id_transportation), r.route_name), t.id_transportation);
    }
}

void MainWindow::onAddTransportation() {
    if (m_transRouteCombo->count() == 0) {
        QMessageBox::warning(this, "Ошибка", "Сначала создайте хотя бы один маршрут!");
        return;
    }

    Transportation t;
    t.id_route = m_transRouteCombo->currentData().toInt();
    t.departure_date = m_transDepEdit->dateTime();
    
    if (m_transCompletedCheck->isChecked()) {
        t.arrival_date = m_transArrEdit->dateTime();
    }

    int dummy = 0;
    if (DBManager::instance().insertTransportation(t, dummy)) {
        onRefreshTransportations();
    } else {
        QMessageBox::critical(this, "Ошибка СУБД", "Ошибка создания рейса: " + DBManager::instance().lastError());
    }
}

void MainWindow::onEditTransportation() {
    int row = m_transTable->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "Внимание", "Выберите рейс для редактирования!");
        return;
    }

    Transportation t;
    t.id_transportation = m_transTable->item(row, 0)->text().toInt();
    t.id_route = m_transRouteCombo->currentData().toInt();
    t.departure_date = m_transDepEdit->dateTime();
    
    if (m_transCompletedCheck->isChecked()) {
        t.arrival_date = m_transArrEdit->dateTime();
    } else {
        t.arrival_date = QDateTime(); // В пути (NULL)
    }

    if (DBManager::instance().updateTransportation(t)) {
        onRefreshTransportations();
    } else {
        QMessageBox::critical(this, "Ошибка СУБД", "Ошибка: " + DBManager::instance().lastError());
    }
}

void MainWindow::onDeleteTransportation() {
    int row = m_transTable->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "Внимание", "Выберите рейс для удаления!");
        return;
    }
    int id_trans = m_transTable->item(row, 0)->text().toInt();

    if (QMessageBox::question(this, "Подтверждение", "Удалить этот рейс? Это сотрет все назначения водителей.", QMessageBox::Yes|QMessageBox::No) == QMessageBox::Yes) {
        if (DBManager::instance().deleteTransportation(id_trans)) {
            onRefreshTransportations();
            onRefreshAssignments();
        } else {
            QMessageBox::critical(this, "Ошибка", DBManager::instance().lastError());
        }
    }
}

// ==========================================
// СЛОТЫ: НАЗНАЧЕНИЯ И ПРЕМИИ
// ==========================================

void MainWindow::onRefreshAssignments() {
    m_assignTable->setRowCount(0);
    m_assignDriverCombo->clear();

    // Загрузка списка водителей для комбобокса
    QList<Driver> drivers = DBManager::instance().getAllDrivers();
    for (const auto& d : drivers) {
        m_assignDriverCombo->addItem(QString("%1 (стаж %2 лет)").arg(d.getFullName(), QString::number(d.experience)), d.id_driver);
    }

    // Заполняем таблицу назначений
    QList<Transportation> trans = DBManager::instance().getAllTransportations();
    for (const auto& t : trans) {
        QList<DriverTransportation> assigns = DBManager::instance().getAssignmentsForTransportation(t.id_transportation);
        for (const auto& a : assigns) {
            Driver d = DBManager::instance().getDriverById(a.id_driver);

            int row = m_assignTable->rowCount();
            m_assignTable->insertRow(row);

            m_assignTable->setItem(row, 0, new QTableWidgetItem(QString::number(a.id_driver)));
            m_assignTable->setItem(row, 1, new QTableWidgetItem(d.getFullName()));
            m_assignTable->setItem(row, 2, new QTableWidgetItem(QString::number(a.id_transportation)));
            m_assignTable->setItem(row, 3, new QTableWidgetItem(QString::number(a.payment, 'f', 2)));
            m_assignTable->setItem(row, 4, new QTableWidgetItem(QString("%1 (%2)").arg(QString::number(a.bonus_amount, 'f', 2), a.bonus_reason)));
        }
    }
}

void MainWindow::onAssignDriver() {
    if (m_assignDriverCombo->count() == 0 || m_assignTransCombo->count() == 0) {
        QMessageBox::warning(this, "Ошибка", "Должны существовать и водители, и рейсы!");
        return;
    }

    int driverId = m_assignDriverCombo->currentData().toInt();
    int transId = m_assignTransCombo->currentData().toInt();

    // Назначаем водителя на рейс. Оплату передаем -1.0, чтобы триггер в БД рассчитал её по стажу автоматически!
    if (DBManager::instance().assignDriverToTransportation(driverId, transId, -1.0)) {
        QMessageBox::information(this, "Успех", "Водитель успешно назначен на рейс! Зарплата рассчитана сервером БД.");
        onRefreshAssignments();
    } else {
        // Здесь мы поймаем ошибку от триггера PostgreSQL, например, если на рейс назначено уже 2 водителя!
        QMessageBox::critical(this, "Ошибка целостности (СУБД)", "Не удалось назначить водителя:\n" + DBManager::instance().lastError());
    }
}

void MainWindow::onRemoveAssignment() {
    int row = m_assignTable->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "Внимание", "Выберите строку назначения в таблице выше для снятия водителя с рейса!");
        return;
    }

    int driverId = m_assignTable->item(row, 0)->text().toInt();
    int transId = m_assignTable->item(row, 2)->text().toInt();

    if (QMessageBox::question(this, "Подтверждение", "Снять водителя с этого рейса?", QMessageBox::Yes|QMessageBox::No) == QMessageBox::Yes) {
        if (DBManager::instance().removeDriverFromTransportation(driverId, transId)) {
            onRefreshAssignments();
        } else {
            QMessageBox::critical(this, "Ошибка СУБД", DBManager::instance().lastError());
        }
    }
}

void MainWindow::onAddBonus() {
    int row = m_assignTable->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "Внимание", "Выберите назначение водителя в таблице, которому начисляется премия!");
        return;
    }

    int driverId = m_assignTable->item(row, 0)->text().toInt();
    int transId = m_assignTable->item(row, 2)->text().toInt();
    double bonus = m_bonusAmountSpin->value();
    QString reason = m_bonusReasonEdit->text().trimmed();

    if (bonus <= 0.0) {
        QMessageBox::warning(this, "Внимание", "Премия должна быть больше нуля!");
        return;
    }
    if (reason.isEmpty()) {
        QMessageBox::warning(this, "Внимание", "Укажите причину выплаты премии!");
        return;
    }

    if (DBManager::instance().updateAssignmentBonus(driverId, transId, bonus, reason)) {
        QMessageBox::information(this, "Успех", "Премия успешно начислена водителю за рейс!");
        onRefreshAssignments();
        m_bonusAmountSpin->setValue(0.0);
        m_bonusReasonEdit->clear();
    } else {
        QMessageBox::critical(this, "Ошибка СУБД", DBManager::instance().lastError());
    }
}

// ==========================================
// СЛОТЫ: ОТЧЕТЫ (VIEWS)
// ==========================================

void MainWindow::onRefreshReports() {
    // Освобождаем старые модели
    if (m_reportSalaryModel) delete m_reportSalaryModel;
    if (m_reportTransModel) delete m_reportTransModel;

    // Загружаем отчеты из представлений БД
    m_reportSalaryModel = DBManager::instance().getSalaryStatementModel(this);
    m_reportSalaryTable->setModel(m_reportSalaryModel);

    m_reportTransModel = DBManager::instance().getTransportationSummaryModel(this);
    m_reportTransTable->setModel(m_reportTransModel);
}

bool MainWindow::validatePassword(const QString& password, QString& errorMessage) {
    if (password.length() < 8) {
        errorMessage = "Длина пароля должна быть не менее 8 символов!";
        return false;
    }
    
    QRegularExpression rxDigit("[0-9]");
    if (!password.contains(rxDigit)) {
        errorMessage = "Пароль должен содержать хотя бы одну цифру!";
        return false;
    }
    
    QRegularExpression rxUpper("[A-ZА-Я]");
    if (!password.contains(rxUpper)) {
        errorMessage = "Пароль должен содержать хотя бы одну заглавную букву!";
        return false;
    }
    
    QRegularExpression rxSpecial("[^a-zA-Z0-9а-яА-Я\\s]");
    if (!password.contains(rxSpecial)) {
        errorMessage = "Пароль должен содержать хотя бы один специальный символ (знак пунктуации)!";
        return false;
    }
    
    return true;
}

void MainWindow::onAdminDriverRoleToggled(bool checked) {
    m_adminNewUserDriverWidget->setVisible(checked);
}

void MainWindow::onAdminCreateUser() {
    QString login = m_adminNewUserLoginEdit->text().trimmed();
    QString password = m_adminNewUserPassEdit->text();
    QString confirm = m_adminNewUserConfirmPassEdit->text();

    if (login.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "Внимание", "Логин и пароль не могут быть пустыми!");
        return;
    }

    if (password != confirm) {
        QMessageBox::warning(this, "Внимание", "Пароли не совпадают!");
        return;
    }

    QString pwdError;
    if (!validatePassword(password, pwdError)) {
        QMessageBox::warning(this, "Внимание", pwdError);
        return;
    }

    // Сбор выбранных ролей
    QStringList selectedRoles;
    if (m_adminNewUserAdminCheck->isChecked()) selectedRoles.append("Администратор");
    if (m_adminNewUserDispCheck->isChecked()) selectedRoles.append("Диспетчер");
    if (m_adminNewUserDriverCheck->isChecked()) selectedRoles.append("Водитель");
    if (m_adminNewUserMechCheck->isChecked()) selectedRoles.append("Механик");

    if (selectedRoles.isEmpty()) {
        QMessageBox::warning(this, "Внимание", "Выберите хотя бы одну роль!");
        return;
    }

    // Если водитель - проверяем заполненность ФИО
    if (m_adminNewUserDriverCheck->isChecked()) {
        if (m_adminNewUserLastNameEdit->text().trimmed().isEmpty() || m_adminNewUserFirstNameEdit->text().trimmed().isEmpty()) {
            QMessageBox::warning(this, "Внимание", "Заполните Фамилию и Имя водителя!");
            return;
        }
    }

    int newUserId = 0;
    if (DBManager::instance().registerUser(login, password, selectedRoles, newUserId)) {
        // Если была выбрана роль водителя, создаем его профиль
        if (m_adminNewUserDriverCheck->isChecked()) {
            Driver d;
            d.id_user = newUserId;
            d.last_name = m_adminNewUserLastNameEdit->text().trimmed();
            d.first_name = m_adminNewUserFirstNameEdit->text().trimmed();
            d.middle_name = m_adminNewUserMiddleNameEdit->text().trimmed();
            d.experience = m_adminNewUserExpSpin->value();

            int newDriverId = 0;
            if (!DBManager::instance().insertDriver(d, newDriverId)) {
                QMessageBox::warning(this, "Внимание", "Пользователь зарегистрирован, но не удалось создать профиль водителя: " + DBManager::instance().lastError());
            }
        }
        
        QMessageBox::information(this, "Успех", "Пользователь успешно зарегистрирован СУБД!");
        
        // Очищаем форму
        m_adminNewUserLoginEdit->clear();
        m_adminNewUserPassEdit->clear();
        m_adminNewUserConfirmPassEdit->clear();
        m_adminNewUserAdminCheck->setChecked(false);
        m_adminNewUserDispCheck->setChecked(false);
        m_adminNewUserDriverCheck->setChecked(false);
        m_adminNewUserMechCheck->setChecked(false);
        
        m_adminNewUserLastNameEdit->clear();
        m_adminNewUserFirstNameEdit->clear();
        m_adminNewUserMiddleNameEdit->clear();
        m_adminNewUserExpSpin->setValue(0);
        
        onAdminRefreshUsers(); // Обновляем таблицу пользователей
        onRefreshDrivers();    // Обновляем список водителей в диспетчере
    } else {
        QMessageBox::critical(this, "Ошибка СУБД", "Ошибка регистрации: " + DBManager::instance().lastError());
    }
}

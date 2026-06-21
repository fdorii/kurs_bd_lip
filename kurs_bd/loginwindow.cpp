#include "loginwindow.h"
#include "dbmanager.h"
#include <QMessageBox>

LoginWindow::LoginWindow(QWidget* parent)
    : QDialog(parent), m_userId(0) {
    setWindowTitle("Авторизация");
    resize(360, 220);

    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(20, 20, 20, 20);
    m_mainLayout->setSpacing(15);

    m_titleLabel = new QLabel("Вход в систему", this);
    m_titleLabel->setObjectName("titleLabel");
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_mainLayout->addWidget(m_titleLabel);

    m_formLayout = new QFormLayout();
    m_formLayout->setSpacing(10);

    m_loginEdit = new QLineEdit(this);
    m_loginEdit->setPlaceholderText("Введите логин");
    m_formLayout->addRow("Логин:", m_loginEdit);

    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setPlaceholderText("Введите пароль");
    m_formLayout->addRow("Пароль:", m_passwordEdit);

    m_mainLayout->addLayout(m_formLayout);

    // Статус и ошибки
    m_statusLabel = new QLabel(this);
    m_statusLabel->setStyleSheet("color: red;");
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_mainLayout->addWidget(m_statusLabel);

    // Кнопки
    m_loginBtn = new QPushButton("Войти", this);
    m_loginBtn->setObjectName("primaryButton");
    m_mainLayout->addWidget(m_loginBtn);

    connect(m_loginBtn, &QPushButton::clicked, this, &LoginWindow::onLoginClicked);
}

LoginWindow::~LoginWindow() {}

void LoginWindow::onLoginClicked() {
    QString login = m_loginEdit->text().trimmed();
    QString password = m_passwordEdit->text();

    if (login.isEmpty() || password.isEmpty()) {
        m_statusLabel->setText("Заполните логин и пароль!");
        return;
    }

    bool success = false;
    User u = DBManager::instance().authenticateUser(login, password, success);

    if (success) {
        m_userId = u.id_user;
        m_username = u.login;
        m_userRoles = DBManager::instance().getUserRoles(u.id_user);

        if (m_userRoles.isEmpty()) {
            m_statusLabel->setText("Ошибка: данному пользователю не назначена роль!");
        } else {
            accept(); // Закрываем окно с успехом
        }
    } else {
        m_statusLabel->setText("Неверный логин или пароль!");
    }
}

#include <QApplication>
#include <QDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QFile>
#include <QMessageBox>
#include "dbmanager.h"
#include "loginwindow.h"
#include "mainwindow.h"

// Простой диалог настройки подключения к базе данных
class DBConnectionDialog : public QDialog {
public:
    DBConnectionDialog(QWidget* parent = nullptr) : QDialog(parent) {
        setWindowTitle("Подключение к PostgreSQL");
        resize(350, 220);

        QFormLayout* layout = new QFormLayout(this);

        m_hostEdit = new QLineEdit("localhost", this);
        m_portSpin = new QSpinBox(this);
        m_portSpin->setRange(1, 65535);
        m_portSpin->setValue(5432);
        
        m_dbNameEdit = new QLineEdit("kurs_bd", this);
        m_userEdit = new QLineEdit("postgres", this);
        
        m_passwordEdit = new QLineEdit(this);
        m_passwordEdit->setEchoMode(QLineEdit::Password);
        m_passwordEdit->setPlaceholderText("Введите пароль");

        layout->addRow("Хост (Host):", m_hostEdit);
        layout->addRow("Порт (Port):", m_portSpin);
        layout->addRow("Имя БД (Database):", m_dbNameEdit);
        layout->addRow("Пользователь (User):", m_userEdit);
        layout->addRow("Пароль (Password):", m_passwordEdit);

        m_connectBtn = new QPushButton("Подключиться", this);
        m_connectBtn->setObjectName("primaryButton");
        layout->addRow("", m_connectBtn);

        m_statusLabel = new QLabel(this);
        m_statusLabel->setStyleSheet("color: red;");
        m_statusLabel->setWordWrap(true);
        layout->addRow(m_statusLabel);

        connect(m_connectBtn, &QPushButton::clicked, this, &DBConnectionDialog::onConnectClicked);
    }

private slots:
    void onConnectClicked() {
        m_statusLabel->clear();
        QString host = m_hostEdit->text().trimmed();
        int port = m_portSpin->value();
        QString dbName = m_dbNameEdit->text().trimmed();
        QString user = m_userEdit->text().trimmed();
        QString password = m_passwordEdit->text();

        if (DBManager::instance().openConnection(host, port, dbName, user, password)) {
            accept();
        } else {
            m_statusLabel->setText("Ошибка подключения:\n" + DBManager::instance().lastError());
        }
    }

private:
    QLineEdit* m_hostEdit;
    QSpinBox* m_portSpin;
    QLineEdit* m_dbNameEdit;
    QLineEdit* m_userEdit;
    QLineEdit* m_passwordEdit;
    QPushButton* m_connectBtn;
    QLabel* m_statusLabel;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // Загрузка стилей QSS (светлая тема)
    QFile styleFile("style.qss");
    // Если запускается из сборочной директории, пробуем найти в исходниках
    if (!styleFile.exists()) {
        styleFile.setFileName("../kurs_bd/style.qss");
    }
    if (styleFile.open(QFile::ReadOnly)) {
        QString styleSheet = QLatin1String(styleFile.readAll());
        app.setStyleSheet(styleSheet);
        styleFile.close();
    }

    // 1. Попытка автоматического подключения с дефолтными параметрами
    // (Хост: localhost, БД: kurs_bd, Пользователь: postgres, Пароль: пустой)
    bool connected = DBManager::instance().openConnection("localhost", 5432, "kurs_bd", "postgres", "");
    
    if (!connected) {
        // Если автоподключение не удалось, показываем диалог настройки
        DBConnectionDialog connDialog;
        if (connDialog.exec() != QDialog::Accepted) {
            return 0; // Закрытие программы, если пользователь отказался подключаться
        }
    }

    // 2. Запуск цикла авторизации (поддержка выхода из аккаунта)
    bool logoutRequested = true;
    while (logoutRequested) {
        logoutRequested = false;

        LoginWindow loginWin;
        if (loginWin.exec() == QDialog::Accepted) {
            // 3. Запуск главного окна с правами пользователя
            MainWindow mainWin(
                loginWin.getLoggedInUserId(),
                loginWin.getLoggedInUsername(),
                loginWin.getLoggedInUserRoles()
            );
            mainWin.show();
            
            app.exec();

            // Если пользователь нажал кнопку выхода
            if (mainWin.isLogoutRequested()) {
                logoutRequested = true;
            }
        } else {
            break; // закрытие программы
        }
    }

    // Закрываем соединение при выходе
    DBManager::instance().closeConnection();
    return 0;
}

#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QFormLayout>

class LoginWindow : public QDialog {
    Q_OBJECT
public:
    LoginWindow(QWidget* parent = nullptr);
    ~LoginWindow();

    int getLoggedInUserId() const { return m_userId; }
    QString getLoggedInUsername() const { return m_username; }
    QStringList getLoggedInUserRoles() const { return m_userRoles; }

private slots:
    void onLoginClicked();

private:
    int m_userId;
    QString m_username;
    QStringList m_userRoles;

    // Элементы GUI
    QLabel* m_titleLabel;
    QLineEdit* m_loginEdit;
    QLineEdit* m_passwordEdit;
    QPushButton* m_loginBtn;
    QLabel* m_statusLabel;

    QVBoxLayout* m_mainLayout;
    QFormLayout* m_formLayout;
};

#endif // LOGINWINDOW_H

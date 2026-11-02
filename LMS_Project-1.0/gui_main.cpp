#include <QApplication>
#include <QCoreApplication>
#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QMessageBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>

#include <string>
#include <vector>

#include "Database.h"
#include "AuthService.h"
#include "Dashboard.h"

// =========================================================
// DATABASE PATH RESOLUTION
// =========================================================

namespace {

QString resolveDatabasePath()
{
    const QString appDir =
        QCoreApplication::applicationDirPath();

    // Database lives beside the executable.
    const QString databasePath =
        QDir(appDir).filePath("lms.db");

    // Already exists: use it.
    if (QFileInfo::exists(databasePath)) {
        return databasePath;
    }

    // Optional template: copy from the project directory if present.
    const QString sourceDatabase =
        QDir(appDir).filePath("../lms.db");

    if (QFileInfo::exists(sourceDatabase)) {
        QFile::copy(sourceDatabase, databasePath);
    }

    // Always return the path. If no file exists yet,
    // Database::open() creates it and createTables() builds the schema.
    return databasePath;
}

// =========================================================
// FIRST ADMINISTRATOR ACCOUNT
// =========================================================

bool initializeAdminAccount(Database& database, std::string& error)
{
    const std::string adminUsername = "admin";
    const std::string adminPassword = "admin123";
    const std::string adminRole = "ADMIN";

    std::vector<UserRecord> users;

    if (!database.getUsers(users, error)) {
        return false;
    }

    for (const auto& user : users) {
        if (user.username == adminUsername) {
            return true;
        }
    }

    // Database::createUser() handles Argon2id hashing and salt generation.
    return database.createUser(
        adminUsername,
        adminPassword,
        adminRole,
        error
    );
}

} // namespace

// =========================================================
// APPLICATION ENTRY POINT
// =========================================================

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    app.setApplicationName("Learning Management System");

    // =====================================================
    // LAYER 3: DATABASE INITIALIZATION
    // =====================================================

    const QString databasePath = resolveDatabasePath();

    Database database;

    if (!database.open(databasePath.toStdString())) {

        QMessageBox::critical(
            nullptr,
            "Database Error",
            "Could not open or create the LMS database.\n\n"
            "Database location:\n"
            + QDir::toNativeSeparators(databasePath) + "\n\n"
            "Make sure this folder exists and is writable."
        );

        return 1;
    }

    if (!database.createTables()) {

        QMessageBox::critical(
            nullptr,
            "Database Error",
            "Could not initialize the LMS database."
        );

        return 1;
    }

    // =====================================================
    // FIRST ADMINISTRATOR PROVISIONING
    // =====================================================

    std::string adminError;

    if (!initializeAdminAccount(database, adminError)) {

        QMessageBox::critical(
            nullptr,
            "Administrator Initialization Error",
            "Could not create the initial administrator account.\n\n"
            + QString::fromStdString(adminError)
        );

        return 1;
    }

    // =====================================================
    // LAYER 2: AUTHENTICATION INITIALIZATION
    // =====================================================

    AuthService authService(database);

    // =====================================================
    // LAYER 1: LOGIN WINDOW
    // =====================================================

    QWidget window;
    window.setWindowTitle("Learning Management System");
    window.setFixedSize(420, 320);

    QLabel *title = new QLabel("Learning Management System");
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet(
        "font-size: 20px;"
        "font-weight: bold;"
        "color: #1E3A8A;"
    );

    QLabel *subtitle = new QLabel("Sign in to continue");
    subtitle->setAlignment(Qt::AlignCenter);

    QLineEdit *usernameInput = new QLineEdit();
    usernameInput->setPlaceholderText("Enter your username");

    QLineEdit *passwordInput = new QLineEdit();
    passwordInput->setPlaceholderText("Enter your password");
    passwordInput->setEchoMode(QLineEdit::Password);

    QPushButton *loginButton = new QPushButton("Login");
    loginButton->setMinimumHeight(38);
    loginButton->setStyleSheet(
        "background-color: #1E3A8A;"
        "color: white;"
        "font-weight: bold;"
        "border-radius: 5px;"
    );

    QFormLayout *form = new QFormLayout();
    form->addRow("Username:", usernameInput);
    form->addRow("Password:", passwordInput);

    QVBoxLayout *layout = new QVBoxLayout(&window);
    layout->setContentsMargins(30, 25, 30, 25);
    layout->setSpacing(15);
    layout->addWidget(title);
    layout->addWidget(subtitle);
    layout->addLayout(form);
    layout->addWidget(loginButton);

    // =====================================================
    // LOGIN AUTHENTICATION
    // =====================================================

    QObject::connect(
        loginButton,
        &QPushButton::clicked,
        &window,
        [&authService, &database, usernameInput, passwordInput, &window]()
        {
            const QString username =
                usernameInput->text().trimmed();

            const QString password =
                passwordInput->text();

            if (username.isEmpty() || password.isEmpty()) {

                QMessageBox::warning(
                    &window,
                    "Missing Details",
                    "Please enter your username and password."
                );

                return;
            }

            AuthenticatedUser user;

            const bool authenticated = authService.login(
                username.toStdString(),
                password.toStdString(),
                user
            );

            if (!authenticated) {

                QMessageBox::warning(
                    &window,
                    "Login Failed",
                    "Invalid username or password, "
                    "or the account is inactive."
                );

                passwordInput->clear();
                passwordInput->setFocus();

                return;
            }

            // Create the role-specific dashboard.
            Dashboard *dashboard = new Dashboard(
                database,
                user.userId,
                user.username,
                user.role
            );

            dashboard->setAttribute(Qt::WA_DeleteOnClose);

            // Logout: close the dashboard and any other windows,
            // then bring back the login window.
            QObject::connect(
                dashboard,
                &Dashboard::logoutRequested,
                &window,
                [&window, passwordInput]()
                {
                    const auto topLevels =
                        QApplication::topLevelWidgets();

                    for (QWidget* topLevel : topLevels) {
                        if (topLevel != &window) {
                            topLevel->close();
                        }
                    }

                    passwordInput->clear();

                    window.show();
                    window.raise();
                    window.activateWindow();
                }
            );

            window.hide();
            dashboard->show();
        }
    );

    // Press Enter to log in.
    QObject::connect(
        passwordInput,
        &QLineEdit::returnPressed,
        loginButton,
        &QPushButton::click
    );

    window.show();

    return app.exec();
}
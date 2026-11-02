#ifndef DASHBOARD_H
#define DASHBOARD_H

#include "Database.h"

#include <QWidget>
#include <QString>

#include <string>

class QLabel;
class QPushButton;
class QVBoxLayout;

class Dashboard : public QWidget
{
    Q_OBJECT

public:
    explicit Dashboard(
        Database& database,
        int userId,
        const std::string& username,
        const std::string& role,
        QWidget* parent = nullptr
    );

signals:
    void logoutRequested();

private:
    // Reference to the application's existing database connection.
    Database& database;

    // Authenticated user's information.
    int userId;
    std::string username;
    std::string role;

    // Build the dashboard interface.
    void setupDashboard();

    // Create a dashboard action button.
    void addActionButton(
        QVBoxLayout* layout,
        const QString& buttonText
    );
};

#endif
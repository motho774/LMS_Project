
#include "Dashboard.h"

#include "CourseManagementWindow.h"
#include "LecturerWindow.h"
#include "HodWindow.h"
#include "StudentWindow.h"

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QString>

Dashboard::Dashboard(
    Database& database,
    int userId,
    const std::string& username,
    const std::string& role,
    QWidget* parent
)
    : QWidget(parent),
      database(database),
      userId(userId),
      username(username),
      role(role)
{
    setupDashboard();
}

void Dashboard::addActionButton(
    QVBoxLayout* layout,
    const QString& buttonText
)
{
    QPushButton* button = new QPushButton(buttonText, this);
    button->setMinimumHeight(38);

    layout->addWidget(button);

    QObject::connect(
        button,
        &QPushButton::clicked,
        this,
        [this, buttonText]()
        {
            // -------------------------------------------------
            // STUDENT: Open the selected Student Workspace tab.
            // -------------------------------------------------
            if (role == "STUDENT")
            {
                int initialTab = -1;

                if (buttonText == "My Courses")
                    initialTab = 0;
                else if (buttonText == "View Assignments")
                    initialTab = 1;
                else if (buttonText == "My Submissions")
                    initialTab = 2;
                else if (buttonText == "My Marks and Feedback")
                    initialTab = 3;

                if (initialTab >= 0)
                {
                    auto* studentWindow =
                        new StudentWindow(database, userId);

                    studentWindow->setAttribute(
                        Qt::WA_DeleteOnClose
                    );

                    studentWindow->selectTab(initialTab);
                    studentWindow->show();
                    studentWindow->raise();
                    studentWindow->activateWindow();

                    return;
                }
            }

            // -------------------------------------------------
            // ADMIN: Open the appropriate management tab.
            // -------------------------------------------------
            if (role == "ADMIN")
            {
                int initialTab = -1;

                if (buttonText == "Manage Departments")
                    initialTab = 0;
                else if (buttonText == "Manage Courses")
                    initialTab = 0;
                else if (buttonText == "Manage Users")
                    initialTab = 1;
                else if (buttonText == "Manage Enrolments")
                    initialTab = 2;
                else if (buttonText == "Audit Trail")
                    initialTab = 5;

                if (initialTab >= 0)
                {
                    auto* managementWindow =
                        new CourseManagementWindow(
                            database, role, userId);

                    managementWindow->setAttribute(
                        Qt::WA_DeleteOnClose
                    );

                    managementWindow->selectTab(initialTab);
                    managementWindow->show();
                    managementWindow->raise();
                    managementWindow->activateWindow();

                    return;
                }
            }

            // -------------------------------------------------
            // LECTURER: Open the selected Lecturer Workspace tab.
            // -------------------------------------------------
            if (role == "LECTURER")
            {
                int initialTab = 0;

                if (buttonText == "My Courses")
                    initialTab = 0;
                else if (buttonText == "Manage Assignments")
                    initialTab = 1;
                else if (buttonText == "View Submissions")
                    initialTab = 2;
                else if (buttonText == "Mark Submissions")
                    initialTab = 2;

                auto* lecturerWindow =
                    new LecturerWindow(database, userId);

                lecturerWindow->setAttribute(
                    Qt::WA_DeleteOnClose
                );

                lecturerWindow->selectTab(initialTab);
                lecturerWindow->show();
                lecturerWindow->raise();
                lecturerWindow->activateWindow();

                return;
            }

            // -------------------------------------------------
            // HOD: Open the selected HoD Workspace tab.
            // -------------------------------------------------
            if (role == "HOD")
            {
                int initialTab = -1;

                if (buttonText == "Department Courses")
                    initialTab = 0;
                else if (buttonText == "Review Marks")
                    initialTab = 1;
                else if (buttonText == "Moderate Marks")
                    initialTab = 1;
                else if (buttonText == "Release Approved Marks")
                    initialTab = 2;

                if (initialTab >= 0)
                {
                    auto* hodWindow =
                        new HodWindow(database, userId);

                    hodWindow->setAttribute(
                        Qt::WA_DeleteOnClose
                    );

                    hodWindow->selectTab(initialTab);
                    hodWindow->show();
                    hodWindow->raise();
                    hodWindow->activateWindow();

                    return;
                }
            }

            // -------------------------------------------------
            // Unconnected services.
            // -------------------------------------------------
            QMessageBox::information(
                this,
                "Feature Not Implemented",
                buttonText +
                    " will be implemented in a later step."
            );
        }
    );
}

void Dashboard::setupDashboard()
{
    setWindowTitle("LMS Dashboard");
    setMinimumSize(600, 450);

    auto* heading = new QLabel(
        "Learning Management System",
        this
    );

    heading->setStyleSheet(
        "font-size: 22px;"
        "font-weight: bold;"
        "color: #1E3A8A;"
    );

    auto* welcome = new QLabel(
        "Welcome, " + QString::fromStdString(username),
        this
    );

    auto* roleLabel = new QLabel(
        "Role: " + QString::fromStdString(role),
        this
    );

    auto* idLabel = new QLabel(
        "User ID: " + QString::number(userId),
        this
    );

    auto* layout = new QVBoxLayout(this);

    layout->setContentsMargins(30, 25, 30, 25);
    layout->setSpacing(12);

    layout->addWidget(heading);
    layout->addWidget(welcome);
    layout->addWidget(roleLabel);
    layout->addWidget(idLabel);

    layout->addSpacing(10);

    // STUDENT DASHBOARD
    if (role == "STUDENT")
    {
        addActionButton(layout, "My Courses");
        addActionButton(layout, "View Assignments");
        addActionButton(layout, "My Submissions");
        addActionButton(layout, "My Marks and Feedback");
    }

    // LECTURER DASHBOARD
    else if (role == "LECTURER")
    {
        addActionButton(layout, "My Courses");
        addActionButton(layout, "Manage Assignments");
        addActionButton(layout, "View Submissions");
        addActionButton(layout, "Mark Submissions");
    }

    // HEAD OF DEPARTMENT DASHBOARD
    else if (role == "HOD")
    {
        addActionButton(layout, "Department Courses");
        addActionButton(layout, "Review Marks");
        addActionButton(layout, "Moderate Marks");
        addActionButton(layout, "Release Approved Marks");
    }

    // ADMIN DASHBOARD
    else if (role == "ADMIN")
    {
        addActionButton(layout, "Manage Users");
        addActionButton(layout, "Manage Departments");
        addActionButton(layout, "Manage Courses");
        addActionButton(layout, "Manage Enrolments");
        addActionButton(layout, "Audit Trail");
    }

    // UNKNOWN ROLE
    else
    {
        auto* errorLabel = new QLabel(
            "Unrecognized user role. "
            "Please contact the administrator.",
            this
        );

        layout->addWidget(errorLabel);
    }

    auto* logoutButton = new QPushButton("Logout", this);
    logoutButton->setMinimumHeight(38);
    layout->addWidget(logoutButton);

    QObject::connect(
        logoutButton,
        &QPushButton::clicked,
        this,
        [this]()
        {
            emit logoutRequested();
            close();
        }
    );

    layout->addStretch();
}

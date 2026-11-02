#include "CourseManagementWindow.h"

#include <QAbstractItemView>
#include <QComboBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTabWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QString>
#include <QDialog>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QCheckBox>
#include <QDateEdit>
#include <QDate>
#include <QStringList>

#include <string>
#include <vector>

// ============================================================
// CONSTRUCTOR
// ============================================================

CourseManagementWindow::CourseManagementWindow(
    Database& database,
    const std::string& role,
    int currentUserId,
    QWidget* parent
)
    : QWidget(parent),
      database(database),
      courseService(database),
      role(role),
      currentUserId(currentUserId),
      departmentNameInput(nullptr),
      courseCodeInput(nullptr),
      courseNameInput(nullptr),
      departmentCombo(nullptr),
      coursesTable(nullptr),
      usersTable(nullptr),
      enrolStudentCombo(nullptr),
      enrolCourseCombo(nullptr),
      enrolmentsTable(nullptr),
      lecturerCombo(nullptr),
      lecturerCourseCombo(nullptr),
      lecturerAssignmentsTable(nullptr),
      hodDepartmentCombo(nullptr),
      hodUserCombo(nullptr)
{
    setupUi();
    refreshAll();
}

// ============================================================
// USER INTERFACE
// ============================================================

void CourseManagementWindow::setupUi()
{
    setWindowTitle("LMS - Administration");
    resize(1100, 700);

    auto* mainLayout = new QVBoxLayout(this);

    auto* heading = new QLabel(
        "LMS Administration Workspace", this
    );

    heading->setStyleSheet(
        "font-size: 22px;"
        "font-weight: bold;"
        "color: #1E3A8A;"
    );

    mainLayout->addWidget(heading);

    tabs = new QTabWidget(this);
    mainLayout->addWidget(tabs, 1);

    // ========================================================
    // TAB 1: DEPARTMENTS AND COURSES
    // ========================================================

    auto* coursesTab = new QWidget(tabs);
    auto* coursesLayout = new QVBoxLayout(coursesTab);

    coursesLayout->addWidget(
        new QLabel("Create a department", coursesTab)
    );

    auto* departmentRow = new QHBoxLayout();

    departmentNameInput = new QLineEdit(coursesTab);
    departmentNameInput->setPlaceholderText("Department name");

    auto* addDepartmentButton =
        new QPushButton("Add Department", coursesTab);

    departmentRow->addWidget(departmentNameInput, 1);
    departmentRow->addWidget(addDepartmentButton);

    coursesLayout->addLayout(departmentRow);

    coursesLayout->addWidget(
        new QLabel("Create a course", coursesTab)
    );

    auto* courseRow = new QHBoxLayout();

    courseCodeInput = new QLineEdit(coursesTab);
    courseCodeInput->setPlaceholderText(
        "Course code (e.g. CS4433)"
    );

    courseNameInput = new QLineEdit(coursesTab);
    courseNameInput->setPlaceholderText("Course name");

    departmentCombo = new QComboBox(coursesTab);
    departmentCombo->setMinimumWidth(170);

    auto* addCourseButton =
        new QPushButton("Add Course", coursesTab);

    courseRow->addWidget(courseCodeInput, 1);
    courseRow->addWidget(courseNameInput, 2);
    courseRow->addWidget(departmentCombo, 1);
    courseRow->addWidget(addCourseButton);

    coursesLayout->addLayout(courseRow);

    coursesLayout->addWidget(
        new QLabel("Existing courses", coursesTab)
    );

    coursesTable = new QTableWidget(coursesTab);
    coursesTable->setColumnCount(4);

    coursesTable->setHorizontalHeaderLabels({
        "Course ID",
        "Course Code",
        "Course Name",
        "Department"
    });

    coursesTable->setEditTriggers(
        QAbstractItemView::NoEditTriggers
    );

    coursesTable->setSelectionBehavior(
        QAbstractItemView::SelectRows
    );

    coursesTable->setSelectionMode(
        QAbstractItemView::SingleSelection
    );

    coursesTable->horizontalHeader()->setSectionResizeMode(
        QHeaderView::Stretch
    );

    coursesTable->setAlternatingRowColors(true);

    coursesLayout->addWidget(coursesTable, 1);

    tabs->addTab(coursesTab, "Departments & Courses");


    // ========================================================
    // TAB 2: USER MANAGEMENT
    // ========================================================

    auto* usersTab = new QWidget(tabs);
    auto* usersLayout = new QVBoxLayout(usersTab);

    usersLayout->addWidget(
        new QLabel(
            "Create, update, and manage user accounts.",
            usersTab
        )
    );

    usersTable = new QTableWidget(usersTab);
    usersTable->setColumnCount(4);

    usersTable->setHorizontalHeaderLabels({
        "User ID",
        "Username",
        "Role",
        "Account Status"
    });

    usersTable->setEditTriggers(
        QAbstractItemView::NoEditTriggers
    );

    usersTable->setSelectionBehavior(
        QAbstractItemView::SelectRows
    );

    usersTable->setSelectionMode(
        QAbstractItemView::SingleSelection
    );

    usersTable->horizontalHeader()->setSectionResizeMode(
        QHeaderView::Stretch
    );

    usersTable->setAlternatingRowColors(true);

    usersLayout->addWidget(usersTable, 1);

    // User management buttons

    auto* userButtons = new QHBoxLayout();

    auto* createUserButton =
        new QPushButton("Create User", usersTab);

    auto* editUserButton =
        new QPushButton("Edit Selected User", usersTab);

    auto* toggleUserButton =
        new QPushButton(
            "Activate / Deactivate",
            usersTab
        );

    auto* refreshUsersButton =
        new QPushButton("Refresh Users", usersTab);

    userButtons->addWidget(createUserButton);
    userButtons->addWidget(editUserButton);
    userButtons->addWidget(toggleUserButton);
    userButtons->addWidget(refreshUsersButton);

    usersLayout->addLayout(userButtons);

    tabs->addTab(usersTab, "User Management");


    // ========================================================
    // TAB 3: STUDENT ENROLMENTS
    // ========================================================

    auto* enrolmentsTab = new QWidget(tabs);
    auto* enrolmentsLayout = new QVBoxLayout(enrolmentsTab);

    enrolmentsLayout->addWidget(
        new QLabel(
            "Enrol a student in a course",
            enrolmentsTab
        )
    );

    auto* enrolmentForm = new QHBoxLayout();

    enrolStudentCombo = new QComboBox(enrolmentsTab);
    enrolCourseCombo = new QComboBox(enrolmentsTab);

    auto* enrolButton = new QPushButton(
        "Enrol Student", enrolmentsTab
    );

    enrolmentForm->addWidget(enrolStudentCombo, 1);
    enrolmentForm->addWidget(enrolCourseCombo, 2);
    enrolmentForm->addWidget(enrolButton);

    enrolmentsLayout->addLayout(enrolmentForm);

    enrolmentsTable = new QTableWidget(enrolmentsTab);
    enrolmentsTable->setColumnCount(6);

    enrolmentsTable->setHorizontalHeaderLabels({
        "Student ID",
        "Student",
        "Course ID",
        "Course Code",
        "Course Name",
        "Enrolled At"
    });

    enrolmentsTable->setEditTriggers(
        QAbstractItemView::NoEditTriggers
    );

    enrolmentsTable->setSelectionBehavior(
        QAbstractItemView::SelectRows
    );

    enrolmentsTable->setSelectionMode(
        QAbstractItemView::SingleSelection
    );

    enrolmentsTable->horizontalHeader()->setSectionResizeMode(
        QHeaderView::Stretch
    );

    enrolmentsTable->setAlternatingRowColors(true);

    enrolmentsLayout->addWidget(enrolmentsTable, 1);

    auto* enrolmentButtons = new QHBoxLayout();

    auto* removeEnrolmentButton = new QPushButton(
        "Remove Selected Enrolment", enrolmentsTab
    );

    auto* refreshEnrolmentsButton = new QPushButton(
        "Refresh", enrolmentsTab
    );

    enrolmentButtons->addStretch();
    enrolmentButtons->addWidget(removeEnrolmentButton);
    enrolmentButtons->addWidget(refreshEnrolmentsButton);

    enrolmentsLayout->addLayout(enrolmentButtons);

    tabs->addTab(enrolmentsTab, "Student Enrolments");


    // ========================================================
    // TAB 4: LECTURER ASSIGNMENTS
    // ========================================================

    auto* lecturersTab = new QWidget(tabs);
    auto* lecturersLayout = new QVBoxLayout(lecturersTab);

    lecturersLayout->addWidget(
        new QLabel(
            "Assign a lecturer to teach a course.",
            lecturersTab
        )
    );

    auto* lecturerForm = new QHBoxLayout();

    lecturerCombo = new QComboBox(lecturersTab);
    lecturerCourseCombo = new QComboBox(lecturersTab);

    auto* assignLecturerButton = new QPushButton(
        "Assign Lecturer", lecturersTab
    );

    lecturerForm->addWidget(lecturerCombo, 1);
    lecturerForm->addWidget(lecturerCourseCombo, 2);
    lecturerForm->addWidget(assignLecturerButton);

    lecturersLayout->addLayout(lecturerForm);

    lecturerAssignmentsTable = new QTableWidget(lecturersTab);
    lecturerAssignmentsTable->setColumnCount(5);

    lecturerAssignmentsTable->setHorizontalHeaderLabels({
        "Course ID",
        "Course Code",
        "Course Name",
        "Lecturer ID",
        "Lecturer"
    });

    lecturerAssignmentsTable->setEditTriggers(
        QAbstractItemView::NoEditTriggers
    );

    lecturerAssignmentsTable->setSelectionBehavior(
        QAbstractItemView::SelectRows
    );

    lecturerAssignmentsTable->setSelectionMode(
        QAbstractItemView::SingleSelection
    );

    lecturerAssignmentsTable->horizontalHeader()->setSectionResizeMode(
        QHeaderView::Stretch
    );

    lecturerAssignmentsTable->setAlternatingRowColors(true);

    lecturersLayout->addWidget(lecturerAssignmentsTable, 1);

    auto* lecturerButtons = new QHBoxLayout();

    auto* removeLecturerButton = new QPushButton(
        "Remove Selected Assignment", lecturersTab
    );

    auto* refreshLecturersButton = new QPushButton(
        "Refresh", lecturersTab
    );

    lecturerButtons->addStretch();
    lecturerButtons->addWidget(removeLecturerButton);
    lecturerButtons->addWidget(refreshLecturersButton);

    lecturersLayout->addLayout(lecturerButtons);

    tabs->addTab(lecturersTab, "Lecturer Assignments");


    // ========================================================
    // TAB 5: HOD ASSIGNMENTS
    // ========================================================

    auto* hodTab = new QWidget(tabs);
    auto* hodLayout = new QVBoxLayout(hodTab);

    hodLayout->addWidget(
        new QLabel(
            "Assign an active HoD account to a department. "
            "This replaces the department's current HoD assignment.",
            hodTab
        )
    );

    auto* hodForm = new QHBoxLayout();

    hodDepartmentCombo = new QComboBox(hodTab);
    hodUserCombo = new QComboBox(hodTab);

    auto* assignHodButton = new QPushButton(
        "Assign HoD", hodTab
    );

    hodForm->addWidget(hodDepartmentCombo, 1);
    hodForm->addWidget(hodUserCombo, 1);
    hodForm->addWidget(assignHodButton);

    hodLayout->addLayout(hodForm);
    hodLayout->addStretch();

    tabs->addTab(hodTab, "HoD Assignments");

    if (role == "ADMIN") {
        auto* auditTab = new QWidget(tabs);
        auto* auditLayout = new QVBoxLayout(auditTab);
        auto* filtersRow = new QHBoxLayout();

        auditActionCombo = new QComboBox(auditTab);
        auditActionCombo->addItem("All actions", "");
        auditActionCombo->addItem("Mark created", "MARK_CREATED");
        auditActionCombo->addItem("Mark changed", "MARK_CHANGED");
        auditActionCombo->addItem("Mark returned", "MARK_RETURNED");
        auditActionCombo->addItem("Mark approved", "MARK_APPROVED");
        auditActionCombo->addItem(
            "Marks released", "APPROVED_MARKS_RELEASED");
        auditActionCombo->addItem(
            "Assignment created", "ASSIGNMENT_CREATED");
        auditActionCombo->addItem(
            "Submission recorded", "ASSIGNMENT_SUBMITTED");
        auditActionCombo->addItem("Late submission", "LATE_SUBMISSION");
        auditActionCombo->addItem(
            "Deadline extension", "DEADLINE_EXTENSION_GRANTED");

        auditEntityCombo = new QComboBox(auditTab);
        auditEntityCombo->addItem("All entities", "");
        auditEntityCombo->addItem("Mark", "MARK");
        auditEntityCombo->addItem("Assignment", "ASSIGNMENT");
        auditEntityCombo->addItem("Submission", "SUBMISSION");

        auditActorInput = new QLineEdit(auditTab);
        auditActorInput->setPlaceholderText("Actor username");
        auditSearchInput = new QLineEdit(auditTab);
        auditSearchInput->setPlaceholderText("Search values, reason, details");

        auto* filterButton =
            new QPushButton("Apply Filters", auditTab);
        filtersRow->addWidget(auditActionCombo, 1);
        filtersRow->addWidget(auditEntityCombo, 1);
        filtersRow->addWidget(auditActorInput, 1);
        filtersRow->addWidget(auditSearchInput, 2);
        filtersRow->addWidget(filterButton);
        auditLayout->addLayout(filtersRow);

        auto* dateRow = new QHBoxLayout();
        auditDateRangeCheck =
            new QCheckBox("Limit date range", auditTab);
        auditStartDate = new QDateEdit(auditTab);
        auditEndDate = new QDateEdit(auditTab);
        auditStartDate->setCalendarPopup(true);
        auditEndDate->setCalendarPopup(true);
        auditStartDate->setDate(QDate::currentDate().addDays(-30));
        auditEndDate->setDate(QDate::currentDate());
        auditStartDate->setEnabled(false);
        auditEndDate->setEnabled(false);
        dateRow->addWidget(auditDateRangeCheck);
        dateRow->addWidget(new QLabel("From", auditTab));
        dateRow->addWidget(auditStartDate);
        dateRow->addWidget(new QLabel("Through", auditTab));
        dateRow->addWidget(auditEndDate);
        dateRow->addStretch();
        auditLayout->addLayout(dateRow);

        auditTable = new QTableWidget(auditTab);
        auditTable->setColumnCount(11);
        auditTable->setHorizontalHeaderLabels({
            "Timestamp", "Actor", "Actor ID", "Role", "Action", "Entity",
            "Entity ID", "Previous Value", "New Value", "Reason",
            "Additional System Information"
        });
        auditTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        auditTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        auditTable->setAlternatingRowColors(true);
        auditTable->horizontalHeader()->setSectionResizeMode(
            QHeaderView::ResizeToContents);
        auditTable->horizontalHeader()->setStretchLastSection(true);
        auditLayout->addWidget(auditTable, 1);
        tabs->addTab(auditTab, "Audit Trail");

        connect(auditDateRangeCheck, &QCheckBox::toggled,
                this, [this](bool enabled) {
            auditStartDate->setEnabled(enabled);
            auditEndDate->setEnabled(enabled);
        });
        connect(filterButton, &QPushButton::clicked,
                this, [this]() { refreshAuditTrail(); });
        connect(auditSearchInput, &QLineEdit::returnPressed,
                this, [this]() { refreshAuditTrail(); });
    }


    // ========================================================
    // BOTTOM BUTTONS
    // ========================================================

    auto* bottomRow = new QHBoxLayout();

    auto* refreshAllButton = new QPushButton(
        "Refresh All", this
    );

    auto* closeButton = new QPushButton(
        "Close", this
    );

    bottomRow->addStretch();
    bottomRow->addWidget(refreshAllButton);
    bottomRow->addWidget(closeButton);

    mainLayout->addLayout(bottomRow);


    // ========================================================
    // BUTTON CONNECTIONS
    // ========================================================

    // Department management

    connect(addDepartmentButton, &QPushButton::clicked,
            this, [this]() {
        addDepartment();
    });

    // Course management

    connect(addCourseButton, &QPushButton::clicked,
            this, [this]() {
        addCourse();
    });


    // --------------------------------------------------------
    // CREATE USER
    // --------------------------------------------------------

    connect(createUserButton, &QPushButton::clicked,
            this, [this]() {

        QDialog dialog(this);
        dialog.setWindowTitle("Create User");
        dialog.setMinimumWidth(350);

        auto* layout = new QFormLayout(&dialog);

        auto* usernameInput = new QLineEdit(&dialog);
        auto* passwordInput = new QLineEdit(&dialog);

        passwordInput->setEchoMode(QLineEdit::Password);

        auto* roleInput = new QComboBox(&dialog);

        roleInput->addItems({
            "STUDENT",
            "LECTURER",
            "HOD",
            "ADMIN"
        });

        layout->addRow("Username:", usernameInput);
        layout->addRow("Password:", passwordInput);
        layout->addRow("Role:", roleInput);

        auto* buttons = new QDialogButtonBox(
            QDialogButtonBox::Save |
            QDialogButtonBox::Cancel,
            &dialog
        );

        layout->addRow(buttons);

        connect(buttons, &QDialogButtonBox::accepted,
                &dialog, &QDialog::accept);

        connect(buttons, &QDialogButtonBox::rejected,
                &dialog, &QDialog::reject);

        if (dialog.exec() != QDialog::Accepted)
            return;

        const std::string username =
            usernameInput->text().trimmed().toStdString();

        const std::string password =
            passwordInput->text().toStdString();

        const std::string userRole =
            roleInput->currentText().toStdString();

        if (username.empty() || password.empty()) {
            showError("Username and password are required.");
            return;
        }

        std::string error;

        if (!database.createUser(
                username, password, userRole, error)) {
            showError(error);
            return;
        }

        refreshUsers();

        QMessageBox::information(
            this,
            "Success",
            "User account created successfully."
        );
    });


    // --------------------------------------------------------
    // EDIT USER
    // --------------------------------------------------------

    connect(editUserButton, &QPushButton::clicked,
            this, [this]() {

        const int row = usersTable->currentRow();

        if (row < 0) {
            showError("Select a user to edit.");
            return;
        }

        const int userId =
            usersTable->item(row, 0)->text().toInt();

        const QString currentUsername =
            usersTable->item(row, 1)->text();

        const QString currentRole =
            usersTable->item(row, 2)->text();

        QDialog dialog(this);
        dialog.setWindowTitle("Edit User Account");
        dialog.setMinimumWidth(350);

        auto* layout = new QFormLayout(&dialog);

        auto* usernameInput = new QLineEdit(
            currentUsername, &dialog
        );

        auto* passwordInput = new QLineEdit(&dialog);

        passwordInput->setEchoMode(QLineEdit::Password);

        passwordInput->setPlaceholderText(
            "Leave blank to keep current password"
        );

        auto* roleInput = new QComboBox(&dialog);

        roleInput->addItems({
            "STUDENT",
            "LECTURER",
            "HOD",
            "ADMIN"
        });

        roleInput->setCurrentText(currentRole);

        layout->addRow("Username:", usernameInput);
        layout->addRow("New Password:", passwordInput);
        layout->addRow("Role:", roleInput);

        auto* buttons = new QDialogButtonBox(
            QDialogButtonBox::Save |
            QDialogButtonBox::Cancel,
            &dialog
        );

        layout->addRow(buttons);

        connect(buttons, &QDialogButtonBox::accepted,
                &dialog, &QDialog::accept);

        connect(buttons, &QDialogButtonBox::rejected,
                &dialog, &QDialog::reject);

        if (dialog.exec() != QDialog::Accepted)
            return;

        const std::string username =
            usernameInput->text().trimmed().toStdString();

        const std::string password =
            passwordInput->text().toStdString();

        const std::string userRole =
            roleInput->currentText().toStdString();

        if (username.empty()) {
            showError("Username cannot be empty.");
            return;
        }

        std::string error;

        if (!database.updateUser(
                userId, username, userRole, password, error)) {
            showError(error);
            return;
        }

        refreshUsers();

        QMessageBox::information(
            this,
            "Success",
            "User account updated successfully."
        );
    });


    // Activate / Deactivate user

    connect(toggleUserButton, &QPushButton::clicked,
            this, [this]() {
        toggleSelectedUser();
    });

    // Refresh users

    connect(refreshUsersButton, &QPushButton::clicked,
            this, [this]() {
        refreshUsers();
    });


    // Student enrolments

    connect(enrolButton, &QPushButton::clicked,
            this, [this]() {
        addEnrolment();
    });

    connect(removeEnrolmentButton, &QPushButton::clicked,
            this, [this]() {
        removeSelectedEnrolment();
    });

    connect(refreshEnrolmentsButton, &QPushButton::clicked,
            this, [this]() {
        refreshEnrolmentOptions();
        refreshEnrolments();
    });


    // Lecturer assignments

    connect(assignLecturerButton, &QPushButton::clicked,
            this, [this]() {
        addLecturerAssignment();
    });

    connect(removeLecturerButton, &QPushButton::clicked,
            this, [this]() {
        removeSelectedLecturerAssignment();
    });

    connect(refreshLecturersButton, &QPushButton::clicked,
            this, [this]() {
        refreshLecturerOptions();
        refreshLecturerAssignments();
    });


    // HoD assignments

    connect(assignHodButton, &QPushButton::clicked,
            this, [this]() {
        assignHod();
    });


    // Refresh all management data

    connect(refreshAllButton, &QPushButton::clicked,
            this, [this]() {
        refreshAll();
    });


    // Close administration window

    connect(closeButton, &QPushButton::clicked,
            this, &QWidget::close);
}

// ============================================================
// REFRESH ALL
// ============================================================

void CourseManagementWindow::refreshAll()
{
    refreshDepartments();
    refreshCourses();
    refreshUsers();
    refreshEnrolments();
    refreshLecturerAssignments();
    if (auditTable != nullptr)
        refreshAuditTrail();
}

void CourseManagementWindow::refreshAuditTrail()
{
    if (role != "ADMIN" || auditTable == nullptr)
        return;

    AuditTrailFilters filters;
    filters.action = auditActionCombo->currentData().toString().toStdString();
    filters.entityType =
        auditEntityCombo->currentData().toString().toStdString();
    filters.actor = auditActorInput->text().trimmed().toStdString();
    filters.searchText = auditSearchInput->text().trimmed().toStdString();
    if (auditDateRangeCheck->isChecked()) {
        filters.startDate =
            auditStartDate->date().toString("yyyy-MM-dd").toStdString();
        filters.endDate =
            auditEndDate->date().toString("yyyy-MM-dd").toStdString();
    }

    std::vector<AuditTrailRecord> records;
    std::string error;
    if (!database.getAdminAuditTrail(
            currentUserId, filters, records, error)) {
        showError(error);
        return;
    }

    auditTable->setRowCount(0);
    for (const auto& record : records) {
        const int row = auditTable->rowCount();
        auditTable->insertRow(row);
        const QStringList values = {
            QString::fromStdString(record.occurredAt),
            QString::fromStdString(record.actorUsername),
            QString::number(record.actorId),
            QString::fromStdString(record.actorRole),
            QString::fromStdString(record.action),
            QString::fromStdString(record.entityType),
            QString::number(record.entityId),
            QString::fromStdString(record.oldValue),
            QString::fromStdString(record.newValue),
            QString::fromStdString(record.reason),
            QString::fromStdString(record.details)
        };
        for (int column = 0; column < values.size(); ++column)
            auditTable->setItem(
                row, column, new QTableWidgetItem(values[column]));
    }
}

// ============================================================
// DEPARTMENTS AND COURSES
// ============================================================

void CourseManagementWindow::refreshDepartments()
{
    std::vector<DepartmentRecord> departments;
    std::string error;

    if (!courseService.getDepartments(role, departments, error)) {
        showError(error);
        return;
    }

    departmentCombo->clear();
    hodDepartmentCombo->clear();

    for (const auto& department : departments) {
        departmentCombo->addItem(
            QString::fromStdString(department.name),
            department.departmentId
        );

        hodDepartmentCombo->addItem(
            QString::fromStdString(department.name),
            department.departmentId
        );
    }

    if (departments.empty()) {
        departmentCombo->addItem("Create a department first", -1);
        hodDepartmentCombo->addItem("Create a department first", -1);
    }
}

void CourseManagementWindow::refreshCourses()
{
    std::vector<CourseRecord> courses;
    std::string error;

    if (!courseService.getCourses(role, courses, error)) {
        showError(error);
        return;
    }

    coursesTable->setRowCount(0);

    for (const auto& course : courses) {
        const int row = coursesTable->rowCount();
        coursesTable->insertRow(row);

        coursesTable->setItem(row, 0,
            new QTableWidgetItem(QString::number(course.courseId)));

        coursesTable->setItem(row, 1,
            new QTableWidgetItem(
                QString::fromStdString(course.courseCode)));

        coursesTable->setItem(row, 2,
            new QTableWidgetItem(
                QString::fromStdString(course.name)));

        coursesTable->setItem(row, 3,
            new QTableWidgetItem(
                QString::fromStdString(course.departmentName)));
    }
}

void CourseManagementWindow::addDepartment()
{
    const std::string name =
        departmentNameInput->text().trimmed().toStdString();

    if (name.empty()) {
        showError("Enter a department name.");
        return;
    }

    std::string error;

    if (!courseService.createDepartment(role, name, error)) {
        showError(error);
        return;
    }

    departmentNameInput->clear();
    refreshDepartments();

    QMessageBox::information(
        this, "Success", "Department created successfully."
    );
}

void CourseManagementWindow::addCourse()
{
    const int departmentId =
        departmentCombo->currentData().toInt();

    if (departmentId <= 0) {
        showError("Create a department before adding a course.");
        return;
    }

    const std::string code =
        courseCodeInput->text().trimmed().toStdString();

    const std::string name =
        courseNameInput->text().trimmed().toStdString();

    if (code.empty() || name.empty()) {
        showError("Enter both a course code and course name.");
        return;
    }

    std::string error;

    if (!courseService.createCourse(
            role, code, name, departmentId, error)) {
        showError(error);
        return;
    }

    courseCodeInput->clear();
    courseNameInput->clear();

    refreshCourses();
    refreshEnrolmentOptions();
    refreshLecturerOptions();

    QMessageBox::information(
        this, "Success", "Course created successfully."
    );
}

// ============================================================
// USER MANAGEMENT
// ============================================================

void CourseManagementWindow::refreshUsers()
{
    std::vector<UserRecord> users;
    std::string error;

    if (!database.getUsers(users, error)) {
        showError(error);
        return;
    }

    usersTable->setRowCount(0);

    for (const auto& user : users) {
        const int row = usersTable->rowCount();
        usersTable->insertRow(row);

        usersTable->setItem(row, 0,
            new QTableWidgetItem(QString::number(user.userId)));

        usersTable->setItem(row, 1,
            new QTableWidgetItem(
                QString::fromStdString(user.username)));

        usersTable->setItem(row, 2,
            new QTableWidgetItem(
                QString::fromStdString(user.role)));

        usersTable->setItem(row, 3,
            new QTableWidgetItem(
                user.active ? "Active" : "Inactive"));
    }

    refreshEnrolmentOptions();
    refreshLecturerOptions();
    refreshHodOptions();
}

void CourseManagementWindow::toggleSelectedUser()
{
    const int row = usersTable->currentRow();

    if (row < 0) {
        showError("Select a user first.");
        return;
    }

    const int userId =
        usersTable->item(row, 0)->text().toInt();

    const bool activate =
        usersTable->item(row, 3)->text() != "Active";

    if (QMessageBox::question(
            this,
            "Confirm Account Status Change",
            activate
                ? "Activate this user account?"
                : "Deactivate this user account?"
        ) != QMessageBox::Yes) {
        return;
    }

    std::string error;

    if (!database.setUserActive(userId, activate, error)) {
        showError(error);
        return;
    }

    refreshUsers();

    QMessageBox::information(
        this, "Success", "User account status updated."
    );
}

// ============================================================
// STUDENT ENROLMENTS
// ============================================================

void CourseManagementWindow::refreshEnrolmentOptions()
{
    std::vector<UserRecord> users;
    std::vector<CourseRecord> courses;
    std::string error;

    if (!database.getUsers(users, error)) {
        showError(error);
        return;
    }

    if (!courseService.getCourses(role, courses, error)) {
        showError(error);
        return;
    }

    enrolStudentCombo->clear();
    enrolCourseCombo->clear();

    for (const auto& user : users) {
        if (user.role == "STUDENT" && user.active) {
            enrolStudentCombo->addItem(
                QString::fromStdString(user.username),
                user.userId
            );
        }
    }

    for (const auto& course : courses) {
        enrolCourseCombo->addItem(
            QString::fromStdString(
                course.courseCode + " - " + course.name),
            course.courseId
        );
    }

    if (enrolStudentCombo->count() == 0) {
        enrolStudentCombo->addItem(
            "No active students available", -1
        );
    }

    if (enrolCourseCombo->count() == 0) {
        enrolCourseCombo->addItem(
            "No courses available", -1
        );
    }
}

void CourseManagementWindow::refreshEnrolments()
{
    std::vector<EnrolmentRecord> enrolments;
    std::string error;

    if (!database.getEnrolments(enrolments, error)) {
        showError(error);
        return;
    }

    enrolmentsTable->setRowCount(0);

    for (const auto& record : enrolments) {
        const int row = enrolmentsTable->rowCount();
        enrolmentsTable->insertRow(row);

        enrolmentsTable->setItem(row, 0,
            new QTableWidgetItem(QString::number(record.studentId)));

        enrolmentsTable->setItem(row, 1,
            new QTableWidgetItem(
                QString::fromStdString(record.studentUsername)));

        enrolmentsTable->setItem(row, 2,
            new QTableWidgetItem(QString::number(record.courseId)));

        enrolmentsTable->setItem(row, 3,
            new QTableWidgetItem(
                QString::fromStdString(record.courseCode)));

        enrolmentsTable->setItem(row, 4,
            new QTableWidgetItem(
                QString::fromStdString(record.courseName)));

        enrolmentsTable->setItem(row, 5,
            new QTableWidgetItem(
                QString::fromStdString(record.enrolledAt)));
    }
}

void CourseManagementWindow::addEnrolment()
{
    const int studentId =
        enrolStudentCombo->currentData().toInt();

    const int courseId =
        enrolCourseCombo->currentData().toInt();

    if (studentId <= 0 || courseId <= 0) {
        showError("Select a valid student and course.");
        return;
    }

    std::string error;

    if (!database.enrolStudent(studentId, courseId, error)) {
        showError(error);
        return;
    }

    refreshEnrolments();

    QMessageBox::information(
        this, "Success", "Student enrolled successfully."
    );
}

void CourseManagementWindow::removeSelectedEnrolment()
{
    const int row = enrolmentsTable->currentRow();

    if (row < 0) {
        showError("Select an enrolment first.");
        return;
    }

    const int studentId =
        enrolmentsTable->item(row, 0)->text().toInt();

    const int courseId =
        enrolmentsTable->item(row, 2)->text().toInt();

    if (QMessageBox::question(
            this,
            "Confirm Removal",
            "Remove this student's course enrolment?"
        ) != QMessageBox::Yes) {
        return;
    }

    std::string error;

    if (!database.removeEnrolment(studentId, courseId, error)) {
        showError(error);
        return;
    }

    refreshEnrolments();

    QMessageBox::information(
        this, "Success", "Enrolment removed."
    );
}

// ============================================================
// LECTURER ASSIGNMENTS
// ============================================================

void CourseManagementWindow::refreshLecturerOptions()
{
    std::vector<UserRecord> users;
    std::vector<CourseRecord> courses;
    std::string error;

    if (!database.getUsers(users, error)) {
        showError(error);
        return;
    }

    if (!courseService.getCourses(role, courses, error)) {
        showError(error);
        return;
    }

    lecturerCombo->clear();
    lecturerCourseCombo->clear();

    for (const auto& user : users) {
        if (user.role == "LECTURER" && user.active) {
            lecturerCombo->addItem(
                QString::fromStdString(user.username),
                user.userId
            );
        }
    }

    for (const auto& course : courses) {
        lecturerCourseCombo->addItem(
            QString::fromStdString(
                course.courseCode + " - " + course.name),
            course.courseId
        );
    }

    if (lecturerCombo->count() == 0) {
        lecturerCombo->addItem(
            "No active lecturers available", -1
        );
    }

    if (lecturerCourseCombo->count() == 0) {
        lecturerCourseCombo->addItem(
            "No courses available", -1
        );
    }
}

void CourseManagementWindow::refreshLecturerAssignments()
{
    std::vector<CourseLecturerRecord> assignments;
    std::string error;

    if (!database.getCourseLecturers(assignments, error)) {
        showError(error);
        return;
    }

    lecturerAssignmentsTable->setRowCount(0);

    for (const auto& assignment : assignments) {
        const int row = lecturerAssignmentsTable->rowCount();
        lecturerAssignmentsTable->insertRow(row);

        lecturerAssignmentsTable->setItem(row, 0,
            new QTableWidgetItem(
                QString::number(assignment.courseId)));

        lecturerAssignmentsTable->setItem(row, 1,
            new QTableWidgetItem(
                QString::fromStdString(assignment.courseCode)));

        lecturerAssignmentsTable->setItem(row, 2,
            new QTableWidgetItem(
                QString::fromStdString(assignment.courseName)));

        lecturerAssignmentsTable->setItem(row, 3,
            new QTableWidgetItem(
                QString::number(assignment.lecturerId)));

        lecturerAssignmentsTable->setItem(row, 4,
            new QTableWidgetItem(
                QString::fromStdString(
                    assignment.lecturerUsername)));
    }
}

void CourseManagementWindow::addLecturerAssignment()
{
    const int lecturerId =
        lecturerCombo->currentData().toInt();

    const int courseId =
        lecturerCourseCombo->currentData().toInt();

    if (lecturerId <= 0 || courseId <= 0) {
        showError("Select a valid lecturer and course.");
        return;
    }

    std::string error;

    if (!database.assignLecturerToCourse(
            courseId, lecturerId, error)) {
        showError(error);
        return;
    }

    refreshLecturerAssignments();

    QMessageBox::information(
        this,
        "Success",
        "Lecturer assigned to course successfully."
    );
}

void CourseManagementWindow::removeSelectedLecturerAssignment()
{
    const int row = lecturerAssignmentsTable->currentRow();

    if (row < 0) {
        showError("Select a lecturer assignment first.");
        return;
    }

    const int courseId =
        lecturerAssignmentsTable->item(row, 0)->text().toInt();

    const int lecturerId =
        lecturerAssignmentsTable->item(row, 3)->text().toInt();

    if (QMessageBox::question(
            this,
            "Confirm Removal",
            "Remove this lecturer from the course?"
        ) != QMessageBox::Yes) {
        return;
    }

    std::string error;

    if (!database.removeLecturerFromCourse(
            courseId, lecturerId, error)) {
        showError(error);
        return;
    }

    refreshLecturerAssignments();

    QMessageBox::information(
        this, "Success", "Lecturer assignment removed."
    );
}

// ============================================================
// HOD ASSIGNMENTS
// ============================================================

void CourseManagementWindow::refreshHodOptions()
{
    std::vector<UserRecord> users;
    std::string error;

    if (!database.getUsers(users, error)) {
        showError(error);
        return;
    }

    hodUserCombo->clear();

    for (const auto& user : users) {
        if (user.role == "HOD" && user.active) {
            hodUserCombo->addItem(
                QString::fromStdString(user.username),
                user.userId
            );
        }
    }

    if (hodUserCombo->count() == 0) {
        hodUserCombo->addItem(
            "No active HoD accounts available", -1
        );
    }
}

void CourseManagementWindow::assignHod()
{
    const int departmentId =
        hodDepartmentCombo->currentData().toInt();

    const int hodId =
        hodUserCombo->currentData().toInt();

    if (departmentId <= 0 || hodId <= 0) {
        showError(
            "Select a valid department and active HoD account."
        );
        return;
    }

    if (QMessageBox::question(
            this,
            "Confirm HoD Assignment",
            "Assign this user as HoD for the selected department? "
            "The department's current HoD will be replaced."
        ) != QMessageBox::Yes) {
        return;
    }

    std::string error;

    if (!database.assignHodToDepartment(
            departmentId, hodId, error)) {
        showError(error);
        return;
    }

    QMessageBox::information(
        this,
        "Success",
        "HoD assigned to department successfully."
    );
}

// ============================================================
// ERROR DIALOG
// ============================================================

void CourseManagementWindow::showError(
    const std::string& message
)
{
    QMessageBox::warning(
        this,
        "LMS Administration",
        QString::fromStdString(message)
    );
}

void CourseManagementWindow::selectTab(int index)
{
    if (tabs == nullptr)
        return;

    if (index >= 0 && index < tabs->count())
        tabs->setCurrentIndex(index);
}
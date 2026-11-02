#ifndef COURSEMANAGEMENTWINDOW_H
#define COURSEMANAGEMENTWINDOW_H

#include "CourseService.h"

#include <QWidget>
#include <string>

class QComboBox;
class QLineEdit;
class QTableWidget;
class QTabWidget;
class QCheckBox;
class QDateEdit;

class CourseManagementWindow : public QWidget
{
public:
    explicit CourseManagementWindow(
        Database& database,
        const std::string& role,
        int currentUserId = 0,
        QWidget* parent = nullptr
    );

    // Allow the dashboard to open a specific management tab.
    void selectTab(int index);

private:
    Database& database;
    CourseService courseService;
    std::string role;
    int currentUserId;
    QTabWidget* tabs = nullptr;

    // Department and course management
    QLineEdit* departmentNameInput;
    QLineEdit* courseCodeInput;
    QLineEdit* courseNameInput;
    QComboBox* departmentCombo;
    QTableWidget* coursesTable;

    // User management
    QTableWidget* usersTable;

    // Student enrolments
    QComboBox* enrolStudentCombo;
    QComboBox* enrolCourseCombo;
    QTableWidget* enrolmentsTable;

    // Lecturer assignments
    QComboBox* lecturerCombo;
    QComboBox* lecturerCourseCombo;
    QTableWidget* lecturerAssignmentsTable;

    // HoD assignments
    QComboBox* hodDepartmentCombo;
    QComboBox* hodUserCombo;

    // Administrator-only audit trail
    QComboBox* auditActionCombo = nullptr;
    QComboBox* auditEntityCombo = nullptr;
    QLineEdit* auditActorInput = nullptr;
    QLineEdit* auditSearchInput = nullptr;
    QCheckBox* auditDateRangeCheck = nullptr;
    QDateEdit* auditStartDate = nullptr;
    QDateEdit* auditEndDate = nullptr;
    QTableWidget* auditTable = nullptr;

    // Main interface
    void setupUi();
    void refreshAll();

    // Departments and courses
    void refreshDepartments();
    void refreshCourses();
    void addDepartment();
    void addCourse();

    // Users
    void refreshUsers();
    void toggleSelectedUser();

    // Student enrolments
    void refreshEnrolmentOptions();
    void refreshEnrolments();
    void addEnrolment();
    void removeSelectedEnrolment();

    // Lecturer assignments
    void refreshLecturerOptions();
    void refreshLecturerAssignments();
    void addLecturerAssignment();
    void removeSelectedLecturerAssignment();

    // HoD assignments
    void refreshHodOptions();
    void assignHod();

    // Audit trail
    void refreshAuditTrail();

    // Error handling
    void showError(const std::string& message);

};

#endif
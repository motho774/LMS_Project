#ifndef LECTURERWINDOW_H
#define LECTURERWINDOW_H

#include "Database.h"
#include <QWidget>

class QComboBox;
class QDateTimeEdit;
class QLineEdit;
class QPlainTextEdit;
class QSpinBox;
class QTabWidget;
class QTableWidget;

class LecturerWindow : public QWidget
{
public:
    explicit LecturerWindow(Database& database, int lecturerId,
                            QWidget* parent = nullptr);

    void selectTab(int index);

private:
    Database& database;
    int lecturerId;
    QTabWidget* tabs;

    QComboBox* courseCombo;
    QLineEdit* assignmentTitle;
    QPlainTextEdit* assignmentDescription;
    QDateTimeEdit* assignmentDeadline;
    QLineEdit* maxMarkInput;
    QSpinBox* maxAttemptsInput;

    QTableWidget* coursesTable;
    QTableWidget* assignmentsTable;
    QTableWidget* submissionsTable;

    void setupUi();
    void loadCourses();
    void loadAssignments();
    void loadSubmissions();
    void downloadSelectedSubmission();
    void markSelectedSubmission();
    void createAssignment();
    void grantIndividualExtension();
};

#endif

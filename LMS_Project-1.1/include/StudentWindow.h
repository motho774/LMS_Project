#ifndef STUDENTWINDOW_H
#define STUDENTWINDOW_H

#include "Database.h"
#include <QWidget>

class QTabWidget;
class QTableWidget;
class QLabel;
class QPushButton;

class StudentWindow : public QWidget {
public:
    explicit StudentWindow(
        Database& database,
        int studentId,
        QWidget* parent = nullptr
    );

    void selectTab(int index);

private:
    Database& database;
    int studentId;
    int selectedCourseId;

    QTabWidget* tabs;
    QTableWidget* coursesTable;
    QTableWidget* assignmentsTable;
    QTableWidget* submissionsTable;
    QTableWidget* marksTable;
    QLabel* assignmentDetailsLabel;
    QPushButton* submitButton;

    void setupUi();

    void loadCourses();
    void loadAssignments();
    void showSelectedAssignmentDetails();
    void updateSubmissionButtonState();
    void loadSubmissions();
    void loadMarks();

    void submitSelectedAssignment();
};

#endif
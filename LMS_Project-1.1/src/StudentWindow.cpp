#include "StudentWindow.h"

#include <QTabWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QDateTime>
#include <QFileInfo>
#include <QMessageBox>
#include <QLabel>
#include <QAbstractItemView>
#include <QString>
#include <QStringList>

StudentWindow::StudentWindow(
    Database& database,
    int studentId,
    QWidget* parent
)
    : QWidget(parent),
      database(database),
      studentId(studentId),
      selectedCourseId(0),
      tabs(nullptr),
      coursesTable(nullptr),
      assignmentsTable(nullptr),
      submissionsTable(nullptr),
      marksTable(nullptr),
      assignmentDetailsLabel(nullptr),
      submitButton(nullptr)
{
    setupUi();

    loadCourses();
    loadAssignments();
    loadSubmissions();
    loadMarks();
}

void StudentWindow::selectTab(int index)
{
    if (tabs && index >= 0 && index < tabs->count())
        tabs->setCurrentIndex(index);
}

void StudentWindow::setupUi()
{
    setWindowTitle("LMS - Student Workspace");
    resize(1100, 650);
    setMinimumSize(800, 500);

    auto* mainLayout = new QVBoxLayout(this);

    auto* heading = new QLabel("Student Workspace", this);
    heading->setStyleSheet(
        "font-size: 22px;"
        "font-weight: bold;"
        "color: #1E3A8A;"
    );

    auto* subtitle = new QLabel(
        "View your courses, assignments, submissions and released marks.",
        this
    );

    tabs = new QTabWidget(this);

    // Tab 1: My Courses
    auto* coursesPage = new QWidget(this);
    auto* coursesLayout = new QVBoxLayout(coursesPage);

    coursesTable = new QTableWidget(coursesPage);
    coursesTable->setColumnCount(4);
    coursesTable->setHorizontalHeaderLabels({
        "Course Code", "Course Name", "Department", "Course ID"
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

    auto* refreshCoursesButton =
        new QPushButton("Refresh Courses", coursesPage);
    auto* viewCourseAssignmentsButton =
        new QPushButton("View Selected Course Assignments", coursesPage);

    coursesLayout->addWidget(coursesTable);
    coursesLayout->addWidget(refreshCoursesButton);
    coursesLayout->addWidget(viewCourseAssignmentsButton);

    QObject::connect(
        refreshCoursesButton,
        &QPushButton::clicked,
        this,
        [this]() { loadCourses(); }
    );

    QObject::connect(
        viewCourseAssignmentsButton,
        &QPushButton::clicked,
        this,
        [this]() {
            const int row = coursesTable->currentRow();
            auto* idItem = row >= 0 ? coursesTable->item(row, 3) : nullptr;
            if (!idItem) {
                QMessageBox::information(
                    this, "Select Course",
                    "Select a course before viewing its assignments."
                );
                return;
            }

            selectedCourseId = idItem->data(Qt::UserRole).toInt();
            loadAssignments();
            tabs->setCurrentIndex(1);
        }
    );

    tabs->addTab(coursesPage, "My Courses");


    // Tab 2: View Assignments
    auto* assignmentsPage = new QWidget(this);
    auto* assignmentsLayout = new QVBoxLayout(assignmentsPage);

    assignmentsTable = new QTableWidget(assignmentsPage);
    assignmentsTable->setColumnCount(8);
    assignmentsTable->setHorizontalHeaderLabels({
        "Assignment ID",
        "Course",
        "Assignment",
        "Deadline",
        "Max Mark",
        "Attempt / Max",
        "Status",
        "Deadline Extension"
    });
    assignmentsTable->setEditTriggers(
        QAbstractItemView::NoEditTriggers
    );
    assignmentsTable->setSelectionBehavior(
        QAbstractItemView::SelectRows
    );
    assignmentsTable->setSelectionMode(
        QAbstractItemView::SingleSelection
    );
    assignmentsTable->horizontalHeader()->setSectionResizeMode(
        QHeaderView::ResizeToContents
    );
    assignmentsTable->horizontalHeader()->setStretchLastSection(true);

    auto* assignmentButtons = new QHBoxLayout();

    auto* refreshAssignmentsButton =
        new QPushButton("Refresh Assignments", assignmentsPage);

    auto* allCoursesButton =
        new QPushButton("Show All Courses", assignmentsPage);

    submitButton =
        new QPushButton("Submit Selected Assignment", assignmentsPage);

    assignmentButtons->addWidget(refreshAssignmentsButton);
    assignmentButtons->addWidget(allCoursesButton);
    assignmentButtons->addWidget(submitButton);

    assignmentsLayout->addWidget(assignmentsTable);
    assignmentDetailsLabel = new QLabel(
        "Select an assignment to view its details and applicable deadline.",
        assignmentsPage
    );
    assignmentDetailsLabel->setWordWrap(true);
    assignmentsLayout->addWidget(assignmentDetailsLabel);
    assignmentsLayout->addLayout(assignmentButtons);

    QObject::connect(
        refreshAssignmentsButton,
        &QPushButton::clicked,
        this,
        [this]() {
            loadAssignments();
        }
    );

    QObject::connect(
        allCoursesButton,
        &QPushButton::clicked,
        this,
        [this]() {
            selectedCourseId = 0;
            loadAssignments();
        }
    );

    QObject::connect(
        assignmentsTable,
        &QTableWidget::itemSelectionChanged,
        this,
        [this]() {
            showSelectedAssignmentDetails();
            updateSubmissionButtonState();
        }
    );

    QObject::connect(
        submitButton,
        &QPushButton::clicked,
        this,
        [this]() {
            submitSelectedAssignment();
        }
    );

    tabs->addTab(assignmentsPage, "View Assignments");


    // Tab 3: My Submissions
    auto* submissionsPage = new QWidget(this);
    auto* submissionsLayout = new QVBoxLayout(submissionsPage);

    submissionsTable = new QTableWidget(submissionsPage);
    submissionsTable->setColumnCount(7);
    submissionsTable->setHorizontalHeaderLabels({
        "Submission ID",
        "Course",
        "Assignment",
        "Version",
        "Submitted At",
        "Status",
        "File Reference"
    });
    submissionsTable->setEditTriggers(
        QAbstractItemView::NoEditTriggers
    );
    submissionsTable->setSelectionBehavior(
        QAbstractItemView::SelectRows
    );
    submissionsTable->setSelectionMode(
        QAbstractItemView::SingleSelection
    );
    submissionsTable->horizontalHeader()->setSectionResizeMode(
        QHeaderView::ResizeToContents
    );
    submissionsTable->horizontalHeader()->setStretchLastSection(true);

    auto* refreshSubmissionsButton =
        new QPushButton("Refresh Submissions", submissionsPage);

    submissionsLayout->addWidget(submissionsTable);
    submissionsLayout->addWidget(refreshSubmissionsButton);

    QObject::connect(
        refreshSubmissionsButton,
        &QPushButton::clicked,
        this,
        [this]() {
            loadSubmissions();
        }
    );

    tabs->addTab(submissionsPage, "My Submissions");


    // Tab 4: My Marks and Feedback
    auto* marksPage = new QWidget(this);
    auto* marksLayout = new QVBoxLayout(marksPage);

    auto* marksNotice = new QLabel(
        "Only marks released by your Head of Department are displayed.",
        marksPage
    );

    marksTable = new QTableWidget(marksPage);
    marksTable->setColumnCount(7);
    marksTable->setHorizontalHeaderLabels({
        "Course",
        "Assignment",
        "Version",
        "Raw Mark",
        "Final Mark",
        "Maximum Mark",
        "Feedback"
    });
    marksTable->setEditTriggers(
        QAbstractItemView::NoEditTriggers
    );
    marksTable->setSelectionBehavior(
        QAbstractItemView::SelectRows
    );
    marksTable->horizontalHeader()->setSectionResizeMode(
        QHeaderView::ResizeToContents
    );
    marksTable->horizontalHeader()->setStretchLastSection(true);

    auto* refreshMarksButton =
        new QPushButton("Refresh Released Marks", marksPage);

    marksLayout->addWidget(marksNotice);
    marksLayout->addWidget(marksTable);
    marksLayout->addWidget(refreshMarksButton);

    QObject::connect(
        refreshMarksButton,
        &QPushButton::clicked,
        this,
        [this]() {
            loadMarks();
        }
    );

    tabs->addTab(marksPage, "My Marks and Feedback");

    mainLayout->addWidget(heading);
    mainLayout->addWidget(subtitle);
    mainLayout->addWidget(tabs);
}

void StudentWindow::loadCourses()
{
    std::vector<CourseRecord> courses;

    if (!database.getStudentCourses(studentId, courses)) {
        QMessageBox::warning(
            this, "Database Error",
            "Could not load your courses."
        );
        return;
    }

    coursesTable->setRowCount(0);

    for (const auto& course : courses) {
        int row = coursesTable->rowCount();
        coursesTable->insertRow(row);

        coursesTable->setItem(
            row, 0,
            new QTableWidgetItem(
                QString::fromStdString(course.courseCode)
            )
        );

        coursesTable->setItem(
            row, 1,
            new QTableWidgetItem(
                QString::fromStdString(course.name)
            )
        );

        coursesTable->setItem(
            row, 2,
            new QTableWidgetItem(
                QString::fromStdString(course.departmentName)
            )
        );

        auto* courseIdItem = new QTableWidgetItem(
            QString::number(course.courseId)
        );
        courseIdItem->setData(Qt::UserRole, course.courseId);
        coursesTable->setItem(row, 3, courseIdItem);
    }
}

void StudentWindow::loadAssignments()
{
    std::vector<StudentAssignmentRecord> assignments;

    if (!database.getStudentAssignments(studentId, assignments)) {
        QMessageBox::warning(
            this, "Database Error",
            "Could not load assignments."
        );
        return;
    }

    assignmentsTable->setRowCount(0);

    for (const auto& a : assignments) {
        if (selectedCourseId != 0 && a.courseId != selectedCourseId)
            continue;

        int row = assignmentsTable->rowCount();
        assignmentsTable->insertRow(row);

        auto* idItem = new QTableWidgetItem(
            QString::number(a.assignmentId)
        );

        // Store the actual assignment ID independently of display text.
        idItem->setData(Qt::UserRole, a.assignmentId);

        assignmentsTable->setItem(row, 0, idItem);

        assignmentsTable->setItem(
            row, 1,
            new QTableWidgetItem(
                QString::fromStdString(a.courseCode)
            )
        );

        assignmentsTable->setItem(
            row, 2,
            new QTableWidgetItem(
                QString::fromStdString(a.title)
            )
        );

        assignmentsTable->setItem(
            row, 3,
            new QTableWidgetItem(
                QString::fromStdString(a.deadline)
            )
        );

        assignmentsTable->setItem(
            row, 4,
            new QTableWidgetItem(
                QString::number(a.maxMark, 'f', 2)
            )
        );

        assignmentsTable->setItem(
            row, 5,
            new QTableWidgetItem(
                    QString("%1 / %2").arg(a.latestVersion).arg(a.maxAttempts)
            )
        );

        assignmentsTable->setItem(
            row, 6,
            new QTableWidgetItem(
                QString::fromStdString(a.latestStatus)
            )
        );

        assignmentsTable->setItem(
            row, 7,
            new QTableWidgetItem(
                QString::fromStdString(a.deadlineExtension)
            )
        );

        // Show the description when the assignment title is hovered.
        assignmentsTable->item(row, 2)->setToolTip(
            QString::fromStdString(a.description)
        );
    }

    assignmentDetailsLabel->setText(
        "Select an assignment to view its details and applicable deadline."
    );
    updateSubmissionButtonState();
}

void StudentWindow::showSelectedAssignmentDetails()
{
    const int row = assignmentsTable->currentRow();
    if (row < 0) {
        assignmentDetailsLabel->setText(
            "Select an assignment to view its details and applicable deadline."
        );
        return;
    }

    const auto* courseItem = assignmentsTable->item(row, 1);
    const auto* titleItem = assignmentsTable->item(row, 2);
    const auto* deadlineItem = assignmentsTable->item(row, 3);
    const auto* maxMarkItem = assignmentsTable->item(row, 4);
    const auto* descriptionItem = assignmentsTable->item(row, 2);
    const auto* attemptsItem = assignmentsTable->item(row, 5);

    if (!courseItem || !titleItem || !deadlineItem ||
        !maxMarkItem || !descriptionItem || !attemptsItem) {
        assignmentDetailsLabel->setText(
            "Assignment details are unavailable."
        );
        return;
    }

    assignmentDetailsLabel->setText(
        QString("%1 — %2\nDeadline: %3\nMaximum mark: %4\n"
                "Attempts used / allowed: %5\n%6")
            .arg(courseItem->text(), titleItem->text(),
                 deadlineItem->text(), maxMarkItem->text(),
                 attemptsItem->text(), descriptionItem->toolTip())
    );
}

void StudentWindow::updateSubmissionButtonState()
{
    if (!submitButton)
        return;

    const int row = assignmentsTable->currentRow();
    if (row < 0) {
        submitButton->setEnabled(false);
        submitButton->setToolTip("Select an assignment first.");
        return;
    }

    bool attemptOk = false;
    bool limitOk = false;
    const QStringList attempts = assignmentsTable->item(row, 5)
        ? assignmentsTable->item(row, 5)->text().split(" / ")
        : QStringList();
    const int attemptsUsed = attempts.value(0).toInt(&attemptOk);
    const int maxAttempts = attempts.value(1).toInt(&limitOk);
    const QString deadlineText = assignmentsTable->item(row, 3)
        ? assignmentsTable->item(row, 3)->text() : QString();
    const QDateTime deadline = QDateTime::fromString(
        deadlineText, "yyyy-MM-dd HH:mm:ss"
    );
    const bool resubmissionExpired =
        attemptsUsed > 0 && deadline.isValid() &&
        QDateTime::currentDateTime() > deadline;
    const bool attemptsExhausted =
        attemptOk && limitOk && attemptsUsed >= maxAttempts;

    const bool canSubmit = attemptOk && limitOk &&
        !attemptsExhausted && !resubmissionExpired;
    submitButton->setEnabled(canSubmit);
    if (attemptsExhausted) {
        submitButton->setToolTip(
            "The maximum number of submissions has been reached."
        );
    } else if (resubmissionExpired) {
        submitButton->setToolTip(
            "Resubmissions are not allowed after the applicable deadline."
        );
    } else {
        submitButton->setToolTip(QString());
    }
}

void StudentWindow::loadSubmissions()
{
    std::vector<SubmissionRecord> submissions;

    if (!database.getStudentSubmissions(studentId, submissions)) {
        QMessageBox::warning(
            this, "Database Error",
            "Could not load your submissions."
        );
        return;
    }

    submissionsTable->setRowCount(0);

    for (const auto& s : submissions) {
        int row = submissionsTable->rowCount();
        submissionsTable->insertRow(row);

        submissionsTable->setItem(
            row, 0,
            new QTableWidgetItem(
                QString::number(s.submissionId)
            )
        );

        submissionsTable->setItem(
            row, 1,
            new QTableWidgetItem(
                QString::fromStdString(s.courseCode)
            )
        );

        submissionsTable->setItem(
            row, 2,
            new QTableWidgetItem(
                QString::fromStdString(s.assignmentTitle)
            )
        );

        submissionsTable->setItem(
            row, 3,
            new QTableWidgetItem(
                QString::number(s.versionNumber)
            )
        );

        submissionsTable->setItem(
            row, 4,
            new QTableWidgetItem(
                QString::fromStdString(s.submittedAt)
            )
        );

        submissionsTable->setItem(
            row, 5,
            new QTableWidgetItem(
                QString::fromStdString(s.status)
            )
        );

        submissionsTable->setItem(
            row, 6,
            new QTableWidgetItem(
                QString::fromStdString(s.fileReference)
            )
        );
    }
}

void StudentWindow::loadMarks()
{
    std::vector<StudentMarkRecord> marks;

    if (!database.getStudentMarks(studentId, marks)) {
        QMessageBox::warning(
            this, "Database Error",
            "Could not load your released marks."
        );
        return;
    }

    marksTable->setRowCount(0);

    for (const auto& m : marks) {
        int row = marksTable->rowCount();
        marksTable->insertRow(row);

        marksTable->setItem(
            row, 0,
            new QTableWidgetItem(
                QString::fromStdString(m.courseCode)
            )
        );

        marksTable->setItem(
            row, 1,
            new QTableWidgetItem(
                QString::fromStdString(m.assignmentTitle)
            )
        );

        marksTable->setItem(
            row, 2,
            new QTableWidgetItem(
                QString::number(m.versionNumber)
            )
        );

        marksTable->setItem(
            row, 3,
            new QTableWidgetItem(
                QString::number(m.rawMark, 'f', 2)
            )
        );

        marksTable->setItem(
            row, 4,
            new QTableWidgetItem(
                QString::number(m.finalMark, 'f', 2)
            )
        );

        marksTable->setItem(
            row, 5,
            new QTableWidgetItem(
                QString::number(m.maxMark, 'f', 2)
            )
        );

        marksTable->setItem(
            row, 6,
            new QTableWidgetItem(
                QString::fromStdString(m.feedback)
            )
        );
    }
}

void StudentWindow::submitSelectedAssignment()
{
    int row = assignmentsTable->currentRow();

    if (row < 0) {
        QMessageBox::information(
            this, "Select Assignment",
            "Select an assignment before submitting."
        );
        return;
    }

    auto* idItem = assignmentsTable->item(row, 0);

    if (!idItem) {
        QMessageBox::warning(
            this, "Selection Error",
            "Could not determine the selected assignment."
        );
        return;
    }

    const int assignmentId =
        idItem->data(Qt::UserRole).toInt();

    const QString filePath = QFileDialog::getOpenFileName(
        this,
        "Select Assignment File",
        QString(),
        "Documents (*.pdf *.doc *.docx *.txt *.zip);;"
        "All Files (*)"
    );

    if (filePath.isEmpty())
        return;

    const QFileInfo selectedFile(filePath);
    if (!selectedFile.exists() || !selectedFile.isFile() ||
        !selectedFile.isReadable()) {
        QMessageBox::warning(
            this, "Invalid Submission File",
            "Select an existing, readable file to submit."
        );
        return;
    }

    const QString course = assignmentsTable->item(row, 1)->text();
    const QString title = assignmentsTable->item(row, 2)->text();
    const QString deadline = assignmentsTable->item(row, 3)->text();
    const bool confirmed = QMessageBox::question(
        this,
        "Confirm Submission",
        QString("Submit this file for %1 (%2)?\n\n"
                "Applicable deadline: %3\n\nFile: %4")
            .arg(title, course, deadline, filePath)
    ) == QMessageBox::Yes;

    if (!confirmed)
        return;

    if (!database.submitAssignment(
            studentId,
            assignmentId,
            filePath.toStdString())) {
        QMessageBox::warning(
            this, "Submission Failed",
            "The submission could not be saved. "
            "Check your enrolment and database output."
        );
        return;
    }

    QMessageBox::information(
        this, "Submission Successful",
        "Your submission has been recorded."
    );

    loadAssignments();
    loadSubmissions();

    tabs->setCurrentIndex(2);
}
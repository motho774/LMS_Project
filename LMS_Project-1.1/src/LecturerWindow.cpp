
#include "LecturerWindow.h"
#include "MarkingService.h"

#include <QComboBox>
#include <QDateTimeEdit>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleValidator>
#include <QDoubleSpinBox>
#include <QDateTime>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

#include <algorithm>
#include <vector>

LecturerWindow::LecturerWindow(
    Database& database,
    int lecturerId,
    QWidget* parent
)
    : QWidget(parent),
      database(database),
      lecturerId(lecturerId),
      tabs(nullptr),
      courseCombo(nullptr),
      assignmentTitle(nullptr),
      assignmentDescription(nullptr),
      assignmentDeadline(nullptr),
      maxMarkInput(nullptr),
      maxAttemptsInput(nullptr),
      coursesTable(nullptr),
      assignmentsTable(nullptr),
      submissionsTable(nullptr)
{
    setupUi();
    loadCourses();
    loadAssignments();
    loadSubmissions();
}


void LecturerWindow::setupUi()
{
    setWindowTitle("Lecturer Workspace");
    resize(1000, 650);

    auto* mainLayout = new QVBoxLayout(this);

    auto* heading = new QLabel("Lecturer Workspace");
    heading->setStyleSheet(
        "font-size: 22px; font-weight: bold; color: #1E3A8A;"
    );
    mainLayout->addWidget(heading);

    tabs = new QTabWidget(this);

    // My Courses tab
    auto* coursesPage = new QWidget();
    auto* coursesLayout = new QVBoxLayout(coursesPage);

    coursesTable = new QTableWidget(coursesPage);
    coursesTable->setColumnCount(4);
    coursesTable->setHorizontalHeaderLabels(
        {"Course ID", "Course Code", "Course Name", "Department"}
    );
    coursesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    coursesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    coursesTable->horizontalHeader()->setSectionResizeMode(
        QHeaderView::Stretch
    );

    auto* refreshCoursesButton = new QPushButton("Refresh Courses");
    coursesLayout->addWidget(coursesTable);
    coursesLayout->addWidget(refreshCoursesButton);

    QObject::connect(
        refreshCoursesButton, &QPushButton::clicked,
        this, [this]() { loadCourses(); }
    );

    tabs->addTab(coursesPage, "My Courses");

    // Manage Assignments tab
    auto* assignmentsPage = new QWidget();
    auto* assignmentsLayout = new QVBoxLayout(assignmentsPage);

    auto* form = new QFormLayout();

    courseCombo = new QComboBox();
    assignmentTitle = new QLineEdit();
    assignmentDescription = new QPlainTextEdit();
    assignmentDescription->setMaximumHeight(90);

    assignmentDeadline = new QDateTimeEdit(QDateTime::currentDateTime());
    assignmentDeadline->setCalendarPopup(true);
    assignmentDeadline->setDisplayFormat("yyyy-MM-dd HH:mm:ss");

    maxMarkInput = new QLineEdit();
    maxMarkInput->setPlaceholderText("e.g. 100");
    maxMarkInput->setValidator(
        new QDoubleValidator(0.01, 1000000.0, 2, maxMarkInput)
    );
    maxAttemptsInput = new QSpinBox();
    maxAttemptsInput->setRange(1, 100);
    maxAttemptsInput->setValue(2);

    form->addRow("Course:", courseCombo);
    form->addRow("Assignment title:", assignmentTitle);
    form->addRow("Description:", assignmentDescription);
    form->addRow("Deadline:", assignmentDeadline);
    form->addRow("Maximum mark:", maxMarkInput);
    form->addRow("Maximum submissions (including first):", maxAttemptsInput);

    auto* createButton = new QPushButton("Create Assignment");
    auto* refreshAssignmentsButton = new QPushButton("Refresh Assignments");
    auto* grantExtensionButton =
        new QPushButton("Grant Deadline Extension");

    auto* assignmentButtons = new QHBoxLayout();
    assignmentButtons->addWidget(createButton);
    assignmentButtons->addWidget(refreshAssignmentsButton);
    assignmentButtons->addWidget(grantExtensionButton);

    assignmentsTable = new QTableWidget(assignmentsPage);
    assignmentsTable->setColumnCount(6);
    assignmentsTable->setHorizontalHeaderLabels(
        {"ID", "Course", "Title", "Deadline", "Maximum Mark", "Max Attempts"}
    );
    assignmentsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    assignmentsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    assignmentsTable->horizontalHeader()->setSectionResizeMode(
        QHeaderView::Stretch
    );

    assignmentsLayout->addLayout(form);
    assignmentsLayout->addLayout(assignmentButtons);
    assignmentsLayout->addWidget(assignmentsTable);

    QObject::connect(
        createButton, &QPushButton::clicked,
        this, [this]() { createAssignment(); }
    );

    QObject::connect(
        refreshAssignmentsButton, &QPushButton::clicked,
        this, [this]() { loadAssignments(); }
    );

    QObject::connect(
        grantExtensionButton, &QPushButton::clicked,
        this, [this]() { grantDeadlineExtension(); }
    );

    tabs->addTab(assignmentsPage, "Manage Assignments");

    // View Submissions tab
    auto* submissionsPage = new QWidget();
    auto* submissionsLayout = new QVBoxLayout(submissionsPage);

    submissionsTable = new QTableWidget(submissionsPage);
    submissionsTable->setColumnCount(8);
    submissionsTable->setHorizontalHeaderLabels(
        {"Submission ID", "Student", "Course", "Assignment",
         "Version", "Submitted At", "Status", "Final Mark"}
    );
    submissionsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    submissionsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    submissionsTable->horizontalHeader()->setSectionResizeMode(
        QHeaderView::ResizeToContents
    );

    auto* refreshSubmissionsButton =
        new QPushButton("Refresh Submissions");
    auto* downloadSubmissionButton =
        new QPushButton("Download Selected File");
    auto* markSubmissionButton =
        new QPushButton("Mark / Correct Selected Submission");

    auto* submissionActions = new QHBoxLayout();
    submissionActions->addWidget(downloadSubmissionButton);
    submissionActions->addWidget(markSubmissionButton);
    submissionActions->addWidget(refreshSubmissionsButton);

    submissionsLayout->addWidget(submissionsTable);
    submissionsLayout->addLayout(submissionActions);

    QObject::connect(
        markSubmissionButton, &QPushButton::clicked,
        this, [this]() { markSelectedSubmission(); }
    );

    QObject::connect(
        downloadSubmissionButton, &QPushButton::clicked,
        this, [this]() { downloadSelectedSubmission(); }
    );

    QObject::connect(
        refreshSubmissionsButton, &QPushButton::clicked,
        this, [this]() { loadSubmissions(); }
    );

    tabs->addTab(submissionsPage, "View Submissions");

    mainLayout->addWidget(tabs);
}

void LecturerWindow::loadCourses()
{
    std::vector<CourseRecord> courses;

    if (!database.getLecturerCourses(lecturerId, courses)) {
        QMessageBox::warning(
            this, "Database Error", "Could not load your courses."
        );
        return;
    }

    coursesTable->setRowCount(0);
    courseCombo->clear();

    for (const auto& course : courses) {
        int row = coursesTable->rowCount();
        coursesTable->insertRow(row);

        coursesTable->setItem(
            row, 0, new QTableWidgetItem(
                QString::number(course.courseId)
            )
        );
        coursesTable->setItem(
            row, 1, new QTableWidgetItem(
                QString::fromStdString(course.courseCode)
            )
        );
        coursesTable->setItem(
            row, 2, new QTableWidgetItem(
                QString::fromStdString(course.name)
            )
        );
        coursesTable->setItem(
            row, 3, new QTableWidgetItem(
                QString::fromStdString(course.departmentName)
            )
        );

        courseCombo->addItem(
            QString::fromStdString(course.courseCode + " - " + course.name),
            course.courseId
        );
    }

    if (courses.empty()) {
        courseCombo->addItem("No courses assigned", -1);
    }
}

void LecturerWindow::createAssignment()
{
    if (courseCombo->currentData().toInt() <= 0) {
        QMessageBox::warning(
            this, "No Course", "You must be assigned to a course first."
        );
        return;
    }

    QString title = assignmentTitle->text().trimmed();
    QString description = assignmentDescription->toPlainText().trimmed();
    QString deadline = assignmentDeadline->dateTime().toString(
        "yyyy-MM-dd HH:mm:ss"
    );

    bool validMark = false;
    const double maxMark =
        maxMarkInput->text().toDouble(&validMark);
    const int maxAttempts = maxAttemptsInput->value();

    if (title.isEmpty() || description.isEmpty() ||
        !validMark || maxMark <= 0) {
        QMessageBox::warning(
            this, "Invalid Details",
            "Enter an assignment title, description, and positive maximum mark."
        );
        return;
    }

    if (assignmentDeadline->dateTime() <= QDateTime::currentDateTime()) {
        QMessageBox::warning(
            this, "Invalid Deadline",
            "The deadline must be in the future."
        );
        return;
    }

    bool success = database.createAssignmentForLecturer(
        lecturerId,
        courseCombo->currentData().toInt(),
        title.toStdString(),
        description.toStdString(),
        deadline.toStdString(),
        maxMark,
        maxAttempts
    );

    if (!success) {
        QMessageBox::warning(
            this, "Assignment Error",
            "The assignment could not be created. Check your course assignment."
        );
        return;
    }

    QMessageBox::information(
        this, "Success", "Assignment created successfully."
    );

    assignmentTitle->clear();
    assignmentDescription->clear();
    maxMarkInput->clear();
    maxAttemptsInput->setValue(2);

    loadAssignments();
}

void LecturerWindow::grantDeadlineExtension()
{
    std::vector<AssignmentRecord> assignments;
    if (!database.getLecturerAssignments(lecturerId, assignments)) {
        QMessageBox::warning(
            this, "Database Error",
            "Could not load your assignments to grant an extension."
        );
        return;
    }
    if (assignments.empty()) {
        QMessageBox::information(
            this, "No Assignments",
            "Create an assignment before granting an extension."
        );
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle("Grant Deadline Extension");
    dialog.setMinimumWidth(440);

    auto* form = new QFormLayout(&dialog);
    auto* assignmentInput = new QComboBox(&dialog);
    auto* scopeInput = new QComboBox(&dialog);
    scopeInput->addItem("Individual student", "INDIVIDUAL");
    scopeInput->addItem("Whole class", "CLASS");
    auto* studentInput = new QComboBox(&dialog);
    auto* studentLabel = new QLabel("Student:", &dialog);
    auto* revisedDeadlineInput = new QDateTimeEdit(
        QDateTime::currentDateTime().addDays(7), &dialog
    );
    revisedDeadlineInput->setCalendarPopup(true);
    revisedDeadlineInput->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    revisedDeadlineInput->setMinimumDateTime(QDateTime::currentDateTime());
    auto* reasonInput = new QLineEdit(&dialog);
    reasonInput->setPlaceholderText("Reason for the extension");

    for (const auto& assignment : assignments) {
        const QString label = QString("%1 - %2 (deadline: %3)")
            .arg(QString::fromStdString(assignment.courseCode),
                 QString::fromStdString(assignment.title),
                 QString::fromStdString(assignment.deadline));
        assignmentInput->addItem(label, assignment.assignmentId);
    }

    const int selectedAssignmentRow = assignmentsTable->currentRow();
    if (selectedAssignmentRow >= 0 &&
        assignmentsTable->item(selectedAssignmentRow, 0)) {
        const int selectedAssignmentId =
            assignmentsTable->item(selectedAssignmentRow, 0)->text().toInt();
        const int comboIndex =
            assignmentInput->findData(selectedAssignmentId);
        if (comboIndex >= 0)
            assignmentInput->setCurrentIndex(comboIndex);
    }

    form->addRow("Assignment:", assignmentInput);
    form->addRow("Extension scope:", scopeInput);
    form->addRow(studentLabel, studentInput);
    form->addRow("Revised deadline:", revisedDeadlineInput);
    form->addRow("Reason:", reasonInput);

    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Save | QDialogButtonBox::Cancel,
        &dialog
    );
    form->addRow(buttons);

    const auto refreshEligibleStudents =
        [this, assignmentInput, scopeInput, studentInput, studentLabel,
         revisedDeadlineInput,
         &assignments](int) {
            studentInput->clear();
            const int assignmentId = assignmentInput->currentData().toInt();
            const bool wholeClass =
                scopeInput->currentData().toString() == "CLASS";
            studentLabel->setVisible(!wholeClass);
            studentInput->setVisible(!wholeClass);
            studentInput->setEnabled(!wholeClass);

            if (!wholeClass) {
                std::vector<UserRecord> students;
                std::string error;
                if (!database.getAssignmentStudents(
                        lecturerId, assignmentId, students, error)) {
                    QMessageBox::warning(
                        this, "Database Error", QString::fromStdString(error)
                    );
                    return;
                }

                for (const auto& student : students) {
                    studentInput->addItem(
                        QString::fromStdString(student.username),
                        student.userId
                    );
                }
                studentInput->setEnabled(!students.empty());
                if (students.empty()) {
                    studentInput->addItem("No eligible students");
                    studentInput->setEnabled(false);
                }
            }

            const auto assignment = std::find_if(
                assignments.begin(), assignments.end(),
                [assignmentId](const AssignmentRecord& item) {
                    return item.assignmentId == assignmentId;
                }
            );
            if (assignment != assignments.end()) {
                const QDateTime currentDeadline = QDateTime::fromString(
                    QString::fromStdString(assignment->deadline),
                    "yyyy-MM-dd HH:mm:ss"
                );
                QDateTime defaultDeadline =
                    QDateTime::currentDateTime().addDays(7);
                if (currentDeadline.isValid() &&
                    defaultDeadline <= currentDeadline) {
                    defaultDeadline = currentDeadline.addDays(1);
                }
                revisedDeadlineInput->setDateTime(defaultDeadline);
            }
        };

    QObject::connect(
        assignmentInput,
        qOverload<int>(&QComboBox::currentIndexChanged),
        &dialog,
        refreshEligibleStudents
    );
    QObject::connect(
        scopeInput,
        qOverload<int>(&QComboBox::currentIndexChanged),
        &dialog,
        refreshEligibleStudents
    );
    refreshEligibleStudents(assignmentInput->currentIndex());

    QObject::connect(
        buttons, &QDialogButtonBox::rejected,
        &dialog, &QDialog::reject
    );
    QObject::connect(
        buttons, &QDialogButtonBox::accepted,
        &dialog,
        [&dialog, scopeInput, studentInput, reasonInput]() {
            const bool wholeClass =
                scopeInput->currentData().toString() == "CLASS";
            if ((!wholeClass && studentInput->currentData().toInt() <= 0) ||
                reasonInput->text().trimmed().isEmpty()) {
                QMessageBox::warning(
                    &dialog, "Required Details",
                    wholeClass
                        ? "Enter a reason for the class extension."
                        : "Select an eligible student and enter a reason."
                );
                return;
            }
            dialog.accept();
        }
    );

    if (dialog.exec() != QDialog::Accepted)
        return;

    const bool wholeClass =
        scopeInput->currentData().toString() == "CLASS";
    const int assignmentId = assignmentInput->currentData().toInt();
    const int studentId = studentInput->currentData().toInt();
    const QString studentName = studentInput->currentText();
    const QString assignmentLabel = assignmentInput->currentText();
    const std::string revisedDeadline =
        revisedDeadlineInput->dateTime()
            .toString("yyyy-MM-dd HH:mm:ss").toStdString();
    const std::string reason =
        reasonInput->text().trimmed().toStdString();

    std::string error;
    const bool granted = wholeClass
        ? database.grantClassAssignmentExtension(
              lecturerId, assignmentId, revisedDeadline, reason, error)
        : database.grantAssignmentExtension(
              lecturerId, assignmentId, studentId, revisedDeadline,
              reason, error);
    if (!granted) {
        QMessageBox::warning(
            this, "Extension Failed", QString::fromStdString(error)
        );
        return;
    }

    QMessageBox::information(
        this, "Extension Granted",
        wholeClass
            ? QString("The class deadline extension for %1 has been saved until %2.")
                  .arg(assignmentLabel, QString::fromStdString(revisedDeadline))
            : QString("The deadline extension for %1 on %2 has been saved until %3.")
                  .arg(studentName, assignmentLabel,
                       QString::fromStdString(revisedDeadline))
    );
    loadAssignments();
}

void LecturerWindow::loadAssignments()
{
    std::vector<AssignmentRecord> assignments;

    if (!database.getLecturerAssignments(lecturerId, assignments)) {
        QMessageBox::warning(
            this, "Database Error", "Could not load assignments."
        );
        return;
    }

    assignmentsTable->setRowCount(0);

    for (const auto& a : assignments) {
        int row = assignmentsTable->rowCount();
        assignmentsTable->insertRow(row);

        assignmentsTable->setItem(
            row, 0, new QTableWidgetItem(QString::number(a.assignmentId))
        );
        assignmentsTable->setItem(
            row, 1, new QTableWidgetItem(
                QString::fromStdString(a.courseCode)
            )
        );
        assignmentsTable->setItem(
            row, 2, new QTableWidgetItem(QString::fromStdString(a.title))
        );
        assignmentsTable->setItem(
            row, 3, new QTableWidgetItem(QString::fromStdString(a.deadline))
        );
        assignmentsTable->setItem(
            row, 4, new QTableWidgetItem(QString::number(a.maxMark))
        );
        assignmentsTable->setItem(
            row, 5, new QTableWidgetItem(QString::number(a.maxAttempts))
        );
    }
}

void LecturerWindow::loadSubmissions()
{
    std::vector<SubmissionRecord> submissions;

    if (!database.getLecturerSubmissions(lecturerId, submissions)) {
        QMessageBox::warning(
            this, "Database Error", "Could not load submissions."
        );
        return;
    }

    submissionsTable->setRowCount(0);

    for (const auto& s : submissions) {
        int row = submissionsTable->rowCount();
        submissionsTable->insertRow(row);

        auto* submissionIdItem = new QTableWidgetItem(
            QString::number(s.submissionId)
        );
        submissionIdItem->setData(
            Qt::UserRole, QString::fromStdString(s.fileReference)
        );
        submissionsTable->setItem(row, 0, submissionIdItem);
        submissionsTable->setItem(
            row, 1, new QTableWidgetItem(
                QString::fromStdString(s.studentUsername)
            )
        );
        submissionsTable->setItem(
            row, 2, new QTableWidgetItem(
                QString::fromStdString(s.courseCode)
            )
        );
        submissionsTable->setItem(
            row, 3, new QTableWidgetItem(
                QString::fromStdString(s.assignmentTitle)
            )
        );
        submissionsTable->setItem(
            row, 4, new QTableWidgetItem(QString::number(s.versionNumber))
        );
        submissionsTable->setItem(
            row, 5, new QTableWidgetItem(
                QString::fromStdString(s.submittedAt)
            )
        );
        QString statusText = QString::fromStdString(s.status);
        if (!s.isCurrentSubmission) {
            statusText = "SUPERSEDED - NOT FOR MARKING";
        } else if (s.latestDecision == "RETURNED") {
            statusText = "RETURNED - HOD REVIEW REQUIRED";
        }
        auto* statusItem = new QTableWidgetItem(statusText);
        if (!s.moderationComment.empty()) {
            statusItem->setToolTip(
                "HoD feedback: " +
                QString::fromStdString(s.moderationComment)
            );
        }
        submissionsTable->setItem(row, 6, statusItem);

        QString markText = s.hasMark
            ? QString::number(s.finalMark, 'f', 2)
            : "Not marked";

        submissionsTable->setItem(
            row, 7, new QTableWidgetItem(markText)
        );
    }
}

void LecturerWindow::downloadSelectedSubmission()
{
    const int row = submissionsTable->currentRow();
    if (row < 0 || !submissionsTable->item(row, 0)) {
        QMessageBox::information(
            this, "Select Submission",
            "Select a submission before downloading its file."
        );
        return;
    }

    const QString sourcePath = submissionsTable->item(row, 0)
        ->data(Qt::UserRole).toString();
    const QFileInfo sourceInfo(sourcePath);
    if (sourcePath.isEmpty() || !sourceInfo.exists() || !sourceInfo.isFile()) {
        QMessageBox::warning(
            this, "File Unavailable",
            "The submitted file could not be found at its saved location."
        );
        return;
    }

    const QString destinationPath = QFileDialog::getSaveFileName(
        this,
        "Download Submission",
        sourceInfo.fileName(),
        "All Files (*)"
    );
    if (destinationPath.isEmpty()) {
        return;
    }

    if (QFileInfo(destinationPath).canonicalFilePath() ==
        sourceInfo.canonicalFilePath()) {
        QMessageBox::information(
            this, "File Already There",
            "The selected destination is the original submitted file."
        );
        return;
    }

    if (QFileInfo::exists(destinationPath)) {
        QMessageBox::warning(
            this, "Destination Exists",
            "A file already exists at that destination. Choose another name."
        );
        return;
    }

    if (!QFile::copy(sourcePath, destinationPath)) {
        QMessageBox::warning(
            this, "Download Failed",
            "The submitted file could not be copied to the selected destination."
        );
        return;
    }

    QMessageBox::information(
        this, "Download Complete",
        "The submitted file was saved to:\n" + destinationPath
    );
}

void LecturerWindow::markSelectedSubmission()
{
    const int row = submissionsTable->currentRow();
    if (row < 0 || !submissionsTable->item(row, 0)) {
        QMessageBox::information(
            this, "Select Submission",
            "Select a submission before entering a mark."
        );
        return;
    }

    const int submissionId = submissionsTable->item(row, 0)->text().toInt();
    SubmissionMarkContext context;
    std::string error;
    if (!database.getSubmissionMarkingContext(
            lecturerId, submissionId, context, error)) {
        QMessageBox::warning(
            this, "Marking Error", QString::fromStdString(error)
        );
        return;
    }

    if (context.status == "RELEASED") {
        QMessageBox::information(
            this, "Marking Closed",
            "This mark has already been released and can no longer be changed."
        );
        return;
    }
    if (!context.isCurrentSubmission) {
        QMessageBox::information(
            this, "Superseded Submission",
            "A newer submission is the current version for marking."
        );
        return;
    }

    const bool isReturned = context.latestDecision == "RETURNED";
    QDialog dialog(this);
    dialog.setWindowTitle(
        isReturned ? "Correct Returned Mark" : "Mark Submission"
    );
    dialog.setMinimumWidth(420);

    auto* form = new QFormLayout(&dialog);
    if (isReturned) {
        auto* returnNotice = new QLabel(
            "The HoD returned this mark for correction:\n" +
                QString::fromStdString(context.moderationComment),
            &dialog
        );
        returnNotice->setWordWrap(true);
        form->addRow("HoD feedback:", returnNotice);
    }

    auto* rawMarkInput = new QDoubleSpinBox(&dialog);
    rawMarkInput->setRange(0.0, context.maxMark);
    rawMarkInput->setDecimals(2);
    rawMarkInput->setSingleStep(1.0);
    rawMarkInput->setValue(
        context.hasExistingMark ? context.currentRawMark : 0.0
    );

    auto* feedbackInput = new QPlainTextEdit(&dialog);
    feedbackInput->setPlaceholderText("Feedback for the student");
    feedbackInput->setMaximumHeight(100);

    auto* reasonInput = new QLineEdit(&dialog);
    reasonInput->setPlaceholderText("Reason for this mark or change");

    form->addRow(
        "Raw mark (maximum " + QString::number(context.maxMark, 'f', 2) + "):",
        rawMarkInput
    );
    form->addRow("Feedback:", feedbackInput);
    form->addRow("Reason:", reasonInput);

    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Save | QDialogButtonBox::Cancel,
        &dialog
    );
    form->addRow(buttons);

    QObject::connect(
        buttons, &QDialogButtonBox::rejected,
        &dialog, &QDialog::reject
    );
    QObject::connect(
        buttons, &QDialogButtonBox::accepted,
        &dialog,
        [&dialog, feedbackInput, reasonInput]()
        {
            if (feedbackInput->toPlainText().trimmed().isEmpty() ||
                reasonInput->text().trimmed().isEmpty()) {
                QMessageBox::warning(
                    &dialog, "Required Details",
                    "Feedback and a reason are required to save a mark."
                );
                return;
            }
            dialog.accept();
        }
    );

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    MarkingService markingService(database);
    const double penalty = markingService.calculateLatePenalty(
        context,
        rawMarkInput->value()
    );
    const double previewFinalMark = std::max(
        0.0, rawMarkInput->value() - penalty
    );

    if (context.isLate) {
        const QString confirmationText =
            QString("This submission was late.\n\n"
                    "Raw mark: %1\n"
                    "Late penalty: %2 points\n"
                    "Final mark after penalty: %3\n\n"
                    "Apply the penalty and save this mark?")
                .arg(QString::number(rawMarkInput->value(), 'f', 2),
                     QString::number(penalty, 'f', 2),
                     QString::number(previewFinalMark, 'f', 2));
        if (QMessageBox::question(
                this,
                "Confirm Late Submission Penalty",
                confirmationText,
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No) != QMessageBox::Yes) {
            return;
        }
    }

    double finalMark = 0.0;
    if (!markingService.markSubmission(
            lecturerId,
            submissionId,
            rawMarkInput->value(),
            feedbackInput->toPlainText().trimmed().toStdString(),
            reasonInput->text().trimmed().toStdString(),
            finalMark,
            error)) {
        QMessageBox::warning(
            this, "Marking Error", QString::fromStdString(error)
        );
        return;
    }

    loadSubmissions();
    if (context.isLate) {
        const QString correctionMessage = isReturned
            ? "\n\nThe corrected mark is saved and ready for HoD review."
            : QString();
        QMessageBox::information(
            this, "Late Penalty Applied",
            QString("The late submission penalty has been applied.%1\n\n"
                    "Penalty: %2 points\n"
                    "Final mark after deduction: %3")
                .arg(correctionMessage,
                     QString::number(penalty, 'f', 2),
                     QString::number(finalMark, 'f', 2))
        );
    } else if (isReturned) {
        QMessageBox::information(
            this, "Correction Saved",
            "The corrected mark has been saved and is ready for HoD review."
        );
    } else {
        QMessageBox::information(
            this, "Mark Saved",
            "The submission was marked. Final mark: " +
                QString::number(finalMark, 'f', 2)
        );
    }
}

void LecturerWindow::selectTab(int index)
{
    if (tabs && index >= 0 && index < tabs->count()) {
        tabs->setCurrentIndex(index);
    }
}

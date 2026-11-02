#include "HodWindow.h"

#include <QAbstractItemView>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTabWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

HodWindow::HodWindow(Database& database, int hodId, QWidget* parent)
    : QWidget(parent),
      database(database),
      hodId(hodId),
      tabs(nullptr),
      coursesTable(nullptr),
      marksTable(nullptr),
      releaseTable(nullptr)
{
    setupUi();
    loadCourses();
    loadMarks();
    loadReleaseQueue();
}

void HodWindow::selectTab(int index)
{
    if (tabs && index >= 0 && index < tabs->count()) {
        tabs->setCurrentIndex(index);
    }
}

void HodWindow::setupUi()
{
    setWindowTitle("Head of Department Workspace");
    resize(1150, 700);

    auto* mainLayout = new QVBoxLayout(this);
    auto* heading = new QLabel("Head of Department Workspace", this);
    heading->setStyleSheet(
        "font-size: 22px; font-weight: bold; color: #1E3A8A;"
    );
    mainLayout->addWidget(heading);

    tabs = new QTabWidget(this);

    // Department Courses
    auto* coursesPage = new QWidget(tabs);
    auto* coursesLayout = new QVBoxLayout(coursesPage);
    coursesTable = new QTableWidget(coursesPage);
    coursesTable->setColumnCount(3);
    coursesTable->setHorizontalHeaderLabels(
        {"Course ID", "Course Code", "Course Name"}
    );
    coursesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    coursesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    coursesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    auto* refreshCourses = new QPushButton("Refresh Courses", coursesPage);
    coursesLayout->addWidget(coursesTable);
    coursesLayout->addWidget(refreshCourses);
    connect(refreshCourses, &QPushButton::clicked, this, [this] {
        loadCourses();
    });
    tabs->addTab(coursesPage, "Department Courses");

    // Review / Moderate Marks
    auto* marksPage = new QWidget(tabs);
    auto* marksLayout = new QVBoxLayout(marksPage);
    auto* marksHint = new QLabel(
        "Select a mark, then approve it or return it to the lecturer. "
        "A comment is required for either decision.", marksPage
    );
    marksHint->setWordWrap(true);
    marksTable = new QTableWidget(marksPage);
    marksTable->setColumnCount(11);
    marksTable->setHorizontalHeaderLabels({
        "Mark ID", "Student", "Course", "Assignment", "Submission ID",
        "Raw Mark", "Final Mark", "Feedback", "Submission Status",
        "Latest Decision", "Moderation Comment"
    });
    marksTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    marksTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    marksTable->setSelectionMode(QAbstractItemView::SingleSelection);
    marksTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    marksTable->horizontalHeader()->setStretchLastSection(true);

    auto* approveButton = new QPushButton("Approve Selected Mark", marksPage);
    auto* returnButton = new QPushButton("Return to Lecturer", marksPage);
    auto* refreshMarks = new QPushButton("Refresh Marks", marksPage);
    auto* markButtons = new QHBoxLayout();
    markButtons->addWidget(approveButton);
    markButtons->addWidget(returnButton);
    markButtons->addWidget(refreshMarks);

    marksLayout->addWidget(marksHint);
    marksLayout->addWidget(marksTable);
    marksLayout->addLayout(markButtons);

    connect(approveButton, &QPushButton::clicked, this, [this] {
        moderateSelectedMark("APPROVED");
    });
    connect(returnButton, &QPushButton::clicked, this, [this] {
        moderateSelectedMark("RETURNED");
    });
    connect(refreshMarks, &QPushButton::clicked, this, [this] {
        loadMarks();
        loadReleaseQueue();
    });
    tabs->addTab(marksPage, "Review / Moderate Marks");

    // Release Approved Marks
    auto* releasePage = new QWidget(tabs);
    auto* releaseLayout = new QVBoxLayout(releasePage);
    auto* releaseHint = new QLabel(
        "Only marks whose latest moderation decision is APPROVED can be released.",
        releasePage
    );
    releaseHint->setWordWrap(true);
    releaseTable = new QTableWidget(releasePage);
    releaseTable->setColumnCount(6);
    releaseTable->setHorizontalHeaderLabels({
        "Mark ID", "Student", "Course", "Assignment", "Final Mark", "Decision"
    });
    releaseTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    releaseTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    releaseTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    releaseTable->horizontalHeader()->setStretchLastSection(true);

    auto* refreshRelease = new QPushButton("Refresh Queue", releasePage);
    auto* releaseButton = new QPushButton("Release All Approved Marks", releasePage);
    auto* releaseButtons = new QHBoxLayout();
    releaseButtons->addWidget(refreshRelease);
    releaseButtons->addWidget(releaseButton);
    releaseLayout->addWidget(releaseHint);
    releaseLayout->addWidget(releaseTable);
    releaseLayout->addLayout(releaseButtons);

    connect(refreshRelease, &QPushButton::clicked, this, [this] {
        loadReleaseQueue();
    });
    connect(releaseButton, &QPushButton::clicked, this, [this] {
        releaseApprovedMarks();
    });
    tabs->addTab(releasePage, "Release Approved Marks");

    mainLayout->addWidget(tabs);
}

void HodWindow::loadCourses()
{
    std::vector<CourseRecord> courses;
    if (!database.getHodCourses(hodId, courses)) {
        QMessageBox::warning(this, "Database Error",
                             "Could not load courses for your department.");
        return;
    }

    coursesTable->setRowCount(0);
    for (const auto& course : courses) {
        const int row = coursesTable->rowCount();
        coursesTable->insertRow(row);
        coursesTable->setItem(row, 0,
            new QTableWidgetItem(QString::number(course.courseId)));
        coursesTable->setItem(row, 1,
            new QTableWidgetItem(QString::fromStdString(course.courseCode)));
        coursesTable->setItem(row, 2,
            new QTableWidgetItem(QString::fromStdString(course.name)));
    }
}

void HodWindow::loadMarks()
{
    std::vector<HodMarkRecord> marks;
    if (!database.getHodMarks(hodId, marks)) {
        QMessageBox::warning(this, "Database Error",
                             "Could not load marks for moderation.");
        return;
    }

    marksTable->setRowCount(0);
    for (const auto& mark : marks) {
        const int row = marksTable->rowCount();
        marksTable->insertRow(row);
        auto set = [this, row](int column, const QString& value) {
            marksTable->setItem(row, column, new QTableWidgetItem(value));
        };

        set(0, QString::number(mark.markId));
        set(1, QString::fromStdString(mark.studentUsername));
        set(2, QString::fromStdString(mark.courseCode));
        set(3, QString::fromStdString(mark.assignmentTitle));
        set(4, QString::number(mark.submissionId));
        set(5, QString::number(mark.rawMark, 'f', 2));
        set(6, QString::number(mark.finalMark, 'f', 2));
        set(7, QString::fromStdString(mark.feedback));
        set(8, QString::fromStdString(mark.submissionStatus));
        set(9, QString::fromStdString(mark.latestDecision.empty()
            ? "NOT REVIEWED" : mark.latestDecision));
        set(10, QString::fromStdString(mark.moderationComment));
        marksTable->item(row, 0)->setData(Qt::UserRole, mark.markId);
    }
}

void HodWindow::loadReleaseQueue()
{
    std::vector<HodMarkRecord> marks;
    if (!database.getHodMarks(hodId, marks)) {
        QMessageBox::warning(this, "Database Error",
                             "Could not load the approved-mark queue.");
        return;
    }

    releaseTable->setRowCount(0);
    for (const auto& mark : marks) {
        if (mark.latestDecision != "APPROVED" ||
            mark.submissionStatus == "RELEASED") {
            continue;
        }

        const int row = releaseTable->rowCount();
        releaseTable->insertRow(row);
        releaseTable->setItem(row, 0,
            new QTableWidgetItem(QString::number(mark.markId)));
        releaseTable->setItem(row, 1,
            new QTableWidgetItem(QString::fromStdString(mark.studentUsername)));
        releaseTable->setItem(row, 2,
            new QTableWidgetItem(QString::fromStdString(mark.courseCode)));
        releaseTable->setItem(row, 3,
            new QTableWidgetItem(QString::fromStdString(mark.assignmentTitle)));
        releaseTable->setItem(row, 4,
            new QTableWidgetItem(QString::number(mark.finalMark, 'f', 2)));
        releaseTable->setItem(row, 5,
            new QTableWidgetItem(mark.latestDecision == "APPROVED"
                ? "APPROVED" : ""));
    }
}

int HodWindow::selectedMarkId() const
{
    const int row = marksTable->currentRow();
    if (row < 0) {
        return -1;
    }

    auto* item = marksTable->item(row, 0);
    return item ? item->data(Qt::UserRole).toInt() : -1;
}

void HodWindow::moderateSelectedMark(const std::string& decision)
{
    const int markId = selectedMarkId();
    if (markId <= 0) {
        QMessageBox::information(this, "Select a Mark",
                                 "Select a mark row before making a decision.");
        return;
    }

    bool ok = false;
    const QString prompt = decision == "APPROVED"
        ? "Enter a moderation comment explaining the approval:"
        : "Explain why this mark is being returned to the lecturer:";
    const QString comment = QInputDialog::getMultiLineText(
        this, decision == "APPROVED" ? "Approve Mark" : "Return Mark",
        prompt, QString(), &ok
    );

    if (!ok) {
        return;
    }
    if (comment.trimmed().isEmpty()) {
        QMessageBox::warning(this, "Comment Required",
                             "A non-empty moderation comment is required.");
        return;
    }

    if (!database.moderateMark(hodId, markId, decision,
                               comment.trimmed().toStdString())) {
        QMessageBox::warning(
            this, "Moderation Failed",
            "The mark could not be moderated. It may already be released, "
            "or it may not belong to your department."
        );
        return;
    }

    QMessageBox::information(this, "Moderation Saved",
                             decision == "APPROVED"
                                 ? "Mark approved. It can now be released."
                                 : "Mark returned to the lecturer.");
    loadMarks();
    loadReleaseQueue();
}

void HodWindow::releaseApprovedMarks()
{
    const auto answer = QMessageBox::question(
        this, "Confirm Release",
        "Release every currently approved, unreleased mark in your department?"
    );
    if (answer != QMessageBox::Yes) {
        return;
    }

    int releasedCount = 0;
    if (!database.releaseApprovedMarks(hodId, releasedCount)) {
        QMessageBox::warning(this, "Release Failed",
                             "Approved marks could not be released.");
        return;
    }

    QMessageBox::information(
        this, "Release Complete",
        QString("Released %1 approved mark(s).").arg(releasedCount)
    );
    loadMarks();
    loadReleaseQueue();
}

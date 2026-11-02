#include "Database.h"
#include "MarkingService.h"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

namespace {

bool testAuditTrailAccessAndFiltering() {
    Database db;
    const std::string path = "test_lms_audit.db";
    std::remove(path.c_str());
    if (!db.open(path) || !db.createTables()) {
        std::cerr << "Failed to initialize audit test database\n";
        return false;
    }

    std::string error;
    if (!db.createUser("audit_admin", "password", "ADMIN", error) ||
        !db.createUser("audit_lecturer", "password", "LECTURER", error) ||
        !db.createUser("audit_student", "password", "STUDENT", error) ||
        !db.createDepartment("Audit Department") ||
        !db.createCourse("AUDIT101", "Audit Course", 1) ||
        !db.assignLecturerToCourse(1, 2, error) ||
        !db.enrolStudent(3, 1, error) ||
        !db.createAssignmentForLecturer(
            2, 1, "Audit Assignment", "Test audit entries",
            "2099-12-31 23:59:59", 100.0) ||
        !db.submitAssignment(3, 1, "audit-submission.txt")) {
        std::cerr << "Could not create audit test workflow: "
                  << error << "\n";
        return false;
    }

    std::vector<SubmissionRecord> submissions;
    if (!db.getStudentSubmissions(3, submissions) ||
        submissions.size() != 1) {
        std::cerr << "Could not load audit test submission\n";
        return false;
    }

    if (!db.markSubmission(
            2, submissions[0].submissionId, 70.0, 70.0,
            "Initial audit mark", "First mark", error) ||
        !db.markSubmission(
            2, submissions[0].submissionId, 75.0, 75.0,
            "Corrected audit mark", "Correction reason", error)) {
        std::cerr << "Could not create mark audit entries: "
                  << error << "\n";
        return false;
    }

    AuditTrailFilters filters;
    filters.action = "MARK_CHANGED";
    filters.entityType = "MARK";
    filters.actor = "audit_lecturer";
    filters.searchText = "Correction reason";
    std::vector<AuditTrailRecord> records;
    if (!db.getAdminAuditTrail(1, filters, records, error) ||
        records.size() != 1 ||
        records[0].oldValue != "raw=70.000000;final=70.000000" ||
        records[0].newValue != "raw=75.000000;final=75.000000" ||
        records[0].actorRole != "LECTURER" ||
        records[0].occurredAt.empty()) {
        std::cerr << "Mark audit filtering or values were incorrect: "
                  << error << "\n";
        return false;
    }

    filters = AuditTrailFilters{};
    filters.action = "MARK_CREATED";
    if (!db.getAdminAuditTrail(1, filters, records, error) ||
        records.size() != 1 ||
        records[0].oldValue != "none" ||
        records[0].newValue != "raw=70.000000;final=70.000000") {
        std::cerr << "Initial mark was not audited: " << error << "\n";
        return false;
    }

    filters = AuditTrailFilters{};
    filters.action = "ASSIGNMENT_CREATED";
    filters.entityType = "ASSIGNMENT";
    if (!db.getAdminAuditTrail(1, filters, records, error) ||
        records.size() != 1 ||
        records[0].actorUsername != "audit_lecturer") {
        std::cerr << "Assignment audit filtering failed: " << error << "\n";
        return false;
    }

    if (db.getAdminAuditTrail(3, filters, records, error)) {
        std::cerr << "Non-admin user was allowed to read the audit trail\n";
        return false;
    }

    db.close();
    std::remove(path.c_str());
    std::cout << "Audit trail test passed\n";
    return true;
}

bool testAssessmentWorkflow() {
    Database db;
    const std::string path = "test_lms_workflow.db";
    std::remove(path.c_str());

    if (!db.open(path)) {
        std::cerr << "Failed to open database\n";
        return false;
    }
    if (!db.createTables()) {
        std::cerr << "Failed to create tables\n";
        return false;
    }

    std::string error;
    if (!db.createUser("admin", "adminpass", "ADMIN", error)) {
        std::cerr << "admin create failed: " << error << "\n";
        return false;
    }
    if (!db.createUser("lecturer1", "lecturerpass", "LECTURER", error)) {
        std::cerr << "lecturer create failed: " << error << "\n";
        return false;
    }
    if (!db.createUser("hod1", "hodpass", "HOD", error)) {
        std::cerr << "hod create failed: " << error << "\n";
        return false;
    }
    if (!db.createUser("student1", "studentpass", "STUDENT", error)) {
        std::cerr << "student create failed: " << error << "\n";
        return false;
    }

    if (!db.createDepartment("Computer Science")) {
        std::cerr << "department create failed\n";
        return false;
    }

    std::vector<DepartmentRecord> departments;
    if (!db.getDepartments(departments)) {
        std::cerr << "department query failed\n";
        return false;
    }
    if (departments.empty()) {
        std::cerr << "no departments available\n";
        return false;
    }

    if (!db.createCourse("CS101", "Intro to Computing", departments[0].departmentId)) {
        std::cerr << "course create failed\n";
        return false;
    }

    std::vector<CourseRecord> courses;
    if (!db.getCourses(courses)) {
        std::cerr << "course query failed\n";
        return false;
    }
    if (courses.empty()) {
        std::cerr << "no courses available\n";
        return false;
    }

    if (!db.assignLecturerToCourse(courses[0].courseId, 2, error)) {
        std::cerr << "assign lecturer failed: " << error << "\n";
        return false;
    }
    if (!db.assignHodToDepartment(departments[0].departmentId, 3, error)) {
        std::cerr << "assign hod failed: " << error << "\n";
        return false;
    }
    if (!db.enrolStudent(4, courses[0].courseId, error)) {
        std::cerr << "enrol student failed: " << error << "\n";
        return false;
    }

    std::vector<AssignmentRecord> lecturerAssignments;
    if (!db.getLecturerAssignments(2, lecturerAssignments)) {
        std::cerr << "lecturer assignment query failed\n";
        return false;
    }
    if (!db.createAssignmentForLecturer(2, courses[0].courseId, "Assignment 1", "Write a report", "2000-12-31 23:59:59", 100.0)) {
        std::cerr << "assignment creation failed\n";
        return false;
    }

    std::vector<AssignmentRecord> assignments;
    if (!db.getLecturerAssignments(2, assignments) || assignments.empty()) {
        std::cerr << "assignment retrieval failed\n";
        return false;
    }

    std::vector<UserRecord> eligibleStudents;
    if (!db.getAssignmentStudents(
            2, assignments[0].assignmentId, eligibleStudents, error) ||
        eligibleStudents.size() != 1 ||
        eligibleStudents[0].userId != 4) {
        std::cerr << "eligible assignment student lookup failed: "
                  << error << "\n";
        return false;
    }

    const std::string revisedDeadline = "2100-01-15 00:00:00";
    if (!db.grantAssignmentExtension(
            2, assignments[0].assignmentId, 4, revisedDeadline,
            "Approved individual extension", error)) {
        std::cerr << "granting individual extension failed: "
                  << error << "\n";
        return false;
    }
    if (db.grantAssignmentExtension(
            2, assignments[0].assignmentId, 4, "2100-01-01 00:00:00",
            "Attempt to shorten extension", error)) {
        std::cerr << "an existing extension was incorrectly shortened\n";
        return false;
    }

    std::vector<StudentAssignmentRecord> studentAssignments;
    if (!db.getStudentAssignments(4, studentAssignments) ||
        studentAssignments.size() != 1 ||
        studentAssignments[0].maxAttempts != 2 ||
        studentAssignments[0].latestVersion != 0 ||
        studentAssignments[0].assignmentId != assignments[0].assignmentId ||
        studentAssignments[0].courseId != courses[0].courseId ||
        studentAssignments[0].title != "Assignment 1" ||
        studentAssignments[0].description != "Write a report" ||
        studentAssignments[0].deadline != revisedDeadline) {
        std::cerr << "student assignment details or deadline retrieval failed\n";
        return false;
    }

    if (!db.submitAssignment(4, assignments[0].assignmentId, "/tmp/demo.txt")) {
        std::cerr << "student submission failed\n";
        return false;
    }

    std::vector<SubmissionRecord> studentSubmissions;
    if (!db.getStudentSubmissions(4, studentSubmissions) ||
        studentSubmissions.size() != 1) {
        std::cerr << "initial submission lookup failed\n";
        return false;
    }
    const int firstSubmissionId =
        studentSubmissions.empty() ? 0 : studentSubmissions[0].submissionId;
    if (!db.submitAssignment(
            4, assignments[0].assignmentId, "/tmp/demo-revision.txt")) {
        std::cerr << "permitted resubmission failed\n";
        return false;
    }

    if (db.submitAssignment(
            4, assignments[0].assignmentId, "/tmp/demo-third-attempt.txt")) {
        std::cerr << "submission exceeded the configured attempt limit\n";
        return false;
    }

    if (!db.getStudentSubmissions(4, studentSubmissions) ||
        studentSubmissions.size() != 2) {
        std::cerr << "resubmissions were not retrieved\n";
        return false;
    }
    int currentSubmissionId = 0;
    for (const auto& submission : studentSubmissions) {
        if (submission.versionNumber == 2) {
            currentSubmissionId = submission.submissionId;
        }
        if (submission.versionNumber == 1 &&
            submission.isCurrentSubmission) {
            std::cerr << "old submission remained current after resubmission\n";
            return false;
        }
    }
    if (currentSubmissionId <= 0) {
        std::cerr << "newest resubmission was not identified as current\n";
        return false;
    }

    SubmissionMarkContext oldSubmissionContext;
    if (!db.getSubmissionMarkingContext(
            2, firstSubmissionId, oldSubmissionContext, error) ||
        oldSubmissionContext.isCurrentSubmission) {
        std::cerr << "superseded submission was not identified\n";
        return false;
    }

    std::vector<SubmissionRecord> submissions;
    if (!db.getLecturerSubmissions(2, submissions) || submissions.empty()) {
        std::cerr << "lecturer submissions failed\n";
        return false;
    }
    SubmissionMarkContext extensionContext;
    if (!db.getSubmissionMarkingContext(
            2, currentSubmissionId, extensionContext, error) ||
        extensionContext.isLate || !extensionContext.isCurrentSubmission) {
        std::cerr << "extended deadline was not used for lateness validation\n";
        return false;
    }
    if (!db.getStudentSubmissions(4, studentSubmissions)) {
        std::cerr << "student submission retrieval failed\n";
        return false;
    }
    bool currentSubmissionPersisted = false;
    for (const auto& submission : studentSubmissions) {
        if (submission.submissionId == currentSubmissionId &&
            submission.submittedAt.size() > 0 &&
            submission.status == "SUBMITTED" &&
            submission.fileReference == "/tmp/demo-revision.txt" &&
            submission.isCurrentSubmission) {
            currentSubmissionPersisted = true;
        }
    }
    if (!currentSubmissionPersisted) {
        std::cerr << "current submission was not persisted correctly\n";
        return false;
    }
    if (db.grantAssignmentExtension(
            2, assignments[0].assignmentId, 4, "2101-01-15 00:00:00",
            "Attempt after submission", error)) {
        std::cerr << "extension was incorrectly granted after submission\n";
        return false;
    }

    MarkingService markingService(db);
    double finalMark = 0.0;
    std::string markError;
    if (markingService.markSubmission(
            2, firstSubmissionId, 80.0, "Old version mark",
            "Must not mark superseded version", finalMark, markError)) {
        std::cerr << "superseded submission was allowed to be marked\n";
        return false;
    }
    if (!markingService.markSubmission(2, currentSubmissionId, 80.0, "Good work", "Initial marking", finalMark, markError)) {
        std::cerr << "mark submission failed: " << markError << "\n";
        return false;
    }

    if (finalMark < 0.0 || finalMark > 100.0) {
        std::cerr << "final mark out of range\n";
        return false;
    }

    std::vector<HodMarkRecord> hodMarks;
    if (!db.getHodMarks(3, hodMarks) || hodMarks.empty()) {
        std::cerr << "hod marks query failed\n";
        return false;
    }

    if (!db.moderateMark(
            3, hodMarks[0].markId, "RETURNED",
            "Please correct the calculation in section two.")) {
        std::cerr << "moderation return failed\n";
        return false;
    }

    std::vector<SubmissionRecord> returnedSubmissions;
    if (!db.getLecturerSubmissions(2, returnedSubmissions) ||
        returnedSubmissions.empty() ||
        returnedSubmissions[0].latestDecision != "RETURNED" ||
        returnedSubmissions[0].moderationComment !=
            "Please correct the calculation in section two.") {
        std::cerr << "returned mark details were not shown to lecturer\n";
        return false;
    }

    SubmissionMarkContext returnedContext;
    if (!db.getSubmissionMarkingContext(
            2, currentSubmissionId, returnedContext, markError) ||
        returnedContext.latestDecision != "RETURNED" ||
        returnedContext.moderationComment !=
            "Please correct the calculation in section two.") {
        std::cerr << "returned mark context could not be loaded\n";
        return false;
    }

    double correctedFinalMark = 0.0;
    if (!markingService.markSubmission(
            2, currentSubmissionId, 86.0,
            "Calculation corrected", "Corrected after HoD return",
            correctedFinalMark, markError) ||
        correctedFinalMark != 86.0) {
        std::cerr << "lecturer could not correct the returned mark: "
                  << markError << "\n";
        return false;
    }

    if (!db.getLecturerSubmissions(2, returnedSubmissions) ||
        returnedSubmissions.empty() ||
        returnedSubmissions[0].status != "MARKED" ||
        returnedSubmissions[0].latestDecision != "RETURNED" ||
        returnedSubmissions[0].rawMark != 86.0) {
        std::cerr << "corrected mark was not visible for HoD re-review\n";
        return false;
    }

    if (!db.getHodMarks(3, hodMarks) || hodMarks.empty() ||
        !db.moderateMark(
            3, hodMarks[0].markId, "APPROVED",
            "Correction checked and approved.")) {
        std::cerr << "moderation approval after correction failed\n";
        return false;
    }

    int releasedCount = 0;
    if (!db.releaseApprovedMarks(3, releasedCount) || releasedCount != 1) {
        std::cerr << "release approved marks failed\n";
        return false;
    }

    std::vector<StudentMarkRecord> studentMarks;
    if (!db.getStudentMarks(4, studentMarks) || studentMarks.empty()) {
        std::cerr << "student mark release query failed\n";
        return false;
    }

    if (!db.createAssignmentForLecturer(
            2, courses[0].courseId, "Late Assignment", "Past deadline",
            "2000-01-01 00:00:00", 100.0)) {
        std::cerr << "late assignment creation failed\n";
        return false;
    }

    std::vector<AssignmentRecord> allAssignments;
    if (!db.getLecturerAssignments(2, allAssignments)) {
        std::cerr << "late assignment lookup failed\n";
        return false;
    }
    int lateAssignmentId = 0;
    for (const auto& assignment : allAssignments) {
        if (assignment.title == "Late Assignment") {
            lateAssignmentId = assignment.assignmentId;
            break;
        }
    }
    if (lateAssignmentId == 0 ||
        !db.submitAssignment(4, lateAssignmentId, "/tmp/late.txt")) {
        std::cerr << "late submission failed\n";
        return false;
    }

   
if (!db.getStudentSubmissions(4, studentSubmissions) ||
    studentSubmissions.size() != 3) {

    std::cerr << "late submission retrieval failed\n";
    return false;
}

bool foundLateSubmission = false;
bool foundCurrentVersion = false;
bool foundOldVersion = false;

for (const auto& submission : studentSubmissions) {

    if (submission.assignmentId == 2 &&
        submission.status == "LATE" &&
        submission.isCurrentSubmission) {

        foundLateSubmission = true;
    }

    if (submission.assignmentId == 1 &&
        submission.versionNumber == 2 &&
        submission.isCurrentSubmission) {

        foundCurrentVersion = true;
    }

    if (submission.assignmentId == 1 &&
        submission.versionNumber == 1 &&
        !submission.isCurrentSubmission) {

        foundOldVersion = true;
    }
}

if (!foundLateSubmission ||
    !foundCurrentVersion ||
    !foundOldVersion) {

    std::cerr << "Submission retrieval validation failed\n";
    return false;
}
 bool lateStatusRecorded = false;
    int lateSubmissionId = 0;
    for (const auto& submission : studentSubmissions) {
        if (submission.assignmentTitle == "Late Assignment" &&
            submission.status == "LATE" &&
            !submission.submittedAt.empty()) {
            lateStatusRecorded = true;
            lateSubmissionId = submission.submissionId;
        }
    }
    if (!lateStatusRecorded) {
        std::cerr << "late status or submission timestamp was not recorded\n";
        return false;
    }

    SubmissionMarkContext lateContext;
    if (!db.getSubmissionMarkingContext(
            2, lateSubmissionId, lateContext, markError) ||
        !lateContext.isLate) {
        std::cerr << "late submission marking context failed: "
                  << markError << "\n";
        return false;
    }

    const double latePenalty =
        markingService.calculateLatePenalty(lateContext, 80.0);
    if (latePenalty <= 0.0) {
        std::cerr << "late penalty was not calculated\n";
        return false;
    }

    double lateFinalMark = 0.0;
    if (!markingService.markSubmission(
            2, lateSubmissionId, 80.0, "Late work marked",
            "Confirmed late penalty", lateFinalMark, markError)) {
        std::cerr << "late submission marking failed: " << markError << "\n";
        return false;
    }

    if (lateFinalMark != std::max(0.0, 80.0 - latePenalty)) {
        std::cerr << "late penalty was not applied to the final mark\n";
        return false;
    }

    std::vector<SubmissionRecord> lecturerSubmissions;
    if (!db.getLecturerSubmissions(2, lecturerSubmissions)) {
        std::cerr << "lecturer submission reload failed\n";
        return false;
    }
    bool persistedLateMark = false;
    for (const auto& submission : lecturerSubmissions) {
        if (submission.submissionId == lateSubmissionId &&
            submission.hasMark &&
            submission.rawMark == 80.0 &&
            submission.finalMark == lateFinalMark) {
            persistedLateMark = true;
        }
    }
    if (!persistedLateMark) {
        std::cerr << "late penalty mark was not persisted\n";
        return false;
    }

    AuditTrailFilters filters;
    filters.action = "MARK_CHANGED";
    filters.entityType = "MARK";
    filters.actor = "lecturer1";
    filters.searchText = "Corrected after HoD return";
    std::vector<AuditTrailRecord> auditRecords;
    if (!db.getAdminAuditTrail(1, filters, auditRecords, error) ||
        auditRecords.size() != 1 ||
        auditRecords[0].actorUsername != "lecturer1" ||
        auditRecords[0].actorRole != "LECTURER" ||
        auditRecords[0].reason != "Corrected after HoD return" ||
        auditRecords[0].oldValue != "raw=80.000000;final=80.000000" ||
        auditRecords[0].newValue != "raw=86.000000;final=86.000000" ||
        auditRecords[0].occurredAt.empty()) {
        std::cerr << "filtered mark audit record was incorrect: "
                  << error << "\n";
        return false;
    }

    filters = AuditTrailFilters{};
    if (!db.getAdminAuditTrail(1, filters, auditRecords, error) ||
        auditRecords.size() < 7) {
        std::cerr << "system audit events were not recorded: "
                  << error << "\n";
        return false;
    }

    if (db.getAdminAuditTrail(4, filters, auditRecords, error)) {
        std::cerr << "non-admin user was allowed to read the audit trail\n";
        return false;
    }

    db.close();
    std::remove(path.c_str());
    std::cout << "Assessment workflow test passed\n";
    return true;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc > 1 && std::strcmp(argv[1], "--audit-only") == 0) {
        return testAuditTrailAccessAndFiltering() ? 0 : 1;
    }
    return testAssessmentWorkflow() ? 0 : 1;
}

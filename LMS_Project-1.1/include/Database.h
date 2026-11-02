#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>
#include <string>
#include <vector>

// Represents a department stored in the database.
struct DepartmentRecord {
    int departmentId;
    std::string name;
};

// Represents a course and its associated department.
struct CourseRecord {
    int courseId;
    std::string courseCode;
    std::string name;
    int departmentId;
    std::string departmentName;
};

struct AssignmentRecord {
    int assignmentId;
    int courseId;
    std::string courseCode;
    std::string courseName;
    std::string title;
    std::string description;
    std::string deadline;
    double maxMark;
    int maxAttempts;
};

struct HodMarkRecord {
    int markId;
    int submissionId;
    std::string studentUsername;
    std::string courseCode;
    std::string assignmentTitle;
    double rawMark;
    double finalMark;
    std::string feedback;
    std::string submissionStatus;
    std::string latestDecision;
    std::string moderationComment;
};

struct SubmissionRecord {
    int submissionId;
    int assignmentId;
    int studentId;
    int versionNumber;
    std::string studentUsername;
    std::string courseCode;
    std::string assignmentTitle;
    std::string submittedAt;
    std::string status;
    std::string fileReference;
    bool hasMark;
    double rawMark;
    double finalMark;
    std::string feedback;
    std::string latestDecision;
    std::string moderationComment;
    bool isCurrentSubmission = true;
};

struct StudentAssignmentRecord {
    int assignmentId;
    int courseId;
    std::string courseCode;
    std::string courseName;
    std::string title;
    std::string description;
    std::string deadline;
    std::string deadlineExtension;
    double maxMark;
    int maxAttempts;
    int latestVersion;
    std::string latestStatus;
};

struct StudentMarkRecord {
    std::string courseCode;
    std::string assignmentTitle;
    int versionNumber;
    double rawMark;
    double finalMark;
    double maxMark;
    std::string feedback;
    std::string markedAt;
};

struct SubmissionMarkContext {
    int submissionId = 0;
    int assignmentId = 0;
    int courseId = 0;
    int lecturerId = 0;
    double maxMark = 0.0;
    std::string deadline;
    std::string submittedAt;
    std::string status;
    bool isLate = false;
    bool hasExistingMark = false;
    double currentRawMark = 0.0;
    double currentFinalMark = 0.0;
    std::string latestDecision;
    std::string moderationComment;
    bool isCurrentSubmission = true;
};

struct UserRecord {
    int userId;
    std::string username;
    std::string role;
    bool active;
};

struct EnrolmentRecord {
    int studentId;
    std::string studentUsername;
    int courseId;
    std::string courseCode;
    std::string courseName;
    std::string enrolledAt;
};

struct CourseLecturerRecord {
    int courseId;
    std::string courseCode;
    std::string courseName;
    int lecturerId;
    std::string lecturerUsername;
};

struct AuditTrailFilters {
    std::string action;
    std::string entityType;
    std::string actor;
    std::string startDate;
    std::string endDate;
    std::string searchText;
};

struct AuditTrailRecord {
    std::string occurredAt;
    int actorId = 0;
    std::string actorUsername;
    std::string actorRole;
    std::string action;
    std::string entityType;
    int entityId = 0;
    std::string oldValue;
    std::string newValue;
    std::string reason;
    std::string details;
};

class Database {
private:
    sqlite3* db;
    bool writeAuditEvent(
        int actorId,
        const std::string& action,
        const std::string& entityType,
        int entityId,
        const std::string& oldValue,
        const std::string& newValue,
        const std::string& reason,
        const std::string& details,
        std::string& error
    );

public:
    // Constructor and destructor
    Database();
    ~Database();

    // Database connection and general SQL execution
    bool open(const std::string& filename);
    bool execute(const std::string& sql);
    bool createTables();
    void close();

    // User management and authentication
    bool createUser(
        const std::string& username,
        const std::string& password,
        const std::string& role
    );

    bool createUser(
        const std::string& username,
        const std::string& password,
        const std::string& role,
        std::string& error
    );

    bool login(
        const std::string& username,
        const std::string& password,
        int& userId,
        std::string& role
    );

    // Department management
    bool createDepartment(
        const std::string& name
    );

    bool getDepartments(
        std::vector<DepartmentRecord>& departments
    );

    // Course management
    bool createCourse(
        const std::string& courseCode,
        const std::string& name,
        int departmentId
    );

    bool getCourses(
        std::vector<CourseRecord>& courses
    );

    // Lecturer course access
    bool getLecturerCourses(
        int lecturerId,
        std::vector<CourseRecord>& courses
    );

    // Assignment management
    bool createAssignmentForLecturer(
        int lecturerId,
        int courseId,
        const std::string& title,
        const std::string& description,
        const std::string& deadline,
        double maxMark,
        int maxAttempts = 2
    );

    bool getLecturerAssignments(
        int lecturerId,
        std::vector<AssignmentRecord>& assignments
    );

    bool getAssignmentStudents(
        int lecturerId,
        int assignmentId,
        std::vector<UserRecord>& students,
        std::string& error
    );

    bool grantAssignmentExtension(
        int lecturerId,
        int assignmentId,
        int studentId,
        const std::string& revisedDeadline,
        const std::string& reason,
        std::string& error
    );

    bool grantClassAssignmentExtension(
        int lecturerId,
        int assignmentId,
        const std::string& revisedDeadline,
        const std::string& reason,
        std::string& error
    );

    // Submission viewing
    bool getLecturerSubmissions(
        int lecturerId,
        std::vector<SubmissionRecord>& submissions
    ); 
    
    bool getHodCourses(int hodId, std::vector<CourseRecord>& courses);
    bool getHodMarks(int hodId, std::vector<HodMarkRecord>& marks);
    bool moderateMark(int hodId, int markId,
                    const std::string& decision,
                    const std::string& comment);
    bool releaseApprovedMarks(int hodId, int& releasedCount);
    
        // Student course access
    bool getStudentCourses(
        int studentId,
        std::vector<CourseRecord>& courses
    );

    // Student assignments and submissions
    bool getStudentAssignments(
        int studentId,
        std::vector<StudentAssignmentRecord>& assignments
    );

    bool getStudentSubmissions(
        int studentId,
        std::vector<SubmissionRecord>& submissions
    );

    bool submitAssignment(
        int studentId,
        int assignmentId,
        const std::string& fileReference
    );

    // Only marks released by the HoD may be displayed
    bool getStudentMarks(
        int studentId,
        std::vector<StudentMarkRecord>& marks
    );

    // Marking and audit trail
    bool getSubmissionMarkingContext(
        int lecturerId,
        int submissionId,
        SubmissionMarkContext& context,
        std::string& error
    );

    bool markSubmission(
        int lecturerId,
        int submissionId,
        double rawMark,
        double finalMark,
        const std::string& feedback,
        const std::string& reason,
        std::string& error
    );

    bool getAdminAuditTrail(
        int adminId,
        const AuditTrailFilters& filters,
        std::vector<AuditTrailRecord>& records,
        std::string& error
    );

    // User management
    bool getUsers(
        std::vector<UserRecord>& users,
        std::string& error
    );

    bool setUserActive(
        int userId,
        bool active,
        std::string& error
    );

    // Update username and role, optionally resetting the password.
    bool updateUser(
        int userId,
        const std::string& username,
        const std::string& role,
        const std::string& newPassword,
        std::string& error
    );

    // Student enrolment management
    bool getEnrolments(
        std::vector<EnrolmentRecord>& enrolments,
        std::string& error
    );

    bool enrolStudent(
        int studentId,
        int courseId,
        std::string& error
    );

    bool removeEnrolment(
        int studentId,
        int courseId,
        std::string& error
    );

    // Lecturer-to-course assignments
    bool getCourseLecturers(
        std::vector<CourseLecturerRecord>& assignments,
        std::string& error
    );

    bool assignLecturerToCourse(
        int courseId,
        int lecturerId,
        std::string& error
    );

    bool removeLecturerFromCourse(
        int courseId,
        int lecturerId,
        std::string& error
    );

    // Head of Department assignment
    bool assignHodToDepartment(
        int departmentId,
        int hodUserId,
        std::string& error
    );
};

#endif
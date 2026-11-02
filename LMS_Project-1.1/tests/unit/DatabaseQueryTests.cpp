#include <gtest/gtest.h>

#include "Database.h"

#include <string>
#include <vector>

class DatabaseQueryTests : public ::testing::Test
{
protected:
    Database database;
    std::string error;

    int studentId = 0;
    int lecturerId = 0;
    int hodId = 0;
    int adminId = 0;

    int departmentId = 0;
    int courseId = 0;

    void SetUp() override
    {
        ASSERT_TRUE(database.open(":memory:"));
        ASSERT_TRUE(database.createTables());

        createTestUsers();
        createTestDepartmentAndCourse();
    }

    void TearDown() override
    {
        database.close();
    }

    void createTestUsers()
    {
        error.clear();

        ASSERT_TRUE(database.createUser(
            "student1", "StudentPass123", "STUDENT", error
        ));

        ASSERT_TRUE(database.createUser(
            "lecturer1", "LecturerPass123", "LECTURER", error
        ));

        ASSERT_TRUE(database.createUser(
            "hod1", "HodPass123", "HOD", error
        ));

        ASSERT_TRUE(database.createUser(
            "admin1", "AdminPass123", "ADMIN", error
        ));

        std::vector<UserRecord> users;

        ASSERT_TRUE(database.getUsers(users, error));

        for (const auto& user : users)
        {
            if (user.username == "student1")
                studentId = user.userId;

            else if (user.username == "lecturer1")
                lecturerId = user.userId;

            else if (user.username == "hod1")
                hodId = user.userId;

            else if (user.username == "admin1")
                adminId = user.userId;
        }

        ASSERT_GT(studentId, 0);
        ASSERT_GT(lecturerId, 0);
        ASSERT_GT(hodId, 0);
        ASSERT_GT(adminId, 0);
    }

    void createTestDepartmentAndCourse()
    {
        ASSERT_TRUE(database.createDepartment("Computer Science"));

        std::vector<DepartmentRecord> departments;

        ASSERT_TRUE(database.getDepartments(departments));

        ASSERT_EQ(departments.size(), 1);

        departmentId = departments[0].departmentId;

        ASSERT_TRUE(database.createCourse(
            "CS101", "Programming Fundamentals", departmentId
        ));

        std::vector<CourseRecord> courses;

        ASSERT_TRUE(database.getCourses(courses));

        ASSERT_EQ(courses.size(), 1);

        courseId = courses[0].courseId;
    }

    void assignTestRelationships()
    {
        error.clear();

        ASSERT_TRUE(database.assignHodToDepartment(
            departmentId, hodId, error
        ));

        ASSERT_TRUE(database.assignLecturerToCourse(
            courseId, lecturerId, error
        ));

        ASSERT_TRUE(database.enrolStudent(
            studentId, courseId, error
        ));
    }
};

// =========================================================
// USER RETRIEVAL TESTS
// =========================================================

TEST_F(DatabaseQueryTests, RetrievesAllUsers)
{
    std::vector<UserRecord> users;

    EXPECT_TRUE(database.getUsers(users, error));

    EXPECT_TRUE(error.empty());
    EXPECT_EQ(users.size(), 4);
}

TEST_F(DatabaseQueryTests, RetrievesCorrectUserInformation)
{
    std::vector<UserRecord> users;

    ASSERT_TRUE(database.getUsers(users, error));

    ASSERT_EQ(users.size(), 4);

    bool foundStudent = false;

    for (const auto& user : users)
    {
        if (user.username == "student1")
        {
            foundStudent = true;

            EXPECT_EQ(user.userId, studentId);
            EXPECT_EQ(user.role, "STUDENT");
            EXPECT_TRUE(user.active);
        }
    }

    EXPECT_TRUE(foundStudent);
}

TEST_F(DatabaseQueryTests, HandlesUserRetrievalDatabaseFailure)
{
    database.close();

    std::vector<UserRecord> users;

    EXPECT_FALSE(database.getUsers(users, error));

    EXPECT_FALSE(error.empty());
}

// =========================================================
// UPDATE USER TESTS
// =========================================================

TEST_F(DatabaseQueryTests, UpdatesUserDetailsSuccessfully)
{
    EXPECT_TRUE(database.updateUser(
        studentId,
        "updatedStudent",
        "STUDENT",
        "NewStudentPass123",
        error
    ));

    EXPECT_TRUE(error.empty());

    std::vector<UserRecord> users;

    ASSERT_TRUE(database.getUsers(users, error));

    bool foundUpdatedUser = false;

    for (const auto& user : users)
    {
        if (user.username == "updatedStudent")
        {
            foundUpdatedUser = true;

            EXPECT_EQ(user.userId, studentId);
            EXPECT_EQ(user.role, "STUDENT");
        }
    }

    EXPECT_TRUE(foundUpdatedUser);
}

TEST_F(DatabaseQueryTests, RejectsUpdatingNonexistentUser)
{
    EXPECT_FALSE(database.updateUser(
        99999,
        "unknownUser",
        "STUDENT",
        "NewPassword123",
        error
    ));

    EXPECT_FALSE(error.empty());
}

TEST_F(DatabaseQueryTests, HandlesUserUpdateDatabaseFailure)
{
    database.close();

    EXPECT_FALSE(database.updateUser(
        studentId,
        "updatedStudent",
        "STUDENT",
        "NewPassword123",
        error
    ));

    EXPECT_FALSE(error.empty());
}

// =========================================================
// USER ACTIVATION TESTS
// =========================================================

TEST_F(DatabaseQueryTests, DeactivatesUserSuccessfully)
{
    EXPECT_TRUE(database.setUserActive(
        studentId, false, error
    ));

    EXPECT_TRUE(error.empty());

    std::vector<UserRecord> users;

    ASSERT_TRUE(database.getUsers(users, error));

    for (const auto& user : users)
    {
        if (user.userId == studentId)
            EXPECT_FALSE(user.active);
    }
}

TEST_F(DatabaseQueryTests, ReactivatesUserSuccessfully)
{
    ASSERT_TRUE(database.setUserActive(
        studentId, false, error
    ));

    EXPECT_TRUE(database.setUserActive(
        studentId, true, error
    ));

    std::vector<UserRecord> users;

    ASSERT_TRUE(database.getUsers(users, error));

    for (const auto& user : users)
    {
        if (user.userId == studentId)
            EXPECT_TRUE(user.active);
    }
}

TEST_F(DatabaseQueryTests, RejectsActivatingNonexistentUser)
{
    EXPECT_FALSE(database.setUserActive(
        99999, true, error
    ));

    EXPECT_FALSE(error.empty());
}

TEST_F(DatabaseQueryTests, HandlesUserActivationDatabaseFailure)
{
    database.close();

    EXPECT_FALSE(database.setUserActive(
        studentId, false, error
    ));

    EXPECT_FALSE(error.empty());
}

// =========================================================
// ENROLMENT TESTS
// =========================================================

TEST_F(DatabaseQueryTests, RetrievesStudentEnrolments)
{
    assignTestRelationships();

    std::vector<EnrolmentRecord> enrolments;

    EXPECT_TRUE(database.getEnrolments(enrolments, error));

    EXPECT_TRUE(error.empty());
    EXPECT_EQ(enrolments.size(), 1);
}

TEST_F(DatabaseQueryTests, RetrievesCorrectEnrolmentInformation)
{
    assignTestRelationships();

    std::vector<EnrolmentRecord> enrolments;

    ASSERT_TRUE(database.getEnrolments(enrolments, error));

    ASSERT_EQ(enrolments.size(), 1);

    EXPECT_EQ(enrolments[0].studentId, studentId);
    EXPECT_EQ(enrolments[0].studentUsername, "student1");
    EXPECT_EQ(enrolments[0].courseId, courseId);
    EXPECT_EQ(enrolments[0].courseCode, "CS101");
    EXPECT_EQ(
        enrolments[0].courseName,
        "Programming Fundamentals"
    );
}

TEST_F(DatabaseQueryTests, RemovesStudentEnrolment)
{
    assignTestRelationships();

    ASSERT_TRUE(database.removeEnrolment(
        studentId, courseId, error
    ));

    std::vector<EnrolmentRecord> enrolments;

    ASSERT_TRUE(database.getEnrolments(enrolments, error));

    EXPECT_TRUE(enrolments.empty());
}

TEST_F(DatabaseQueryTests, HandlesEnrolmentRetrievalDatabaseFailure)
{
    database.close();

    std::vector<EnrolmentRecord> enrolments;

    EXPECT_FALSE(database.getEnrolments(enrolments, error));

    EXPECT_FALSE(error.empty());
}

TEST_F(DatabaseQueryTests, HandlesEnrolmentRemovalDatabaseFailure)
{
    database.close();

    EXPECT_FALSE(database.removeEnrolment(
        studentId, courseId, error
    ));

    EXPECT_FALSE(error.empty());
}

// =========================================================
// COURSE LECTURER TESTS
// =========================================================

TEST_F(DatabaseQueryTests, RetrievesCourseLecturers)
{
    assignTestRelationships();

    std::vector<CourseLecturerRecord> assignments;

    EXPECT_TRUE(database.getCourseLecturers(
        assignments, error
    ));

    EXPECT_TRUE(error.empty());
    EXPECT_EQ(assignments.size(), 1);
}

TEST_F(DatabaseQueryTests, RetrievesCorrectCourseLecturerInformation)
{
    assignTestRelationships();

    std::vector<CourseLecturerRecord> assignments;

    ASSERT_TRUE(database.getCourseLecturers(
        assignments, error
    ));

    ASSERT_EQ(assignments.size(), 1);

    EXPECT_EQ(assignments[0].courseId, courseId);
    EXPECT_EQ(assignments[0].courseCode, "CS101");
    EXPECT_EQ(assignments[0].lecturerId, lecturerId);
    EXPECT_EQ(assignments[0].lecturerUsername, "lecturer1");
}

TEST_F(DatabaseQueryTests, RemovesLecturerFromCourse)
{
    assignTestRelationships();

    ASSERT_TRUE(database.removeLecturerFromCourse(
        courseId, lecturerId, error
    ));

    std::vector<CourseLecturerRecord> assignments;

    ASSERT_TRUE(database.getCourseLecturers(
        assignments, error
    ));

    EXPECT_TRUE(assignments.empty());
}

TEST_F(DatabaseQueryTests, HandlesCourseLecturerRetrievalDatabaseFailure)
{
    database.close();

    std::vector<CourseLecturerRecord> assignments;

    EXPECT_FALSE(database.getCourseLecturers(
        assignments, error
    ));

    EXPECT_FALSE(error.empty());
}

TEST_F(DatabaseQueryTests, HandlesLecturerRemovalDatabaseFailure)
{
    database.close();

    EXPECT_FALSE(database.removeLecturerFromCourse(
        courseId, lecturerId, error
    ));

    EXPECT_FALSE(error.empty());
}

// =========================================================
// ROLE-BASED COURSE RETRIEVAL TESTS
// =========================================================

TEST_F(DatabaseQueryTests, RetrievesLecturerCourses)
{
    assignTestRelationships();

    std::vector<CourseRecord> courses;

    EXPECT_TRUE(database.getLecturerCourses(
        lecturerId, courses
    ));

    ASSERT_EQ(courses.size(), 1);

    EXPECT_EQ(courses[0].courseCode, "CS101");
}

TEST_F(DatabaseQueryTests, RetrievesStudentCourses)
{
    assignTestRelationships();

    std::vector<CourseRecord> courses;

    EXPECT_TRUE(database.getStudentCourses(
        studentId, courses
    ));

    ASSERT_EQ(courses.size(), 1);

    EXPECT_EQ(courses[0].courseCode, "CS101");
}

TEST_F(DatabaseQueryTests, RetrievesHodCourses)
{
    assignTestRelationships();

    std::vector<CourseRecord> courses;

    EXPECT_TRUE(database.getHodCourses(
        hodId, courses
    ));

    ASSERT_EQ(courses.size(), 1);

    EXPECT_EQ(courses[0].courseCode, "CS101");
}

TEST_F(DatabaseQueryTests, LecturerWithoutAssignmentsHasNoCourses)
{
    std::vector<CourseRecord> courses;

    EXPECT_TRUE(database.getLecturerCourses(
        lecturerId, courses
    ));

    EXPECT_TRUE(courses.empty());
}

TEST_F(DatabaseQueryTests, StudentWithoutEnrolmentsHasNoCourses)
{
    std::vector<CourseRecord> courses;

    EXPECT_TRUE(database.getStudentCourses(
        studentId, courses
    ));

    EXPECT_TRUE(courses.empty());
}

TEST_F(DatabaseQueryTests, HodWithoutDepartmentHasNoCourses)
{
    std::vector<CourseRecord> courses;

    EXPECT_TRUE(database.getHodCourses(
        hodId, courses
    ));

    EXPECT_TRUE(courses.empty());
}

TEST_F(DatabaseQueryTests, ClassExtensionAppliesToCurrentAndFutureEnrolments)
{
    assignTestRelationships();

    ASSERT_TRUE(database.createAssignmentForLecturer(
        lecturerId,
        courseId,
        "Class Extension Test",
        "Assignment for testing a class extension.",
        "2099-12-31 23:59:59",
        100.0
    ));

    std::vector<AssignmentRecord> assignments;
    ASSERT_TRUE(database.getLecturerAssignments(
        lecturerId, assignments
    ));
    ASSERT_EQ(assignments.size(), 1);

    const std::string classDeadline = "2100-01-15 23:59:59";
    ASSERT_TRUE(database.grantClassAssignmentExtension(
        lecturerId,
        assignments[0].assignmentId,
        classDeadline,
        "Whole class extension test",
        error
    )) << error;

    ASSERT_TRUE(database.createUser(
        "student2", "StudentPass456", "STUDENT", error
    )) << error;
    std::vector<UserRecord> users;
    ASSERT_TRUE(database.getUsers(users, error)) << error;
    int secondStudentId = 0;
    for (const auto& user : users)
    {
        if (user.username == "student2")
            secondStudentId = user.userId;
    }
    ASSERT_GT(secondStudentId, 0);
    ASSERT_TRUE(database.enrolStudent(
        secondStudentId, courseId, error
    )) << error;

    for (const int enrolledStudentId : {studentId, secondStudentId})
    {
        std::vector<StudentAssignmentRecord> studentAssignments;
        ASSERT_TRUE(database.getStudentAssignments(
            enrolledStudentId, studentAssignments
        ));
        ASSERT_EQ(studentAssignments.size(), 1);
        EXPECT_EQ(studentAssignments[0].deadline, classDeadline);
        EXPECT_EQ(
            studentAssignments[0].deadlineExtension, "Class extension"
        );
    }

    EXPECT_TRUE(database.submitAssignment(
        studentId, assignments[0].assignmentId, "student1-work.txt"
    ));

    std::vector<SubmissionRecord> submissions;
    ASSERT_TRUE(database.getStudentSubmissions(studentId, submissions));
    ASSERT_EQ(submissions.size(), 1);
    EXPECT_EQ(submissions[0].status, "SUBMITTED");

    AuditTrailFilters filters;
    filters.action = "DEADLINE_EXTENSION_GRANTED";
    filters.entityType = "ASSIGNMENT";
    std::vector<AuditTrailRecord> auditRecords;
    ASSERT_TRUE(database.getAdminAuditTrail(
        adminId, filters, auditRecords, error
    )) << error;
    ASSERT_EQ(auditRecords.size(), 1);
    EXPECT_EQ(auditRecords[0].details, "scope=CLASS");
}

TEST_F(DatabaseQueryTests, IndividualExtensionCanExtendClassDeadline)
{
    assignTestRelationships();
    ASSERT_TRUE(database.createAssignmentForLecturer(
        lecturerId, courseId, "Extension Precedence",
        "Assignment for extension precedence testing.",
        "2099-12-31 23:59:59", 100.0
    ));

    std::vector<AssignmentRecord> assignments;
    ASSERT_TRUE(database.getLecturerAssignments(
        lecturerId, assignments
    ));
    ASSERT_EQ(assignments.size(), 1);
    ASSERT_TRUE(database.grantClassAssignmentExtension(
        lecturerId, assignments[0].assignmentId, "2100-01-15 23:59:59",
        "Class extension", error
    )) << error;
    ASSERT_TRUE(database.grantAssignmentExtension(
        lecturerId, assignments[0].assignmentId, studentId,
        "2100-02-15 23:59:59", "Additional individual time", error
    )) << error;

    std::vector<StudentAssignmentRecord> studentAssignments;
    ASSERT_TRUE(database.getStudentAssignments(
        studentId, studentAssignments
    ));
    ASSERT_EQ(studentAssignments.size(), 1);
    EXPECT_EQ(studentAssignments[0].deadline, "2100-02-15 23:59:59");
    EXPECT_EQ(
        studentAssignments[0].deadlineExtension, "Class + individual"
    );
}

TEST_F(DatabaseQueryTests, RejectsClassExtensionByUnassignedLecturer)
{
    assignTestRelationships();

    ASSERT_TRUE(database.createUser(
        "lecturer2", "LecturerPass456", "LECTURER", error
    )) << error;
    std::vector<UserRecord> users;
    ASSERT_TRUE(database.getUsers(users, error)) << error;
    int unassignedLecturerId = 0;
    for (const auto& user : users)
    {
        if (user.username == "lecturer2")
            unassignedLecturerId = user.userId;
    }
    ASSERT_GT(unassignedLecturerId, 0);

    ASSERT_TRUE(database.createAssignmentForLecturer(
        lecturerId, courseId, "Authorization Test",
        "Assignment for class extension authorization testing.",
        "2099-12-31 23:59:59", 100.0
    ));
    std::vector<AssignmentRecord> assignments;
    ASSERT_TRUE(database.getLecturerAssignments(
        lecturerId, assignments
    ));
    ASSERT_EQ(assignments.size(), 1);

    EXPECT_FALSE(database.grantClassAssignmentExtension(
        unassignedLecturerId, assignments[0].assignmentId,
        "2100-01-15 23:59:59",
        "Unauthorized test", error
    ));
    EXPECT_EQ(error, "This assignment is not assigned to the lecturer.");
}

TEST_F(DatabaseQueryTests, RejectsClassDeadlineThatDoesNotExtendCurrentDeadline)
{
    assignTestRelationships();

    ASSERT_TRUE(database.createAssignmentForLecturer(
        lecturerId, courseId, "Deadline Validation",
        "Assignment for class extension deadline validation.",
        "2099-12-31 23:59:59", 100.0
    ));
    std::vector<AssignmentRecord> assignments;
    ASSERT_TRUE(database.getLecturerAssignments(
        lecturerId, assignments
    ));
    ASSERT_EQ(assignments.size(), 1);

    EXPECT_FALSE(database.grantClassAssignmentExtension(
        lecturerId, assignments[0].assignmentId, "2099-12-31 23:59:59",
        "Not actually an extension", error
    ));
    EXPECT_EQ(
        error, "The revised deadline must be later than the current deadline."
    );
}

TEST_F(DatabaseQueryTests, CreatesClassExtensionTableForExistingDatabase)
{
    assignTestRelationships();
    ASSERT_TRUE(database.execute(
        "DROP TABLE assignment_class_extensions;"
    ));
    ASSERT_TRUE(database.createTables());

    ASSERT_TRUE(database.createAssignmentForLecturer(
        lecturerId, courseId, "Schema Upgrade",
        "Assignment for additive schema upgrade testing.",
        "2099-12-31 23:59:59", 100.0
    ));
    std::vector<AssignmentRecord> assignments;
    ASSERT_TRUE(database.getLecturerAssignments(
        lecturerId, assignments
    ));
    ASSERT_EQ(assignments.size(), 1);

    EXPECT_TRUE(database.grantClassAssignmentExtension(
        lecturerId, assignments[0].assignmentId, "2100-01-15 23:59:59",
        "Schema upgrade test", error
    )) << error;
}
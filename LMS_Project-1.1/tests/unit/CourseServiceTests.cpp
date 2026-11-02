
#include <gtest/gtest.h>

#include "CourseService.h"
#include "Database.h"

#include <string>
#include <vector>

class CourseServiceTests : public ::testing::Test
{
protected:
    Database database;
    CourseService courseService{database};

    std::string error;

    void SetUp() override
    {
        ASSERT_TRUE(database.open(":memory:"));
        ASSERT_TRUE(database.createTables());
    }

    void TearDown() override
    {
        database.close();
    }

    int createTestDepartment()
    {
        error.clear();

        if (!courseService.createDepartment(
                "ADMIN", "Computer Science", error)) {
            return -1;
        }

        std::vector<DepartmentRecord> departments;

        if (!courseService.getDepartments(
                "ADMIN", departments, error)) {
            return -1;
        }

        if (departments.empty()) {
            return -1;
        }

        return departments.front().departmentId;
    }
};

// =========================================================
// CREATE DEPARTMENT TESTS
// =========================================================

TEST_F(CourseServiceTests, RejectsDepartmentCreationByNonAdmin)
{
    EXPECT_FALSE(
        courseService.createDepartment(
            "LECTURER", "Computer Science", error
        )
    );

    EXPECT_FALSE(error.empty());
}

TEST_F(CourseServiceTests, RejectsEmptyDepartmentName)
{
    EXPECT_FALSE(
        courseService.createDepartment(
            "ADMIN", "", error
        )
    );

    EXPECT_EQ(error, "Enter a department name.");
}

TEST_F(CourseServiceTests, RejectsDepartmentNameContainingOnlySpaces)
{
    EXPECT_FALSE(
        courseService.createDepartment(
            "ADMIN", "     ", error
        )
    );

    EXPECT_EQ(error, "Enter a department name.");
}

TEST_F(CourseServiceTests, CreatesDepartmentSuccessfully)
{
    EXPECT_TRUE(
        courseService.createDepartment(
            "ADMIN", "Computer Science", error
        )
    );

    EXPECT_TRUE(error.empty());
}

TEST_F(CourseServiceTests, TrimsDepartmentNameBeforeSaving)
{
    ASSERT_TRUE(
        courseService.createDepartment(
            "ADMIN", "  Computer Science  ", error
        )
    );

    std::vector<DepartmentRecord> departments;

    ASSERT_TRUE(
        courseService.getDepartments(
            "ADMIN", departments, error
        )
    );

    ASSERT_EQ(departments.size(), 1);
    EXPECT_EQ(departments[0].name, "Computer Science");
}

TEST_F(CourseServiceTests, RejectsDuplicateDepartment)
{
    ASSERT_TRUE(
        courseService.createDepartment(
            "ADMIN", "Computer Science", error
        )
    );

    EXPECT_FALSE(
        courseService.createDepartment(
            "ADMIN", "Computer Science", error
        )
    );

    EXPECT_FALSE(error.empty());
}

TEST_F(CourseServiceTests, HandlesDepartmentCreationDatabaseFailure)
{
    database.close();

    EXPECT_FALSE(
        courseService.createDepartment(
            "ADMIN", "Computer Science", error
        )
    );

    EXPECT_FALSE(error.empty());
}

// =========================================================
// GET DEPARTMENTS TESTS
// =========================================================

TEST_F(CourseServiceTests, RejectsDepartmentRetrievalByNonAdmin)
{
    std::vector<DepartmentRecord> departments;

    EXPECT_FALSE(
        courseService.getDepartments(
            "STUDENT", departments, error
        )
    );

    EXPECT_FALSE(error.empty());
}

TEST_F(CourseServiceTests, RetrievesDepartmentsSuccessfully)
{
    ASSERT_TRUE(
        courseService.createDepartment(
            "ADMIN", "Computer Science", error
        )
    );

    ASSERT_TRUE(
        courseService.createDepartment(
            "ADMIN", "Electrical Engineering", error
        )
    );

    std::vector<DepartmentRecord> departments;

    EXPECT_TRUE(
        courseService.getDepartments(
            "ADMIN", departments, error
        )
    );

    EXPECT_TRUE(error.empty());
    EXPECT_EQ(departments.size(), 2);
}

TEST_F(CourseServiceTests, HandlesDepartmentRetrievalDatabaseFailure)
{
    database.close();

    std::vector<DepartmentRecord> departments;

    EXPECT_FALSE(
        courseService.getDepartments(
            "ADMIN", departments, error
        )
    );

    EXPECT_FALSE(error.empty());
}

// =========================================================
// CREATE COURSE TESTS
// =========================================================

TEST_F(CourseServiceTests, RejectsCourseCreationByNonAdmin)
{
    EXPECT_FALSE(
        courseService.createCourse(
            "LECTURER", "CS101", "Programming", 1, error
        )
    );

    EXPECT_FALSE(error.empty());
}

TEST_F(CourseServiceTests, RejectsEmptyCourseCode)
{
    int departmentId = createTestDepartment();
    ASSERT_GT(departmentId, 0);

    EXPECT_FALSE(
        courseService.createCourse(
            "ADMIN", "", "Programming", departmentId, error
        )
    );

    EXPECT_FALSE(error.empty());
}

TEST_F(CourseServiceTests, RejectsEmptyCourseName)
{
    int departmentId = createTestDepartment();
    ASSERT_GT(departmentId, 0);

    EXPECT_FALSE(
        courseService.createCourse(
            "ADMIN", "CS101", "", departmentId, error
        )
    );

    EXPECT_FALSE(error.empty());
}

TEST_F(CourseServiceTests, RejectsCourseFieldsContainingOnlySpaces)
{
    int departmentId = createTestDepartment();
    ASSERT_GT(departmentId, 0);

    EXPECT_FALSE(
        courseService.createCourse(
            "ADMIN", "   ", "   ", departmentId, error
        )
    );

    EXPECT_FALSE(error.empty());
}

TEST_F(CourseServiceTests, RejectsInvalidDepartmentId)
{
    EXPECT_FALSE(
        courseService.createCourse(
            "ADMIN", "CS101", "Programming", 0, error
        )
    );

    EXPECT_EQ(error, "Select a valid department.");
}

TEST_F(CourseServiceTests, RejectsNegativeDepartmentId)
{
    EXPECT_FALSE(
        courseService.createCourse(
            "ADMIN", "CS101", "Programming", -1, error
        )
    );

    EXPECT_EQ(error, "Select a valid department.");
}

TEST_F(CourseServiceTests, CreatesCourseSuccessfully)
{
    int departmentId = createTestDepartment();
    ASSERT_GT(departmentId, 0);

    EXPECT_TRUE(
        courseService.createCourse(
            "ADMIN", "CS101", "Programming Fundamentals",
            departmentId, error
        )
    );

    EXPECT_TRUE(error.empty());
}

TEST_F(CourseServiceTests, TrimsAndConvertsCourseCodeToUppercase)
{
    int departmentId = createTestDepartment();
    ASSERT_GT(departmentId, 0);

    ASSERT_TRUE(
        courseService.createCourse(
            "ADMIN", "  cs101  ", "  Programming Fundamentals  ",
            departmentId, error
        )
    );

    std::vector<CourseRecord> courses;

    ASSERT_TRUE(
        courseService.getCourses(
            "ADMIN", courses, error
        )
    );

    ASSERT_EQ(courses.size(), 1);

    EXPECT_EQ(courses[0].courseCode, "CS101");
    EXPECT_EQ(courses[0].name, "Programming Fundamentals");
}

TEST_F(CourseServiceTests, RejectsDuplicateCourseCode)
{
    int departmentId = createTestDepartment();
    ASSERT_GT(departmentId, 0);

    ASSERT_TRUE(
        courseService.createCourse(
            "ADMIN", "CS101", "Programming Fundamentals",
            departmentId, error
        )
    );

    EXPECT_FALSE(
        courseService.createCourse(
            "ADMIN", "CS101", "Advanced Programming",
            departmentId, error
        )
    );

    EXPECT_FALSE(error.empty());
}

TEST_F(CourseServiceTests, RejectsCourseWithNonexistentDepartment)
{
    EXPECT_FALSE(
        courseService.createCourse(
            "ADMIN", "CS101", "Programming", 99999, error
        )
    );

    EXPECT_FALSE(error.empty());
}

TEST_F(CourseServiceTests, HandlesCourseCreationDatabaseFailure)
{
    int departmentId = createTestDepartment();
    ASSERT_GT(departmentId, 0);

    database.close();

    EXPECT_FALSE(
        courseService.createCourse(
            "ADMIN", "CS101", "Programming",
            departmentId, error
        )
    );

    EXPECT_FALSE(error.empty());
}

// =========================================================
// GET COURSES TESTS
// =========================================================

TEST_F(CourseServiceTests, RejectsCourseRetrievalByNonAdmin)
{
    std::vector<CourseRecord> courses;

    EXPECT_FALSE(
        courseService.getCourses(
            "STUDENT", courses, error
        )
    );

    EXPECT_FALSE(error.empty());
}

TEST_F(CourseServiceTests, RetrievesCoursesSuccessfully)
{
    int departmentId = createTestDepartment();
    ASSERT_GT(departmentId, 0);

    ASSERT_TRUE(
        courseService.createCourse(
            "ADMIN", "CS101", "Programming Fundamentals",
            departmentId, error
        )
    );

    ASSERT_TRUE(
        courseService.createCourse(
            "ADMIN", "CS202", "Data Structures",
            departmentId, error
        )
    );

    std::vector<CourseRecord> courses;

    EXPECT_TRUE(
        courseService.getCourses(
            "ADMIN", courses, error
        )
    );

    EXPECT_TRUE(error.empty());
    EXPECT_EQ(courses.size(), 2);
}

TEST_F(CourseServiceTests, HandlesCourseRetrievalDatabaseFailure)
{
    database.close();

    std::vector<CourseRecord> courses;

    EXPECT_FALSE(
        courseService.getCourses(
            "ADMIN", courses, error
        )
    );

    EXPECT_FALSE(error.empty());
}

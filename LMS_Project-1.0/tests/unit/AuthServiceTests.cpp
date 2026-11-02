
#include <gtest/gtest.h>

#include "AuthService.h"
#include "Database.h"

#include <string>

class AuthServiceTests : public ::testing::Test
{
protected:
    Database database;
    AuthService authService;

    AuthenticatedUser user{0, "", ""};

    AuthServiceTests()
        : authService(database)
    {
    }

    void SetUp() override
    {
        ASSERT_TRUE(database.open(":memory:"));
        ASSERT_TRUE(database.createTables());
    }

    void TearDown() override
    {
        database.close();
    }

    void createStudentAccount()
    {
        std::string error;

        ASSERT_TRUE(
            database.createUser(
                "student1",
                "CorrectPassword123!",
                "STUDENT",
                error
            )
        ) << error;
    }
};

// Test 1: Empty username must be rejected.
TEST_F(AuthServiceTests, RejectsEmptyUsername)
{
    EXPECT_FALSE(
        authService.login(
            "",
            "CorrectPassword123!",
            user
        )
    );
}

// Test 2: Empty password must be rejected.
TEST_F(AuthServiceTests, RejectsEmptyPassword)
{
    EXPECT_FALSE(
        authService.login(
            "student1",
            "",
            user
        )
    );
}

// Test 3: Unknown username must be rejected.
TEST_F(AuthServiceTests, RejectsUnknownUsername)
{
    EXPECT_FALSE(
        authService.login(
            "unknown",
            "CorrectPassword123!",
            user
        )
    );
}

// Test 4: Incorrect password must be rejected.
TEST_F(AuthServiceTests, RejectsIncorrectPassword)
{
    createStudentAccount();

    EXPECT_FALSE(
        authService.login(
            "student1",
            "WrongPassword123!",
            user
        )
    );
}

// Test 5: Valid credentials must be accepted.
TEST_F(AuthServiceTests, AcceptsValidCredentials)
{
    createStudentAccount();

    EXPECT_TRUE(
        authService.login(
            "student1",
            "CorrectPassword123!",
            user
        )
    );
}

// Test 6: Successful login must return correct user details.
TEST_F(AuthServiceTests, ReturnsCorrectAuthenticatedUserDetails)
{
    createStudentAccount();

    ASSERT_TRUE(
        authService.login(
            "student1",
            "CorrectPassword123!",
            user
        )
    );

    EXPECT_GT(user.userId, 0);
    EXPECT_EQ(user.username, "student1");
    EXPECT_EQ(user.role, "STUDENT");
}

// Test 7: Failed login must not overwrite existing user details.
TEST_F(AuthServiceTests, FailedLoginDoesNotModifyUserDetails)
{
    createStudentAccount();

    user.userId = 999;
    user.username = "existingUser";
    user.role = "ADMIN";

    EXPECT_FALSE(
        authService.login(
            "student1",
            "WrongPassword123!",
            user
        )
    );

    EXPECT_EQ(user.userId, 999);
    EXPECT_EQ(user.username, "existingUser");
    EXPECT_EQ(user.role, "ADMIN");
}

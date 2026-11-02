
#include "LatePenaltyStrategy.h"

#include <gtest/gtest.h>

class LatePenaltyTests : public ::testing::Test {
protected:
    StandardLatePenalty strategy;

    const double maxMark = 100.0;
    const double rawMark = 80.0;
    const std::string deadline = "2026-01-01 12:00:00";
};

// 1. Submission exactly on the deadline.
TEST_F(LatePenaltyTests, SubmissionOnDeadlineHasNoPenalty)
{
    EXPECT_DOUBLE_EQ(
        strategy.calculate(rawMark, maxMark, deadline, deadline),
        0.0
    );
}

// 2. Submission one day late: 10% of maximum mark.
TEST_F(LatePenaltyTests, SubmissionOneDayLateLosesTenPercentOfMaximum)
{
    EXPECT_DOUBLE_EQ(
        strategy.calculate(
            rawMark, maxMark,
            "2026-01-02 12:00:00", deadline
        ),
        10.0
    );
}

// 3. Submission two days late: 20% of maximum mark.
TEST_F(LatePenaltyTests, SubmissionTwoDaysLateLosesTwentyPercentOfMaximum)
{
    EXPECT_DOUBLE_EQ(
        strategy.calculate(
            rawMark, maxMark,
            "2026-01-03 12:00:00", deadline
        ),
        20.0
    );
}

// 4. Submission three days late: 30% of maximum mark.
TEST_F(LatePenaltyTests, SubmissionThreeDaysLateLosesThirtyPercentOfMaximum)
{
    EXPECT_DOUBLE_EQ(
        strategy.calculate(
            rawMark, maxMark,
            "2026-01-04 12:00:00", deadline
        ),
        30.0
    );
}

// 5. Submission more than three days late: entire raw mark is deducted.
TEST_F(LatePenaltyTests, SubmissionMoreThanThreeDaysLateLosesAllMarks)
{
    EXPECT_DOUBLE_EQ(
        strategy.calculate(
            rawMark, maxMark,
            "2026-01-04 12:00:01", deadline
        ),
        rawMark
    );
}

// 6. Penalty cannot exceed the student's raw mark.
TEST_F(LatePenaltyTests, PenaltyCannotExceedRawMark)
{
    EXPECT_DOUBLE_EQ(
        strategy.calculate(
            5.0, maxMark,
            "2026-01-02 12:00:00", deadline
        ),
        5.0
    );
}

// 7. Invalid submission date.
TEST_F(LatePenaltyTests, InvalidDateReturnsNoPenalty)
{
    EXPECT_DOUBLE_EQ(
        strategy.calculate(
            rawMark, maxMark,
            "invalid-date", deadline
        ),
        0.0
    );
}

// 8. Maximum mark is zero.
TEST_F(LatePenaltyTests, ZeroMaximumMarkReturnsNoPenalty)
{
    EXPECT_DOUBLE_EQ(
        strategy.calculate(
            rawMark, 0.0,
            "2026-01-02 12:00:00", deadline
        ),
        0.0
    );
}

// 9. Raw mark is zero.
TEST_F(LatePenaltyTests, ZeroRawMarkReturnsNoPenalty)
{
    EXPECT_DOUBLE_EQ(
        strategy.calculate(
            0.0, maxMark,
            "2026-01-02 12:00:00", deadline
        ),
        0.0
    );
}

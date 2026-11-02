#include "LatePenaltyStrategy.h"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace {

bool parseSqliteDateTime(const std::string& value, std::tm& out)
{
    if (value.empty()) {
        return false;
    }

    std::istringstream stream(value);
    stream >> std::get_time(&out, "%Y-%m-%d %H:%M:%S");

    return !stream.fail();
}

}

double StandardLatePenalty::calculate(
    double rawMark,
    double maxMark,
    const std::string& submittedAt,
    const std::string& deadline
) const
{
    if (maxMark <= 0.0 || rawMark <= 0.0) {
        return 0.0;
    }

    std::tm submitted = {};
    std::tm due = {};

    if (!parseSqliteDateTime(submittedAt, submitted) ||
        !parseSqliteDateTime(deadline, due)) {
        return 0.0;
    }

    const auto submittedTime =
        std::chrono::system_clock::from_time_t(std::mktime(&submitted));

    const auto deadlineTime =
        std::chrono::system_clock::from_time_t(std::mktime(&due));

    if (submittedTime <= deadlineTime) {
        return 0.0;
    }

    const auto elapsedSeconds =
        std::chrono::duration_cast<std::chrono::seconds>(
            submittedTime - deadlineTime
        ).count();

    // Count any started 24-hour period as a late day.
    const int daysLate = static_cast<int>(
        (elapsedSeconds + 86399) / 86400
    );

    // More than three days late means the student receives zero.
    if (daysLate > 3) {
        return rawMark;
    }

    // 10% of the assignment's maximum mark per late day.
    const double penalty = maxMark * 0.10 * daysLate;

    // A penalty cannot exceed the student's raw mark.
    return std::min(rawMark, penalty);
}
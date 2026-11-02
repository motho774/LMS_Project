#include "CourseService.h"

#include <algorithm>
#include <cctype>

namespace {
std::string trim(const std::string& value)
{
    const auto first = std::find_if_not(
        value.begin(), value.end(),
        [](unsigned char ch) { return std::isspace(ch); }
    );

    if (first == value.end()) {
        return "";
    }

    const auto last = std::find_if_not(
        value.rbegin(), value.rend(),
        [](unsigned char ch) { return std::isspace(ch); }
    ).base();

    return std::string(first, last);
}
}

CourseService::CourseService(Database& database)
    : database(database)
{
}

bool CourseService::createDepartment(
    const std::string& role,
    const std::string& name,
    std::string& error
)
{
    if (role != "ADMIN") {
        error = "Only a system administrator can create departments.";
        return false;
    }

    const std::string cleanedName = trim(name);
    if (cleanedName.empty()) {
        error = "Enter a department name.";
        return false;
    }

    if (!database.createDepartment(cleanedName)) {
        error = "Could not create the department. It may already exist.";
        return false;
    }

    error.clear();
    return true;
}

bool CourseService::getDepartments(
    const std::string& role,
    std::vector<DepartmentRecord>& departments,
    std::string& error
)
{
    if (role != "ADMIN") {
        error = "You are not authorized to view department management.";
        return false;
    }

    if (!database.getDepartments(departments)) {
        error = "Could not load departments from the database.";
        return false;
    }

    error.clear();
    return true;
}

bool CourseService::createCourse(
    const std::string& role,
    const std::string& courseCode,
    const std::string& name,
    int departmentId,
    std::string& error
)
{
    if (role != "ADMIN") {
        error = "Only a system administrator can create courses.";
        return false;
    }

    std::string cleanedCode = trim(courseCode);
    std::string cleanedName = trim(name);

    std::transform(
        cleanedCode.begin(), cleanedCode.end(), cleanedCode.begin(),
        [](unsigned char ch) { return static_cast<char>(std::toupper(ch)); }
    );

    if (cleanedCode.empty() || cleanedName.empty()) {
        error = "Course code and course name are both required.";
        return false;
    }

    if (departmentId <= 0) {
        error = "Select a valid department.";
        return false;
    }

    if (!database.createCourse(cleanedCode, cleanedName, departmentId)) {
        error = "Could not create the course. Check the course code is unique "
                "and the selected department still exists.";
        return false;
    }

    error.clear();
    return true;
}

bool CourseService::getCourses(
    const std::string& role,
    std::vector<CourseRecord>& courses,
    std::string& error
)
{
    if (role != "ADMIN") {
        error = "You are not authorized to view course management.";
        return false;
    }

    if (!database.getCourses(courses)) {
        error = "Could not load courses from the database.";
        return false;
    }

    error.clear();
    return true;
}

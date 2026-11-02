#ifndef COURSESERVICE_H
#define COURSESERVICE_H

#include "Database.h"
#include <string>
#include <vector>

class CourseService {
private:
    Database& database;

public:
    explicit CourseService(Database& database);

    bool createDepartment(
        const std::string& role,
        const std::string& name,
        std::string& error
    );

    bool getDepartments(
        const std::string& role,
        std::vector<DepartmentRecord>& departments,
        std::string& error
    );

    bool createCourse(
        const std::string& role,
        const std::string& courseCode,
        const std::string& name,
        int departmentId,
        std::string& error
    );

    bool getCourses(
        const std::string& role,
        std::vector<CourseRecord>& courses,
        std::string& error
    );
};

#endif

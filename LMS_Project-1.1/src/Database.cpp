#include "Database.h"
#include "PasswordUtils.h"
#include <limits>
#include <iostream>


Database::Database() : db(nullptr) {
}

Database::~Database() {
    close();
}

bool Database::open(const std::string& filename) {
    int result = sqlite3_open(filename.c_str(), &db);

    if (result != SQLITE_OK) {
        std::cerr << "Database connection failed: "
                  << sqlite3_errmsg(db) << std::endl;
        close();
        return false;
    }

    if (!execute("PRAGMA foreign_keys = ON;")) {
        close();
        return false;
    }

    std::cout << "Database connected successfully." << std::endl;
    return true;
}

bool Database::execute(const std::string& sql) {
    char* errorMessage = nullptr;

    int result = sqlite3_exec(
        db, sql.c_str(), nullptr, nullptr, &errorMessage
    );

    if (result != SQLITE_OK) {
        std::cerr << "SQL error: "
                  << (errorMessage ? errorMessage : "Unknown error")
                  << std::endl;

        sqlite3_free(errorMessage);
        return false;
    }

    return true;
}

bool Database::createTables() {
    const std::string schema = R"SQL(

    CREATE TABLE IF NOT EXISTS users (
        user_id INTEGER PRIMARY KEY AUTOINCREMENT,
        username TEXT NOT NULL UNIQUE,
        password_hash TEXT NOT NULL,
        password_salt TEXT NOT NULL,
        role TEXT NOT NULL CHECK (
            role IN ('STUDENT', 'LECTURER', 'HOD', 'ADMIN')
        ),
        active INTEGER NOT NULL DEFAULT 1
            CHECK (active IN (0, 1))
    );

    CREATE TABLE IF NOT EXISTS departments (
        department_id INTEGER PRIMARY KEY AUTOINCREMENT,
        name TEXT NOT NULL UNIQUE,
        hod_user_id INTEGER,
        FOREIGN KEY (hod_user_id) REFERENCES users(user_id)
    );

    CREATE TABLE IF NOT EXISTS courses (
        course_id INTEGER PRIMARY KEY AUTOINCREMENT,
        course_code TEXT NOT NULL UNIQUE,
        name TEXT NOT NULL,
        department_id INTEGER NOT NULL,
        FOREIGN KEY (department_id)
            REFERENCES departments(department_id)
    );

    CREATE TABLE IF NOT EXISTS course_lecturers (
        course_id INTEGER NOT NULL,
        lecturer_id INTEGER NOT NULL,
        PRIMARY KEY (course_id, lecturer_id),
        FOREIGN KEY (course_id) REFERENCES courses(course_id),
        FOREIGN KEY (lecturer_id) REFERENCES users(user_id)
    );

    CREATE TABLE IF NOT EXISTS enrolments (
        student_id INTEGER NOT NULL,
        course_id INTEGER NOT NULL,
        enrolled_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
        PRIMARY KEY (student_id, course_id),
        FOREIGN KEY (student_id) REFERENCES users(user_id),
        FOREIGN KEY (course_id) REFERENCES courses(course_id)
    );

    CREATE TABLE IF NOT EXISTS assignments (
        assignment_id INTEGER PRIMARY KEY AUTOINCREMENT,
        course_id INTEGER NOT NULL,
        title TEXT NOT NULL,
        description TEXT NOT NULL,
        deadline TEXT NOT NULL,
        max_mark REAL NOT NULL CHECK (max_mark > 0),
        max_attempts INTEGER NOT NULL DEFAULT 2
            CHECK (max_attempts >= 1),
        FOREIGN KEY (course_id) REFERENCES courses(course_id)
    );

    CREATE TABLE IF NOT EXISTS extensions (
        extension_id INTEGER PRIMARY KEY AUTOINCREMENT,
        assignment_id INTEGER NOT NULL,
        student_id INTEGER NOT NULL,
        revised_deadline TEXT NOT NULL,
        granted_by INTEGER NOT NULL,
        reason TEXT NOT NULL,
        UNIQUE (assignment_id, student_id),
        FOREIGN KEY (assignment_id)
            REFERENCES assignments(assignment_id),
        FOREIGN KEY (student_id) REFERENCES users(user_id),
        FOREIGN KEY (granted_by) REFERENCES users(user_id)
    );

    CREATE TABLE IF NOT EXISTS assignment_class_extensions (
        assignment_id INTEGER PRIMARY KEY,
        revised_deadline TEXT NOT NULL,
        granted_by INTEGER NOT NULL,
        reason TEXT NOT NULL,
        granted_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
        FOREIGN KEY (assignment_id) REFERENCES assignments(assignment_id),
        FOREIGN KEY (granted_by) REFERENCES users(user_id)
    );

    CREATE TABLE IF NOT EXISTS submissions (
        submission_id INTEGER PRIMARY KEY AUTOINCREMENT,
        assignment_id INTEGER NOT NULL,
        student_id INTEGER NOT NULL,
        supersedes_submission_id INTEGER,
        version_number INTEGER NOT NULL CHECK (version_number >= 1),
        submitted_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
        status TEXT NOT NULL CHECK (
            status IN (
                'SUBMITTED', 'LATE', 'MARKED', 'MODERATED', 'RELEASED'
            )
        ),
        file_reference TEXT NOT NULL,
        UNIQUE (assignment_id, student_id, version_number),
        FOREIGN KEY (assignment_id) REFERENCES assignments(assignment_id),
        FOREIGN KEY (student_id) REFERENCES users(user_id),
        FOREIGN KEY (supersedes_submission_id)
            REFERENCES submissions(submission_id)
    );

    CREATE TABLE IF NOT EXISTS marks (
        mark_id INTEGER PRIMARY KEY AUTOINCREMENT,
        submission_id INTEGER NOT NULL UNIQUE,
        marked_by INTEGER NOT NULL,
        raw_mark REAL NOT NULL CHECK (raw_mark >= 0),
        final_mark REAL NOT NULL CHECK (final_mark >= 0),
        feedback TEXT NOT NULL,
        marked_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
        FOREIGN KEY (submission_id)
            REFERENCES submissions(submission_id),
        FOREIGN KEY (marked_by) REFERENCES users(user_id)
    );

    CREATE TABLE IF NOT EXISTS moderation_records (
        moderation_id INTEGER PRIMARY KEY AUTOINCREMENT,
        mark_id INTEGER NOT NULL,
        hod_id INTEGER NOT NULL,
        decision TEXT NOT NULL CHECK (
            decision IN ('APPROVED', 'RETURNED')
        ),
        comment TEXT NOT NULL,
        decided_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
        FOREIGN KEY (mark_id) REFERENCES marks(mark_id),
        FOREIGN KEY (hod_id) REFERENCES users(user_id)
    );

    CREATE TABLE IF NOT EXISTS mark_audit (
        audit_id INTEGER PRIMARY KEY AUTOINCREMENT,
        mark_id INTEGER NOT NULL,
        changed_by INTEGER NOT NULL,
        old_mark REAL CHECK (old_mark IS NULL OR old_mark >= 0),
        new_mark REAL NOT NULL CHECK (new_mark >= 0),
        reason TEXT NOT NULL,
        changed_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
        FOREIGN KEY (mark_id) REFERENCES marks(mark_id),
        FOREIGN KEY (changed_by) REFERENCES users(user_id)
    );

    CREATE TABLE IF NOT EXISTS system_audit (
        audit_id INTEGER PRIMARY KEY AUTOINCREMENT,
        actor_id INTEGER,
        action TEXT NOT NULL,
        entity_type TEXT NOT NULL,
        entity_id INTEGER NOT NULL DEFAULT 0,
        old_value TEXT NOT NULL DEFAULT '',
        new_value TEXT NOT NULL DEFAULT '',
        reason TEXT NOT NULL DEFAULT '',
        details TEXT NOT NULL DEFAULT '',
        occurred_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
        FOREIGN KEY (actor_id) REFERENCES users(user_id) ON DELETE SET NULL
    );

    CREATE INDEX IF NOT EXISTS idx_assignments_course
        ON assignments(course_id);

    CREATE INDEX IF NOT EXISTS idx_submissions_assignment_student
        ON submissions(assignment_id, student_id);

    CREATE INDEX IF NOT EXISTS idx_submissions_status
        ON submissions(status);

    CREATE INDEX IF NOT EXISTS idx_moderation_mark_time
        ON moderation_records(mark_id, decided_at);

    CREATE INDEX IF NOT EXISTS idx_mark_audit_mark_time
        ON mark_audit(mark_id, changed_at);

    CREATE INDEX IF NOT EXISTS idx_system_audit_time
        ON system_audit(occurred_at);

    CREATE INDEX IF NOT EXISTS idx_system_audit_action
        ON system_audit(action, occurred_at);

    )SQL";

    if (!execute(schema)) {
        return false;
    }

    sqlite3_stmt* columns = nullptr;
    if (sqlite3_prepare_v2(
            db, "PRAGMA table_info(assignments);", -1,
            &columns, nullptr) != SQLITE_OK) {
        std::cerr << "Could not inspect assignment schema: "
                  << sqlite3_errmsg(db) << '\n';
        return false;
    }

    bool hasMaxAttempts = false;
    int rc;
    while ((rc = sqlite3_step(columns)) == SQLITE_ROW) {
        const auto* columnName = sqlite3_column_text(columns, 1);
        if (columnName &&
            std::string(reinterpret_cast<const char*>(columnName)) ==
                "max_attempts") {
            hasMaxAttempts = true;
            break;
        }
    }
    sqlite3_finalize(columns);
    if (rc != SQLITE_ROW && rc != SQLITE_DONE) {
        std::cerr << "Could not inspect assignment schema: "
                  << sqlite3_errmsg(db) << '\n';
        return false;
    }

    if (!hasMaxAttempts &&
        !execute(
            "ALTER TABLE assignments ADD COLUMN max_attempts "
            "INTEGER NOT NULL DEFAULT 2 CHECK (max_attempts >= 1);")) {
        return false;
    }

    std::cout << "LMS database tables created successfully."
              << std::endl;

    return true;
}

bool Database::writeAuditEvent(
    int actorId,
    const std::string& action,
    const std::string& entityType,
    int entityId,
    const std::string& oldValue,
    const std::string& newValue,
    const std::string& reason,
    const std::string& details,
    std::string& error
) {
    if (db == nullptr || action.empty() || entityType.empty()) {
        error = "Invalid audit event.";
        return false;
    }

    const char* sql = R"SQL(
        INSERT INTO system_audit (
            actor_id, action, entity_type, entity_id,
            old_value, new_value, reason, details
        )
        VALUES (?, ?, ?, ?, ?, ?, ?, ?);
    )SQL";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        error = sqlite3_errmsg(db);
        return false;
    }

    if (actorId > 0) {
        sqlite3_bind_int(stmt, 1, actorId);
    } else {
        sqlite3_bind_null(stmt, 1);
    }
    sqlite3_bind_text(stmt, 2, action.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, entityType.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, entityId);
    sqlite3_bind_text(stmt, 5, oldValue.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, newValue.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, reason.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 8, details.c_str(), -1, SQLITE_TRANSIENT);

    const int rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        error = sqlite3_errmsg(db);
    }
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

void Database::close() {
    if (db != nullptr) {
        sqlite3_close(db);
        db = nullptr;
    }
}

bool Database::createUser(
    const std::string& username,
    const std::string& password,
    const std::string& role
)
{
    std::string error;

    // Validate the input and create the account.
    if (!createUser(username, password, role, error))
    {
        std::cerr << "Could not create user: "
                  << error << std::endl;

        return false;
    }

    std::cout << "User created successfully." << std::endl;
    return true;
}


bool Database::createUser(
    const std::string& username,
    const std::string& password,
    const std::string& role,
    std::string& error
)
{
    error.clear();

    // 1. Check that the database connection is available.
    if (db == nullptr)
    {
        error = "Database is not open.";
        return false;
    }

    // 2. Validate the username.
    if (username.empty())
    {
        error = "Username cannot be empty.";
        return false;
    }

    // 3. Validate the password.
    if (password.empty())
    {
        error = "Password cannot be empty.";
        return false;
    }

    // 4. Validate the user role.
    if (role != "STUDENT" &&
        role != "LECTURER" &&
        role != "HOD" &&
        role != "ADMIN")
    {
        error = "Invalid role. Choose STUDENT, LECTURER, HOD, or ADMIN.";
        return false;
    }

    // 5. Hash the password before storing it.
    std::string hash;
    std::string salt;

    if (!PasswordUtils::hashPassword(password, hash, salt))
    {
        error = "Password hashing failed.";
        return false;
    }

    // 6. Prepare the SQL statement.
    const char* sql =
        "INSERT INTO users "
        "(username, password_hash, password_salt, role) "
        "VALUES (?, ?, ?, ?);";

    sqlite3_stmt* statement = nullptr;

    int rc = sqlite3_prepare_v2(
        db,
        sql,
        -1,
        &statement,
        nullptr
    );

    if (rc != SQLITE_OK)
    {
        error = sqlite3_errmsg(db);
        return false;
    }

    // 7. Bind the values to the prepared statement.
    sqlite3_bind_text(
        statement,
        1,
        username.c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    sqlite3_bind_text(
        statement,
        2,
        hash.c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    sqlite3_bind_text(
        statement,
        3,
        salt.c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    sqlite3_bind_text(
        statement,
        4,
        role.c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    // 8. Execute the SQL statement.
    rc = sqlite3_step(statement);

    if (rc != SQLITE_DONE)
    {
        error = sqlite3_errmsg(db);

        // Provide a clearer message for duplicate usernames.
        if (sqlite3_extended_errcode(db) == SQLITE_CONSTRAINT_UNIQUE)
        {
            error = "That username already exists. Choose another username.";
        }

        sqlite3_finalize(statement);
        return false;
    }

    // 9. Release the prepared statement.
    sqlite3_finalize(statement);

    error.clear();
    return true;
}

bool Database::updateUser(
    int userId,
    const std::string& username,
    const std::string& role,
    const std::string& newPassword,
    std::string& error
) {
    error.clear();

    if (db == nullptr) {
        error = "Database is not open.";
        return false;
    }

    if (userId <= 0 || username.empty()) {
        error = "Invalid user details.";
        return false;
    }

    if (role != "STUDENT" &&
        role != "LECTURER" &&
        role != "HOD" &&
        role != "ADMIN") {
        error = "Invalid user role.";
        return false;
    }

    std::string hash;
    std::string salt;

    if (!newPassword.empty()) {
        if (!PasswordUtils::hashPassword(
                newPassword, hash, salt)) {
            error = "Password hashing failed.";
            return false;
        }
    }

    const char* sql;

    if (newPassword.empty()) {
        sql =
            "UPDATE users "
            "SET username = ?, role = ? "
            "WHERE user_id = ?;";
    }
    else {
        sql =
            "UPDATE users "
            "SET username = ?, role = ?, "
            "password_hash = ?, password_salt = ? "
            "WHERE user_id = ?;";
    }

    sqlite3_stmt* statement = nullptr;

    int rc = sqlite3_prepare_v2(
        db, sql, -1, &statement, nullptr
    );

    if (rc != SQLITE_OK) {
        error = sqlite3_errmsg(db);
        return false;
    }

    sqlite3_bind_text(
        statement, 1, username.c_str(), -1, SQLITE_TRANSIENT
    );

    sqlite3_bind_text(
        statement, 2, role.c_str(), -1, SQLITE_TRANSIENT
    );

    if (newPassword.empty()) {
        sqlite3_bind_int(statement, 3, userId);
    }
    else {
        sqlite3_bind_text(
            statement, 3, hash.c_str(), -1, SQLITE_TRANSIENT
        );

        sqlite3_bind_text(
            statement, 4, salt.c_str(), -1, SQLITE_TRANSIENT
        );

        sqlite3_bind_int(statement, 5, userId);
    }

    rc = sqlite3_step(statement);

    if (rc != SQLITE_DONE) {
        error = sqlite3_errmsg(db);
        sqlite3_finalize(statement);
        return false;
    }

    if (sqlite3_changes(db) == 0) {
        error = "User not found or no changes were made.";
        sqlite3_finalize(statement);
        return false;
    }

    sqlite3_finalize(statement);

    return true;
}

bool Database::login(
    const std::string& username,
    const std::string& password,
    int& userId,
    std::string& role
) {
    if (db == nullptr || username.empty() || password.empty()) {
        return false;
    }

    const char* sql =
        "SELECT user_id, password_hash, password_salt, role "
        "FROM users "
        "WHERE username = ? AND active = 1;";

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &statement, nullptr)
        != SQLITE_OK) {
        std::cerr << "Could not prepare login statement."
                  << std::endl;
        return false;
    }

    sqlite3_bind_text(
        statement, 1, username.c_str(), -1, SQLITE_TRANSIENT
    );

    int result = sqlite3_step(statement);

    if (result != SQLITE_ROW) {
        sqlite3_finalize(statement);
        return false;
    }

    int foundUserId = sqlite3_column_int(statement, 0);

    const unsigned char* hashText = sqlite3_column_text(statement, 1);
    const unsigned char* saltText = sqlite3_column_text(statement, 2);
    const unsigned char* roleText = sqlite3_column_text(statement, 3);

    std::string storedHash =
        hashText ? reinterpret_cast<const char*>(hashText) : "";
    std::string storedSalt =
        saltText ? reinterpret_cast<const char*>(saltText) : "";
    std::string storedRole =
        roleText ? reinterpret_cast<const char*>(roleText) : "";

    sqlite3_finalize(statement);

    if (!PasswordUtils::verifyPassword(
            password, storedHash, storedSalt)) {
        return false;
    }

    userId = foundUserId;
    role = storedRole;

    return true;
}

bool Database::createDepartment(const std::string& name)
{
    if (db == nullptr || name.empty()) {
        return false;
    }

    const char* sql = "INSERT INTO departments (name) VALUES (?);";
    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &statement, nullptr) != SQLITE_OK) {
        std::cerr << "Could not prepare department creation: "
                  << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    sqlite3_bind_text(statement, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    const int result = sqlite3_step(statement);

    if (result != SQLITE_DONE) {
        std::cerr << "Could not create department: "
                  << sqlite3_errmsg(db) << std::endl;
        sqlite3_finalize(statement);
        return false;
    }

    sqlite3_finalize(statement);
    return true;
}

bool Database::getDepartments(std::vector<DepartmentRecord>& departments)
{
    departments.clear();

    if (db == nullptr) {
        return false;
    }

    const char* sql =
        "SELECT department_id, name FROM departments ORDER BY name;";
    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &statement, nullptr) != SQLITE_OK) {
        std::cerr << "Could not prepare department query: "
                  << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    int result;
    while ((result = sqlite3_step(statement)) == SQLITE_ROW) {
        DepartmentRecord department;
        department.departmentId = sqlite3_column_int(statement, 0);

        const unsigned char* text =
            sqlite3_column_text(statement, 1);
        department.name = text
            ? reinterpret_cast<const char*>(text)
            : "";

        departments.push_back(department);
    }

    const bool success = (result == SQLITE_DONE);
    if (!success) {
        std::cerr << "Could not retrieve departments: "
                  << sqlite3_errmsg(db) << std::endl;
    }

    sqlite3_finalize(statement);
    return success;
}

bool Database::createCourse(
    const std::string& courseCode,
    const std::string& name,
    int departmentId
)
{
    if (db == nullptr ||
        courseCode.empty() ||
        name.empty() ||
        departmentId <= 0) {
        return false;
    }

    const char* sql =
        "INSERT INTO courses (course_code, name, department_id) "
        "VALUES (?, ?, ?);";
    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &statement, nullptr) != SQLITE_OK) {
        std::cerr << "Could not prepare course creation: "
                  << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    sqlite3_bind_text(
        statement, 1, courseCode.c_str(), -1, SQLITE_TRANSIENT
    );
    sqlite3_bind_text(
        statement, 2, name.c_str(), -1, SQLITE_TRANSIENT
    );
    sqlite3_bind_int(statement, 3, departmentId);

    const int result = sqlite3_step(statement);

    if (result != SQLITE_DONE) {
        std::cerr << "Could not create course: "
                  << sqlite3_errmsg(db) << std::endl;
        sqlite3_finalize(statement);
        return false;
    }

    sqlite3_finalize(statement);
    return true;
}

bool Database::getCourses(std::vector<CourseRecord>& courses)
{
    courses.clear();

    if (db == nullptr) {
        return false;
    }

    const char* sql =
        "SELECT c.course_id, c.course_code, c.name, "
        "       d.department_id, d.name "
        "FROM courses AS c "
        "JOIN departments AS d ON d.department_id = c.department_id "
        "ORDER BY c.course_code;";
    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &statement, nullptr) != SQLITE_OK) {
        std::cerr << "Could not prepare course query: "
                  << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    int result;
    while ((result = sqlite3_step(statement)) == SQLITE_ROW) {
        CourseRecord course;
        course.courseId = sqlite3_column_int(statement, 0);

        const unsigned char* code = sqlite3_column_text(statement, 1);
        const unsigned char* courseName = sqlite3_column_text(statement, 2);
        const unsigned char* departmentName = sqlite3_column_text(statement, 4);

        course.courseCode = code
            ? reinterpret_cast<const char*>(code) : "";
        course.name = courseName
            ? reinterpret_cast<const char*>(courseName) : "";
        course.departmentId = sqlite3_column_int(statement, 3);
        course.departmentName = departmentName
            ? reinterpret_cast<const char*>(departmentName) : "";

        courses.push_back(course);
    }

    const bool success = (result == SQLITE_DONE);
    if (!success) {
        std::cerr << "Could not retrieve courses: "
                  << sqlite3_errmsg(db) << std::endl;
    }

    sqlite3_finalize(statement);
    return success;
}

bool Database::getLecturerCourses(
    int lecturerId,
    std::vector<CourseRecord>& courses
) {
    courses.clear();

    if (db == nullptr || lecturerId <= 0) {
        return false;
    }

    const char* sql = R"SQL(
        SELECT c.course_id, c.course_code, c.name,
               d.department_id, d.name
        FROM courses c
        JOIN departments d
          ON d.department_id = c.department_id
        JOIN course_lecturers cl
          ON cl.course_id = c.course_id
        WHERE cl.lecturer_id = ?
        ORDER BY c.course_code;
    )SQL";

    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Could not retrieve lecturer courses: "
                  << sqlite3_errmsg(db) << '\n';
        return false;
    }

    sqlite3_bind_int(stmt, 1, lecturerId);

    int result;
    while ((result = sqlite3_step(stmt)) == SQLITE_ROW) {
        CourseRecord course{};

        course.courseId = sqlite3_column_int(stmt, 0);

        const auto* code = sqlite3_column_text(stmt, 1);
        const auto* name = sqlite3_column_text(stmt, 2);
        const auto* department = sqlite3_column_text(stmt, 4);

        course.courseCode = code
            ? reinterpret_cast<const char*>(code) : "";
        course.name = name
            ? reinterpret_cast<const char*>(name) : "";
        course.departmentId = sqlite3_column_int(stmt, 3);
        course.departmentName = department
            ? reinterpret_cast<const char*>(department) : "";

        courses.push_back(course);
    }

    bool success = (result == SQLITE_DONE);

    if (!success) {
        std::cerr << "Course query failed: "
                  << sqlite3_errmsg(db) << '\n';
    }

    sqlite3_finalize(stmt);
    return success;
}

bool Database::createAssignmentForLecturer(
    int lecturerId,
    int courseId,
    const std::string& title,
    const std::string& description,
    const std::string& deadline,
    double maxMark,
    int maxAttempts
) {
    if (db == nullptr ||
        lecturerId <= 0 ||
        courseId <= 0 ||
        title.empty() ||
        description.empty() ||
        deadline.empty() ||
        maxMark <= 0 ||
        maxAttempts < 1) {
        return false;
    }

    if (!execute("BEGIN IMMEDIATE TRANSACTION;")) {
        std::cerr << "Could not start assignment creation transaction.\n";
        return false;
    }

    const char* sql = R"SQL(
        INSERT INTO assignments
            (course_id, title, description, deadline, max_mark, max_attempts)
        SELECT c.course_id, ?, ?, ?, ?, ?
        FROM courses c
        JOIN course_lecturers cl
          ON cl.course_id = c.course_id
        WHERE c.course_id = ?
          AND cl.lecturer_id = ?;
    )SQL";

    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Could not prepare assignment creation: "
                  << sqlite3_errmsg(db) << '\n';
        execute("ROLLBACK;");
        return false;
    }

    sqlite3_bind_text(
        stmt, 1, title.c_str(), -1, SQLITE_TRANSIENT
    );
    sqlite3_bind_text(
        stmt, 2, description.c_str(), -1, SQLITE_TRANSIENT
    );
    sqlite3_bind_text(
        stmt, 3, deadline.c_str(), -1, SQLITE_TRANSIENT
    );
    sqlite3_bind_double(stmt, 4, maxMark);
    sqlite3_bind_int(stmt, 5, maxAttempts);
    sqlite3_bind_int(stmt, 6, courseId);
    sqlite3_bind_int(stmt, 7, lecturerId);

    int result = sqlite3_step(stmt);
    bool success =
        result == SQLITE_DONE &&
        sqlite3_changes(db) == 1;

    if (!success) {
        std::cerr << "Assignment creation failed or the lecturer "
                     "is not assigned to this course: "
                  << sqlite3_errmsg(db) << '\n';
        sqlite3_finalize(stmt);
        execute("ROLLBACK;");
        return false;
    }

    const int assignmentId = static_cast<int>(sqlite3_last_insert_rowid(db));
    sqlite3_finalize(stmt);

    std::string auditError;
    const std::string details =
        "course_id=" + std::to_string(courseId) +
        "; max_mark=" + std::to_string(maxMark) +
        "; deadline=" + deadline +
        "; max_attempts=" + std::to_string(maxAttempts);
    if (!writeAuditEvent(
            lecturerId, "ASSIGNMENT_CREATED", "ASSIGNMENT", assignmentId,
            "", title, "", details, auditError)) {
        std::cerr << "Could not record assignment audit event: "
                  << auditError << '\n';
        execute("ROLLBACK;");
        return false;
    }
    if (!execute("COMMIT;")) {
        execute("ROLLBACK;");
        std::cerr << "Could not commit assignment creation.\n";
        return false;
    }
    return true;
}

bool Database::getLecturerAssignments(
    int lecturerId,
    std::vector<AssignmentRecord>& assignments
) {
    assignments.clear();

    if (db == nullptr || lecturerId <= 0) {
        return false;
    }

    const char* sql = R"SQL(
        SELECT a.assignment_id,
               c.course_id,
               c.course_code,
               c.name,
               a.title,
               a.description,
               a.deadline,
               a.max_mark,
               a.max_attempts
        FROM assignments a
        JOIN courses c
          ON c.course_id = a.course_id
        JOIN course_lecturers cl
          ON cl.course_id = c.course_id
        WHERE cl.lecturer_id = ?
        ORDER BY a.deadline, c.course_code;
    )SQL";

    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Could not retrieve lecturer assignments: "
                  << sqlite3_errmsg(db) << '\n';
        return false;
    }

    sqlite3_bind_int(stmt, 1, lecturerId);

    int result;
    while ((result = sqlite3_step(stmt)) == SQLITE_ROW) {
        AssignmentRecord a{};

        a.assignmentId = sqlite3_column_int(stmt, 0);
        a.courseId = sqlite3_column_int(stmt, 1);

        const auto* code = sqlite3_column_text(stmt, 2);
        const auto* course = sqlite3_column_text(stmt, 3);
        const auto* title = sqlite3_column_text(stmt, 4);
        const auto* description = sqlite3_column_text(stmt, 5);
        const auto* deadline = sqlite3_column_text(stmt, 6);

        a.courseCode = code
            ? reinterpret_cast<const char*>(code) : "";
        a.courseName = course
            ? reinterpret_cast<const char*>(course) : "";
        a.title = title
            ? reinterpret_cast<const char*>(title) : "";
        a.description = description
            ? reinterpret_cast<const char*>(description) : "";
        a.deadline = deadline
            ? reinterpret_cast<const char*>(deadline) : "";
        a.maxMark = sqlite3_column_double(stmt, 7);
        a.maxAttempts = sqlite3_column_int(stmt, 8);

        assignments.push_back(a);
    }

    bool success = (result == SQLITE_DONE);

    if (!success) {
        std::cerr << "Assignment query failed: "
                  << sqlite3_errmsg(db) << '\n';
    }

    sqlite3_finalize(stmt);
    return success;
}

bool Database::getAssignmentStudents(
    int lecturerId,
    int assignmentId,
    std::vector<UserRecord>& students,
    std::string& error
) {
    students.clear();
    error.clear();

    if (db == nullptr || lecturerId <= 0 || assignmentId <= 0) {
        error = "Invalid lecturer or assignment selection.";
        return false;
    }

    const char* sql = R"SQL(
        SELECT u.user_id, u.username, u.role, u.active
        FROM assignments a
        JOIN course_lecturers cl ON cl.course_id = a.course_id
        JOIN enrolments e ON e.course_id = a.course_id
        JOIN users u ON u.user_id = e.student_id
        WHERE a.assignment_id = ?
          AND cl.lecturer_id = ?
          AND u.role = 'STUDENT'
          AND u.active = 1
          AND NOT EXISTS (
              SELECT 1
              FROM submissions s
              WHERE s.assignment_id = a.assignment_id
                AND s.student_id = u.user_id
          )
        ORDER BY u.username COLLATE NOCASE;
    )SQL";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        error = sqlite3_errmsg(db);
        return false;
    }

    sqlite3_bind_int(stmt, 1, assignmentId);
    sqlite3_bind_int(stmt, 2, lecturerId);

    int rc = SQLITE_DONE;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        UserRecord student{};
        student.userId = sqlite3_column_int(stmt, 0);
        const auto* username = sqlite3_column_text(stmt, 1);
        const auto* role = sqlite3_column_text(stmt, 2);
        student.username = username
            ? reinterpret_cast<const char*>(username) : "";
        student.role = role ? reinterpret_cast<const char*>(role) : "";
        student.active = sqlite3_column_int(stmt, 3) != 0;
        students.push_back(student);
    }

    if (rc != SQLITE_DONE) {
        error = sqlite3_errmsg(db);
    }
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

bool Database::grantAssignmentExtension(
    int lecturerId,
    int assignmentId,
    int studentId,
    const std::string& revisedDeadline,
    const std::string& reason,
    std::string& error
) {
    error.clear();
    if (db == nullptr || lecturerId <= 0 || assignmentId <= 0 ||
        studentId <= 0 || revisedDeadline.empty() || reason.empty()) {
        error = "Complete the assignment, student, revised deadline, and reason.";
        return false;
    }

    if (!execute("BEGIN IMMEDIATE TRANSACTION;")) {
        error = "Could not start a transaction to grant the extension.";
        return false;
    }

    const auto abort = [this, &error](const std::string& message) {
        execute("ROLLBACK;");
        error = message;
        return false;
    };

    const char* validationSql = R"SQL(
        SELECT MAX(
                   a.deadline,
                   COALESCE(ce.revised_deadline, a.deadline),
                   COALESCE(e.revised_deadline, a.deadline)
               ),
               EXISTS (
                   SELECT 1
                   FROM enrolments e
                   JOIN users u ON u.user_id = e.student_id
                   WHERE e.course_id = a.course_id
                     AND e.student_id = ?
                     AND u.role = 'STUDENT'
                     AND u.active = 1
               ),
               EXISTS (
                   SELECT 1 FROM submissions s
                   WHERE s.assignment_id = a.assignment_id
                     AND s.student_id = ?
               )
        FROM assignments a
        JOIN course_lecturers cl ON cl.course_id = a.course_id
        LEFT JOIN extensions e
            ON e.assignment_id = a.assignment_id
           AND e.student_id = ?
        LEFT JOIN assignment_class_extensions ce
            ON ce.assignment_id = a.assignment_id
        WHERE a.assignment_id = ?
          AND cl.lecturer_id = ?;
    )SQL";

    sqlite3_stmt* validation = nullptr;
    if (sqlite3_prepare_v2(
            db, validationSql, -1, &validation, nullptr) != SQLITE_OK) {
        return abort(sqlite3_errmsg(db));
    }

    sqlite3_bind_int(validation, 1, studentId);
    sqlite3_bind_int(validation, 2, studentId);
    sqlite3_bind_int(validation, 3, studentId);
    sqlite3_bind_int(validation, 4, assignmentId);
    sqlite3_bind_int(validation, 5, lecturerId);

    const int validationRc = sqlite3_step(validation);
    if (validationRc != SQLITE_ROW) {
        const std::string validationError = validationRc == SQLITE_DONE
            ? "This assignment is not assigned to the lecturer."
            : sqlite3_errmsg(db);
        sqlite3_finalize(validation);
        return abort(validationError);
    }

    const auto* deadlineText = sqlite3_column_text(validation, 0);
    const std::string currentDeadline = deadlineText
        ? reinterpret_cast<const char*>(deadlineText) : "";
    const bool isEnrolled = sqlite3_column_int(validation, 1) != 0;
    const bool hasSubmitted = sqlite3_column_int(validation, 2) != 0;
    sqlite3_finalize(validation);

    if (!isEnrolled) {
        return abort(
            "The selected student is not enrolled in this assignment's course.");
    }
    if (hasSubmitted) {
        return abort(
            "An extension cannot be granted after the student has submitted.");
    }

    const char* deadlineCheckSql = R"SQL(
        SELECT datetime(?) IS NOT NULL,
               datetime(?) > datetime(?),
               datetime(?) > datetime('now');
    )SQL";
    sqlite3_stmt* deadlineCheck = nullptr;
    if (sqlite3_prepare_v2(
            db, deadlineCheckSql, -1, &deadlineCheck, nullptr) != SQLITE_OK) {
        return abort(sqlite3_errmsg(db));
    }
    sqlite3_bind_text(
        deadlineCheck, 1, revisedDeadline.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(
        deadlineCheck, 2, revisedDeadline.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(
        deadlineCheck, 3, currentDeadline.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(
        deadlineCheck, 4, revisedDeadline.c_str(), -1, SQLITE_TRANSIENT);

    const int deadlineRc = sqlite3_step(deadlineCheck);
    const bool validDateTime = deadlineRc == SQLITE_ROW &&
        sqlite3_column_int(deadlineCheck, 0) != 0;
    const bool extendsDeadline = deadlineRc == SQLITE_ROW &&
        sqlite3_column_int(deadlineCheck, 1) != 0;
    const bool isFuture = deadlineRc == SQLITE_ROW &&
        sqlite3_column_int(deadlineCheck, 2) != 0;
    sqlite3_finalize(deadlineCheck);

    if (deadlineRc != SQLITE_ROW) {
        return abort(sqlite3_errmsg(db));
    }
    if (!validDateTime) {
        return abort("Enter a valid revised deadline.");
    }
    if (!extendsDeadline) {
        return abort(
            "The revised deadline must be later than the current deadline.");
    }
    if (!isFuture) {
        return abort("The revised deadline must be in the future.");
    }

    const char* upsertSql = R"SQL(
        INSERT INTO extensions (
            assignment_id, student_id, revised_deadline, granted_by, reason
        )
        VALUES (?, ?, ?, ?, ?)
        ON CONFLICT (assignment_id, student_id) DO UPDATE SET
            revised_deadline = excluded.revised_deadline,
            granted_by = excluded.granted_by,
            reason = excluded.reason;
    )SQL";

    sqlite3_stmt* upsert = nullptr;
    if (sqlite3_prepare_v2(db, upsertSql, -1, &upsert, nullptr) != SQLITE_OK) {
        return abort(sqlite3_errmsg(db));
    }

    sqlite3_bind_int(upsert, 1, assignmentId);
    sqlite3_bind_int(upsert, 2, studentId);
    sqlite3_bind_text(
        upsert, 3, revisedDeadline.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(upsert, 4, lecturerId);
    sqlite3_bind_text(upsert, 5, reason.c_str(), -1, SQLITE_TRANSIENT);

    const int upsertRc = sqlite3_step(upsert);
    const std::string upsertError = upsertRc == SQLITE_DONE
        ? std::string() : sqlite3_errmsg(db);
    if (upsertRc != SQLITE_DONE) {
        sqlite3_finalize(upsert);
        return abort(upsertError);
    }
    sqlite3_finalize(upsert);
    const std::string details =
        "student_id=" + std::to_string(studentId);
    if (!writeAuditEvent(
            lecturerId, "DEADLINE_EXTENSION_GRANTED", "ASSIGNMENT",
            assignmentId, currentDeadline, revisedDeadline, reason,
            details, error)) {
        return abort(error);
    }
    if (!execute("COMMIT;")) {
        execute("ROLLBACK;");
        error = "Could not commit the individual deadline extension.";
        return false;
    }
    return true;
}

bool Database::getLecturerSubmissions(
    int lecturerId,
    std::vector<SubmissionRecord>& submissions
) {
    submissions.clear();

    if (db == nullptr || lecturerId <= 0) {
        return false;
    }

    const char* sql = R"SQL(
        SELECT s.submission_id,
               s.assignment_id,
               s.student_id,
               s.version_number,
               u.username,
               c.course_code,
               a.title,
               s.submitted_at,
               s.status,
               s.file_reference,
               m.mark_id,
               m.raw_mark,
               m.final_mark,
               m.feedback,
               (
                   SELECT mr.decision
                   FROM moderation_records mr
                   WHERE mr.mark_id = m.mark_id
                   ORDER BY mr.decided_at DESC, mr.moderation_id DESC
                   LIMIT 1
               ),
               (
                   SELECT mr.comment
                   FROM moderation_records mr
                   WHERE mr.mark_id = m.mark_id
                   ORDER BY mr.decided_at DESC, mr.moderation_id DESC
                   LIMIT 1
               ),
               s.version_number = (
                   SELECT MAX(s2.version_number)
                   FROM submissions s2
                   WHERE s2.assignment_id = s.assignment_id
                     AND s2.student_id = s.student_id
               )
        FROM submissions s
        JOIN assignments a
          ON a.assignment_id = s.assignment_id
        JOIN courses c
          ON c.course_id = a.course_id
        JOIN course_lecturers cl
          ON cl.course_id = c.course_id
        JOIN users u
          ON u.user_id = s.student_id
        LEFT JOIN marks m
          ON m.submission_id = s.submission_id
        WHERE cl.lecturer_id = ?
        ORDER BY a.deadline, u.username, s.version_number DESC;
    )SQL";

    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Could not retrieve lecturer submissions: "
                  << sqlite3_errmsg(db) << '\n';
        return false;
    }

    sqlite3_bind_int(stmt, 1, lecturerId);

    int result;
    while ((result = sqlite3_step(stmt)) == SQLITE_ROW) {
        SubmissionRecord s{};

        s.submissionId = sqlite3_column_int(stmt, 0);
        s.assignmentId = sqlite3_column_int(stmt, 1);
        s.studentId = sqlite3_column_int(stmt, 2);
        s.versionNumber = sqlite3_column_int(stmt, 3);

        const auto* username = sqlite3_column_text(stmt, 4);
        const auto* code = sqlite3_column_text(stmt, 5);
        const auto* title = sqlite3_column_text(stmt, 6);
        const auto* submittedAt = sqlite3_column_text(stmt, 7);
        const auto* status = sqlite3_column_text(stmt, 8);
        const auto* file = sqlite3_column_text(stmt, 9);
        const auto* feedback = sqlite3_column_text(stmt, 13);
        const auto* latestDecision = sqlite3_column_text(stmt, 14);
        const auto* moderationComment = sqlite3_column_text(stmt, 15);

        s.studentUsername = username
            ? reinterpret_cast<const char*>(username) : "";
        s.courseCode = code
            ? reinterpret_cast<const char*>(code) : "";
        s.assignmentTitle = title
            ? reinterpret_cast<const char*>(title) : "";
        s.submittedAt = submittedAt
            ? reinterpret_cast<const char*>(submittedAt) : "";
        s.status = status
            ? reinterpret_cast<const char*>(status) : "";
        s.fileReference = file
            ? reinterpret_cast<const char*>(file) : "";

        s.hasMark = sqlite3_column_type(stmt, 10) != SQLITE_NULL;
        s.rawMark = s.hasMark
            ? sqlite3_column_double(stmt, 11) : 0.0;
        s.finalMark = s.hasMark
            ? sqlite3_column_double(stmt, 12) : 0.0;
        s.feedback = feedback
            ? reinterpret_cast<const char*>(feedback) : "";
        s.latestDecision = latestDecision
            ? reinterpret_cast<const char*>(latestDecision) : "";
        s.moderationComment = moderationComment
            ? reinterpret_cast<const char*>(moderationComment) : "";
        s.isCurrentSubmission = sqlite3_column_int(stmt, 16) != 0;

        submissions.push_back(s);
    }

    bool success = (result == SQLITE_DONE);

    if (!success) {
        std::cerr << "Submission query failed: "
                  << sqlite3_errmsg(db) << '\n';
    }

    sqlite3_finalize(stmt);
    return success;
}

bool Database::grantClassAssignmentExtension(
    int lecturerId,
    int assignmentId,
    const std::string& revisedDeadline,
    const std::string& reason,
    std::string& error
) {
    error.clear();
    if (db == nullptr || lecturerId <= 0 || assignmentId <= 0 ||
        revisedDeadline.empty() || reason.empty()) {
        error = "Complete the assignment, revised deadline, and reason.";
        return false;
    }

    if (!execute("BEGIN IMMEDIATE TRANSACTION;")) {
        error = "Could not start a transaction to grant the class extension.";
        return false;
    }

    const auto abort = [this, &error](const std::string& message) {
        execute("ROLLBACK;");
        error = message;
        return false;
    };

    const char* validationSql = R"SQL(
        SELECT MAX(a.deadline, COALESCE(ce.revised_deadline, a.deadline))
        FROM assignments a
        JOIN course_lecturers cl ON cl.course_id = a.course_id
        LEFT JOIN assignment_class_extensions ce
            ON ce.assignment_id = a.assignment_id
        WHERE a.assignment_id = ?
          AND cl.lecturer_id = ?;
    )SQL";

    sqlite3_stmt* validation = nullptr;
    if (sqlite3_prepare_v2(
            db, validationSql, -1, &validation, nullptr) != SQLITE_OK) {
        return abort(sqlite3_errmsg(db));
    }
    sqlite3_bind_int(validation, 1, assignmentId);
    sqlite3_bind_int(validation, 2, lecturerId);

    const int validationRc = sqlite3_step(validation);
    if (validationRc != SQLITE_ROW ||
        sqlite3_column_type(validation, 0) == SQLITE_NULL) {
        const std::string validationError = validationRc == SQLITE_DONE
            ? "This assignment is not assigned to the lecturer."
            : sqlite3_errmsg(db);
        sqlite3_finalize(validation);
        return abort(validationError);
    }

    const auto* deadlineText = sqlite3_column_text(validation, 0);
    const std::string currentDeadline = deadlineText
        ? reinterpret_cast<const char*>(deadlineText) : "";
    sqlite3_finalize(validation);

    const char* deadlineCheckSql = R"SQL(
        SELECT datetime(?) IS NOT NULL,
               datetime(?) > datetime(?),
               datetime(?) > datetime('now');
    )SQL";
    sqlite3_stmt* deadlineCheck = nullptr;
    if (sqlite3_prepare_v2(
            db, deadlineCheckSql, -1, &deadlineCheck, nullptr) != SQLITE_OK) {
        return abort(sqlite3_errmsg(db));
    }
    sqlite3_bind_text(
        deadlineCheck, 1, revisedDeadline.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(
        deadlineCheck, 2, revisedDeadline.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(
        deadlineCheck, 3, currentDeadline.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(
        deadlineCheck, 4, revisedDeadline.c_str(), -1, SQLITE_TRANSIENT);

    const int deadlineRc = sqlite3_step(deadlineCheck);
    const bool validDateTime = deadlineRc == SQLITE_ROW &&
        sqlite3_column_int(deadlineCheck, 0) != 0;
    const bool extendsDeadline = deadlineRc == SQLITE_ROW &&
        sqlite3_column_int(deadlineCheck, 1) != 0;
    const bool isFuture = deadlineRc == SQLITE_ROW &&
        sqlite3_column_int(deadlineCheck, 2) != 0;
    sqlite3_finalize(deadlineCheck);

    if (deadlineRc != SQLITE_ROW) {
        return abort(sqlite3_errmsg(db));
    }
    if (!validDateTime) {
        return abort("Enter a valid revised deadline.");
    }
    if (!extendsDeadline) {
        return abort(
            "The revised deadline must be later than the current deadline.");
    }
    if (!isFuture) {
        return abort("The revised deadline must be in the future.");
    }

    const char* upsertSql = R"SQL(
        INSERT INTO assignment_class_extensions (
            assignment_id, revised_deadline, granted_by, reason
        )
        VALUES (?, ?, ?, ?)
        ON CONFLICT (assignment_id) DO UPDATE SET
            revised_deadline = excluded.revised_deadline,
            granted_by = excluded.granted_by,
            reason = excluded.reason,
            granted_at = CURRENT_TIMESTAMP;
    )SQL";
    sqlite3_stmt* upsert = nullptr;
    if (sqlite3_prepare_v2(
            db, upsertSql, -1, &upsert, nullptr) != SQLITE_OK) {
        return abort(sqlite3_errmsg(db));
    }
    sqlite3_bind_int(upsert, 1, assignmentId);
    sqlite3_bind_text(
        upsert, 2, revisedDeadline.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(upsert, 3, lecturerId);
    sqlite3_bind_text(upsert, 4, reason.c_str(), -1, SQLITE_TRANSIENT);

    const int upsertRc = sqlite3_step(upsert);
    const std::string upsertError = upsertRc == SQLITE_DONE
        ? std::string() : sqlite3_errmsg(db);
    sqlite3_finalize(upsert);
    if (upsertRc != SQLITE_DONE) {
        return abort(upsertError);
    }

    if (!writeAuditEvent(
            lecturerId, "DEADLINE_EXTENSION_GRANTED", "ASSIGNMENT",
            assignmentId, currentDeadline, revisedDeadline, reason,
            "scope=CLASS", error)) {
        return abort(error);
    }
    if (!execute("COMMIT;")) {
        execute("ROLLBACK;");
        error = "Could not commit the class deadline extension.";
        return false;
    }
    return true;
}

bool Database::getHodCourses(
    int hodId, std::vector<CourseRecord>& courses)
{
    courses.clear();
    if (!db) return false;

    const char* sql =
        "SELECT c.course_id, c.course_code, c.name, c.department_id, d.name "
        "FROM courses c "
        "JOIN departments d ON d.department_id = c.department_id "
        "WHERE d.hod_user_id = ? ORDER BY c.course_code;";

    sqlite3_stmt* st = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &st, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_int(st, 1, hodId);
    int rc;
    while ((rc = sqlite3_step(st)) == SQLITE_ROW) {
        CourseRecord r{};
        r.courseId = sqlite3_column_int(st, 0);
        r.courseCode = reinterpret_cast<const char*>(sqlite3_column_text(st, 1));
        r.name = reinterpret_cast<const char*>(sqlite3_column_text(st, 2));
        r.departmentId = sqlite3_column_int(st, 3);
        r.departmentName = reinterpret_cast<const char*>(sqlite3_column_text(st, 4));
        courses.push_back(r);
    }
    sqlite3_finalize(st);
    return rc == SQLITE_DONE;
}

bool Database::getHodMarks(
    int hodId, std::vector<HodMarkRecord>& marks)
{
    marks.clear();
    if (!db) return false;

    const char* sql =
        "SELECT m.mark_id, s.submission_id, u.username, c.course_code, "
        "a.title, m.raw_mark, m.final_mark, m.feedback, s.status, "
        "COALESCE(mr.decision, ''), COALESCE(mr.comment, '') "
        "FROM marks m "
        "JOIN submissions s ON s.submission_id = m.submission_id "
        "JOIN assignments a ON a.assignment_id = s.assignment_id "
        "JOIN courses c ON c.course_id = a.course_id "
        "JOIN departments d ON d.department_id = c.department_id "
        "JOIN users u ON u.user_id = s.student_id "
        "LEFT JOIN moderation_records mr ON mr.moderation_id = ("
        "  SELECT mr2.moderation_id FROM moderation_records mr2 "
        "  WHERE mr2.mark_id = m.mark_id "
        "  ORDER BY mr2.decided_at DESC, mr2.moderation_id DESC LIMIT 1"
        ") "
        "WHERE d.hod_user_id = ? "
        "AND s.version_number = ("
        "  SELECT MAX(s2.version_number) FROM submissions s2 "
        "  WHERE s2.assignment_id = s.assignment_id "
        "    AND s2.student_id = s.student_id"
        ") "
        "ORDER BY c.course_code, a.title, u.username;";

    sqlite3_stmt* st = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &st, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_int(st, 1, hodId);
    int rc;
    while ((rc = sqlite3_step(st)) == SQLITE_ROW) {
        auto str = [st](int col) -> std::string {
            const auto* p = sqlite3_column_text(st, col);
            return p ? reinterpret_cast<const char*>(p) : "";
        };

        HodMarkRecord r{};
        r.markId = sqlite3_column_int(st, 0);
        r.submissionId = sqlite3_column_int(st, 1);
        r.studentUsername = str(2);
        r.courseCode = str(3);
        r.assignmentTitle = str(4);
        r.rawMark = sqlite3_column_double(st, 5);
        r.finalMark = sqlite3_column_double(st, 6);
        r.feedback = str(7);
        r.submissionStatus = str(8);
        r.latestDecision = str(9);
        r.moderationComment = str(10);
        marks.push_back(r);
    }
    sqlite3_finalize(st);
    return rc == SQLITE_DONE;
}

bool Database::moderateMark(
    int hodId, int markId, const std::string& decision,
    const std::string& comment)
{
    if (!db || markId <= 0 || comment.empty() ||
        (decision != "APPROVED" && decision != "RETURNED"))
        return false;

    if (!execute("BEGIN IMMEDIATE TRANSACTION;"))
        return false;

    const char* checkSql =
        "SELECT s.submission_id, s.status "
        "FROM marks m "
        "JOIN submissions s ON s.submission_id = m.submission_id "
        "JOIN assignments a ON a.assignment_id = s.assignment_id "
        "JOIN courses c ON c.course_id = a.course_id "
        "JOIN departments d ON d.department_id = c.department_id "
        "WHERE m.mark_id = ? AND d.hod_user_id = ? "
        "AND s.version_number = ("
        "  SELECT MAX(s2.version_number) FROM submissions s2 "
        "  WHERE s2.assignment_id = s.assignment_id "
        "    AND s2.student_id = s.student_id"
        ");";

    sqlite3_stmt* check = nullptr;
    if (sqlite3_prepare_v2(db, checkSql, -1, &check, nullptr) != SQLITE_OK) {
        execute("ROLLBACK;");
        return false;
    }
    sqlite3_bind_int(check, 1, markId);
    sqlite3_bind_int(check, 2, hodId);
    const int checkRc = sqlite3_step(check);
    const int moderatedSubmissionId = checkRc == SQLITE_ROW
        ? sqlite3_column_int(check, 0) : 0;
    const auto* statusText = checkRc == SQLITE_ROW
        ? sqlite3_column_text(check, 1) : nullptr;
    const std::string status = statusText
        ? reinterpret_cast<const char*>(statusText) : "";
    sqlite3_finalize(check);

    if (checkRc != SQLITE_ROW || status == "RELEASED") {
        execute("ROLLBACK;");
        return false;
    }

    const char* insertSql =
        "INSERT INTO moderation_records(mark_id, hod_id, decision, comment) "
        "VALUES (?, ?, ?, ?);";
    sqlite3_stmt* insert = nullptr;
    if (sqlite3_prepare_v2(db, insertSql, -1, &insert, nullptr) != SQLITE_OK) {
        execute("ROLLBACK;");
        return false;
    }
    sqlite3_bind_int(insert, 1, markId);
    sqlite3_bind_int(insert, 2, hodId);
    sqlite3_bind_text(insert, 3, decision.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(insert, 4, comment.c_str(), -1, SQLITE_TRANSIENT);
    const int insertRc = sqlite3_step(insert);
    sqlite3_finalize(insert);
    if (insertRc != SQLITE_DONE) {
        execute("ROLLBACK;");
        return false;
    }

    const char* updateSql =
        "UPDATE submissions SET status = ? "
        "WHERE submission_id = (SELECT submission_id FROM marks WHERE mark_id = ?) "
        "AND status <> 'RELEASED';";
    sqlite3_stmt* update = nullptr;
    if (sqlite3_prepare_v2(db, updateSql, -1, &update, nullptr) != SQLITE_OK) {
        execute("ROLLBACK;");
        return false;
    }
    const char* newStatus = decision == "APPROVED" ? "MODERATED" : "MARKED";
    sqlite3_bind_text(update, 1, newStatus, -1, SQLITE_STATIC);
    sqlite3_bind_int(update, 2, markId);
    const int updateRc = sqlite3_step(update);
    const int changed = sqlite3_changes(db);
    sqlite3_finalize(update);

    if (updateRc != SQLITE_DONE || changed != 1) {
        execute("ROLLBACK;");
        return false;
    }
    std::string auditError;
    if (!writeAuditEvent(
            hodId,
            decision == "APPROVED" ? "MARK_APPROVED" : "MARK_RETURNED",
            "MARK",
            markId,
            status,
            newStatus,
            comment,
            "submission_id=" + std::to_string(moderatedSubmissionId),
            auditError)) {
        execute("ROLLBACK;");
        return false;
    }
    if (!execute("COMMIT;")) {
        execute("ROLLBACK;");
        return false;
    }
    return true;
}

bool Database::releaseApprovedMarks(int hodId, int& releasedCount)
{
    releasedCount = 0;
    if (!db) return false;

    if (!execute("BEGIN IMMEDIATE TRANSACTION;"))
        return false;

    const char* sql =
        "UPDATE submissions SET status = 'RELEASED' "
        "WHERE status = 'MODERATED' "
        "AND submission_id IN ("
        "  SELECT s.submission_id FROM submissions s "
        "  JOIN marks m ON m.submission_id = s.submission_id "
        "  JOIN assignments a ON a.assignment_id = s.assignment_id "
        "  JOIN courses c ON c.course_id = a.course_id "
        "  JOIN departments d ON d.department_id = c.department_id "
        "  JOIN moderation_records mr ON mr.moderation_id = ("
        "    SELECT mr2.moderation_id FROM moderation_records mr2 "
        "    WHERE mr2.mark_id = m.mark_id "
        "    ORDER BY mr2.decided_at DESC, mr2.moderation_id DESC LIMIT 1"
        "  ) "
        "  WHERE d.hod_user_id = ? AND mr.decision = 'APPROVED' "
        "    AND s.version_number = ("
        "      SELECT MAX(s2.version_number) FROM submissions s2 "
        "      WHERE s2.assignment_id = s.assignment_id "
        "        AND s2.student_id = s.student_id"
        "    )"
        ");";

    sqlite3_stmt* st = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &st, nullptr) != SQLITE_OK) {
        execute("ROLLBACK;");
        return false;
    }
    sqlite3_bind_int(st, 1, hodId);
    const int rc = sqlite3_step(st);
    if (rc == SQLITE_DONE)
        releasedCount = sqlite3_changes(db);
    sqlite3_finalize(st);

    if (rc != SQLITE_DONE) {
        execute("ROLLBACK;");
        return false;
    }
    if (releasedCount > 0) {
        std::string auditError;
        if (!writeAuditEvent(
                hodId, "APPROVED_MARKS_RELEASED", "MARK", 0,
                "MODERATED", "RELEASED", "",
                "released_count=" + std::to_string(releasedCount),
                auditError)) {
            execute("ROLLBACK;");
            releasedCount = 0;
            return false;
        }
    }
    if (!execute("COMMIT;")) {
        execute("ROLLBACK;");
        releasedCount = 0;
        return false;
    }
    return true;
}

bool Database::getStudentCourses(
    int studentId,
    std::vector<CourseRecord>& courses
) {
    courses.clear();

    if (db == nullptr || studentId <= 0)
        return false;

    const char* sql = R"SQL(
        SELECT c.course_id, c.course_code, c.name,
               d.department_id, d.name
        FROM enrolments e
        JOIN courses c ON c.course_id = e.course_id
        JOIN departments d ON d.department_id = c.department_id
        WHERE e.student_id = ?
        ORDER BY c.course_code;
    )SQL";

    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr)
        != SQLITE_OK) {
        std::cerr << "Student course query failed: "
                  << sqlite3_errmsg(db) << '\n';
        return false;
    }

    sqlite3_bind_int(stmt, 1, studentId);

    int rc;

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        CourseRecord r{};

        r.courseId = sqlite3_column_int(stmt, 0);

        auto str = [stmt](int col) -> std::string {
            const auto* p = sqlite3_column_text(stmt, col);
            return p
                ? reinterpret_cast<const char*>(p)
                : "";
        };

        r.courseCode = str(1);
        r.name = str(2);
        r.departmentId = sqlite3_column_int(stmt, 3);
        r.departmentName = str(4);

        courses.push_back(r);
    }

    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        std::cerr << "Student course query failed: "
                  << sqlite3_errmsg(db) << '\n';
        return false;
    }

    return true;
}


bool Database::getStudentAssignments(
    int studentId,
    std::vector<StudentAssignmentRecord>& assignments
) {
    assignments.clear();

    if (db == nullptr || studentId <= 0)
        return false;

    const char* sql = R"SQL(
        SELECT
            a.assignment_id,
            c.course_id,
            c.course_code,
            c.name,
            a.title,
            a.description,
            MAX(
                a.deadline,
                COALESCE(ce.revised_deadline, a.deadline),
                COALESCE(e.revised_deadline, a.deadline)
            ),
            CASE
                WHEN ce.assignment_id IS NOT NULL
                     AND e.extension_id IS NOT NULL
                    THEN 'Class + individual'
                WHEN ce.assignment_id IS NOT NULL
                    THEN 'Class extension'
                WHEN e.extension_id IS NOT NULL
                    THEN 'Individual extension'
                ELSE 'Standard'
            END,
            a.max_mark,
            a.max_attempts,
            COALESCE((
                SELECT s.version_number
                FROM submissions s
                WHERE s.assignment_id = a.assignment_id
                  AND s.student_id = ?
                ORDER BY s.version_number DESC
                LIMIT 1
            ), 0),
            COALESCE((
                SELECT s.status
                FROM submissions s
                WHERE s.assignment_id = a.assignment_id
                  AND s.student_id = ?
                ORDER BY s.version_number DESC
                LIMIT 1
            ), 'NOT SUBMITTED')
        FROM assignments a
        JOIN courses c ON c.course_id = a.course_id
        JOIN enrolments en ON en.course_id = c.course_id
        LEFT JOIN extensions e
            ON e.assignment_id = a.assignment_id
           AND e.student_id = en.student_id
        LEFT JOIN assignment_class_extensions ce
            ON ce.assignment_id = a.assignment_id
        WHERE en.student_id = ?
        ORDER BY MAX(
                     a.deadline,
                     COALESCE(ce.revised_deadline, a.deadline),
                     COALESCE(e.revised_deadline, a.deadline)
                 ),
                 c.course_code, a.title;
    )SQL";

    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr)
        != SQLITE_OK) {
        std::cerr << "Student assignment query failed: "
                  << sqlite3_errmsg(db) << '\n';
        return false;
    }

    sqlite3_bind_int(stmt, 1, studentId);
    sqlite3_bind_int(stmt, 2, studentId);
    sqlite3_bind_int(stmt, 3, studentId);

    int rc;

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        StudentAssignmentRecord r{};

        auto str = [stmt](int col) -> std::string {
            const auto* p = sqlite3_column_text(stmt, col);
            return p
                ? reinterpret_cast<const char*>(p)
                : "";
        };

        r.assignmentId = sqlite3_column_int(stmt, 0);
        r.courseId = sqlite3_column_int(stmt, 1);
        r.courseCode = str(2);
        r.courseName = str(3);
        r.title = str(4);
        r.description = str(5);
        r.deadline = str(6);
        r.deadlineExtension = str(7);
        r.maxMark = sqlite3_column_double(stmt, 8);
        r.maxAttempts = sqlite3_column_int(stmt, 9);
        r.latestVersion = sqlite3_column_int(stmt, 10);
        r.latestStatus = str(11);

        assignments.push_back(r);
    }

    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        std::cerr << "Student assignment query failed: "
                  << sqlite3_errmsg(db) << '\n';
        return false;
    }

    return true;
}


bool Database::getStudentSubmissions(
    int studentId,
    std::vector<SubmissionRecord>& submissions
) {
    submissions.clear();

    if (db == nullptr || studentId <= 0)
        return false;

    const char* sql = R"SQL(
        SELECT
            s.submission_id,
            s.assignment_id,
            s.student_id,
            s.version_number,
            u.username,
            c.course_code,
            a.title,
            s.submitted_at,
            s.status,
            s.file_reference,

            CASE WHEN s.status = 'RELEASED'
                 THEN m.mark_id ELSE NULL END,

            CASE WHEN s.status = 'RELEASED'
                 THEN m.raw_mark ELSE NULL END,

            CASE WHEN s.status = 'RELEASED'
                 THEN m.final_mark ELSE NULL END,

            CASE WHEN s.status = 'RELEASED'
                 THEN m.feedback ELSE NULL END,

            CASE
                WHEN s.version_number = (
                    SELECT MAX(s2.version_number)
                    FROM submissions s2
                    WHERE s2.assignment_id = s.assignment_id
                      AND s2.student_id = s.student_id
                )
                THEN 1
                ELSE 0
            END AS is_current_submission

        FROM submissions s

        JOIN assignments a
            ON a.assignment_id = s.assignment_id

        JOIN courses c
            ON c.course_id = a.course_id

        JOIN users u
            ON u.user_id = s.student_id

        LEFT JOIN marks m
            ON m.submission_id = s.submission_id

        WHERE s.student_id = ?

        ORDER BY s.submitted_at DESC, s.version_number DESC;
    )SQL";

    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr)
        != SQLITE_OK) {

        std::cerr << "Student submission query failed: "
                  << sqlite3_errmsg(db) << '\n';

        return false;
    }

    sqlite3_bind_int(stmt, 1, studentId);

    int rc;

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {

        SubmissionRecord r{};

        auto str = [stmt](int col) -> std::string {
            const auto* p = sqlite3_column_text(stmt, col);

            return p
                ? reinterpret_cast<const char*>(p)
                : "";
        };

        r.submissionId = sqlite3_column_int(stmt, 0);
        r.assignmentId = sqlite3_column_int(stmt, 1);
        r.studentId = sqlite3_column_int(stmt, 2);
        r.versionNumber = sqlite3_column_int(stmt, 3);

        r.studentUsername = str(4);
        r.courseCode = str(5);
        r.assignmentTitle = str(6);
        r.submittedAt = str(7);
        r.status = str(8);
        r.fileReference = str(9);

        r.hasMark =
            sqlite3_column_type(stmt, 10) != SQLITE_NULL;

        r.rawMark = r.hasMark
            ? sqlite3_column_double(stmt, 11)
            : 0.0;

        r.finalMark = r.hasMark
            ? sqlite3_column_double(stmt, 12)
            : 0.0;

        r.feedback = str(13);

        // Identify whether this is the latest submission version.
        r.isCurrentSubmission =
            sqlite3_column_int(stmt, 14) != 0;

        submissions.push_back(r);
    }

    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {

        std::cerr << "Student submission query failed: "
                  << sqlite3_errmsg(db) << '\n';

        submissions.clear();
        return false;
    }

    return true;
}


bool Database::submitAssignment(
    int studentId,
    int assignmentId,
    const std::string& fileReference
) {
    if (db == nullptr ||
        studentId <= 0 ||
        assignmentId <= 0 ||
        fileReference.empty()) {
        return false;
    }

    if (!execute("BEGIN IMMEDIATE TRANSACTION;")) {
        std::cerr << "Could not start submission transaction.\n";
        return false;
    }

    // Confirm enrolment and obtain the applicable deadline
    // and current attempt count while preventing concurrent submissions.
    const char* lookupSql = R"SQL(
        SELECT
            MAX(
                a.deadline,
                COALESCE(ce.revised_deadline, a.deadline),
                COALESCE(e.revised_deadline, a.deadline)
            ),
            a.max_attempts,
            COALESCE((
                SELECT MAX(s.version_number)
                FROM submissions s
                WHERE s.assignment_id = a.assignment_id
                  AND s.student_id = ?
            ), 0),
            (
                SELECT s.submission_id
                FROM submissions s
                WHERE s.assignment_id = a.assignment_id
                  AND s.student_id = ?
                ORDER BY s.version_number DESC
                LIMIT 1
            ),
            CASE
                WHEN datetime('now') >
                     datetime(MAX(
                         a.deadline,
                         COALESCE(ce.revised_deadline, a.deadline),
                         COALESCE(e.revised_deadline, a.deadline)
                     ))
                THEN 1
                ELSE 0
            END
        FROM assignments a
        JOIN enrolments en
            ON en.course_id = a.course_id
           AND en.student_id = ?
        LEFT JOIN extensions e
            ON e.assignment_id = a.assignment_id
           AND e.student_id = en.student_id
        LEFT JOIN assignment_class_extensions ce
            ON ce.assignment_id = a.assignment_id
        WHERE a.assignment_id = ?;
    )SQL";

    sqlite3_stmt* lookup = nullptr;

    if (sqlite3_prepare_v2(
            db, lookupSql, -1, &lookup, nullptr) != SQLITE_OK) {
        std::cerr << "Submission validation failed: "
                  << sqlite3_errmsg(db) << '\n';
        execute("ROLLBACK;");
        return false;
    }

    sqlite3_bind_int(lookup, 1, studentId);
    sqlite3_bind_int(lookup, 2, studentId);
    sqlite3_bind_int(lookup, 3, studentId);
    sqlite3_bind_int(lookup, 4, assignmentId);

    const int lookupRc = sqlite3_step(lookup);

    if (lookupRc != SQLITE_ROW) {
        sqlite3_finalize(lookup);
        std::cerr << "Assignment not found or student is not enrolled.\n";
        execute("ROLLBACK;");
        return false;
    }

    const int maxAttempts = sqlite3_column_int(lookup, 1);
    const int previousVersion = sqlite3_column_int(lookup, 2);
    const bool hasPrevious =
        sqlite3_column_type(lookup, 3) != SQLITE_NULL;
    const int previousSubmissionId =
        hasPrevious ? sqlite3_column_int(lookup, 3) : 0;
    const bool isLate = sqlite3_column_int(lookup, 4) != 0;

    sqlite3_finalize(lookup);

    if (previousVersion >= maxAttempts) {
        std::cerr << "The maximum number of assignment submissions has been reached.\n";
        execute("ROLLBACK;");
        return false;
    }
    if (hasPrevious && isLate) {
        std::cerr << "Resubmissions are not allowed after the applicable deadline.\n";
        execute("ROLLBACK;");
        return false;
    }

    const char* insertSql = R"SQL(
        INSERT INTO submissions (
            assignment_id,
            student_id,
            supersedes_submission_id,
            version_number,
            status,
            file_reference
        )
        VALUES (?, ?, ?, ?, ?, ?);
    )SQL";

    sqlite3_stmt* insert = nullptr;

    if (sqlite3_prepare_v2(
            db, insertSql, -1, &insert, nullptr) != SQLITE_OK) {
        std::cerr << "Could not prepare submission: "
                  << sqlite3_errmsg(db) << '\n';
        execute("ROLLBACK;");
        return false;
    }

    sqlite3_bind_int(insert, 1, assignmentId);
    sqlite3_bind_int(insert, 2, studentId);

    if (hasPrevious) {
        sqlite3_bind_int(insert, 3, previousSubmissionId);
    } else {
        sqlite3_bind_null(insert, 3);
    }

    sqlite3_bind_int(insert, 4, previousVersion + 1);

    const char* status = isLate ? "LATE" : "SUBMITTED";

    sqlite3_bind_text(
        insert, 5, status, -1, SQLITE_STATIC);

    sqlite3_bind_text(
        insert, 6, fileReference.c_str(), -1, SQLITE_TRANSIENT);

    const int insertRc = sqlite3_step(insert);

    if (insertRc != SQLITE_DONE) {
        std::cerr << "Submission failed: "
                  << sqlite3_errmsg(db) << '\n';
        sqlite3_finalize(insert);
        execute("ROLLBACK;");
        return false;
    }

    sqlite3_finalize(insert);

    const int submissionId =
        static_cast<int>(sqlite3_last_insert_rowid(db));
    std::string auditError;
    const std::string details =
        "assignment_id=" + std::to_string(assignmentId) +
        ";student_id=" + std::to_string(studentId) +
        ";attempt=" + std::to_string(previousVersion + 1);
    if (!writeAuditEvent(
            studentId,
            isLate ? "LATE_SUBMISSION" : "ASSIGNMENT_SUBMITTED",
            "SUBMISSION",
            submissionId,
            hasPrevious ? std::to_string(previousSubmissionId) : "none",
            status,
            "",
            details,
            auditError)) {
        std::cerr << "Could not record submission audit event: "
                  << auditError << '\n';
        execute("ROLLBACK;");
        return false;
    }

    if (!execute("COMMIT;")) {
        execute("ROLLBACK;");
        return false;
    }
    return true;
}


bool Database::getStudentMarks(
    int studentId,
    std::vector<StudentMarkRecord>& marks
) {
    marks.clear();

    if (db == nullptr || studentId <= 0)
        return false;

    const char* sql = R"SQL(
        SELECT
            c.course_code,
            a.title,
            s.version_number,
            m.raw_mark,
            m.final_mark,
            a.max_mark,
            m.feedback,
            m.marked_at
        FROM submissions s
        JOIN marks m ON m.submission_id = s.submission_id
        JOIN assignments a ON a.assignment_id = s.assignment_id
        JOIN courses c ON c.course_id = a.course_id
        WHERE s.student_id = ?
          AND s.status = 'RELEASED'
          AND s.version_number = (
              SELECT MAX(s2.version_number)
              FROM submissions s2
              WHERE s2.assignment_id = s.assignment_id
                AND s2.student_id = s.student_id
          )
        ORDER BY m.marked_at DESC, c.course_code, a.title;
    )SQL";

    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr)
        != SQLITE_OK) {
        std::cerr << "Student marks query failed: "
                  << sqlite3_errmsg(db) << '\n';
        return false;
    }

    sqlite3_bind_int(stmt, 1, studentId);

    int rc;

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        StudentMarkRecord r{};

        auto str = [stmt](int col) -> std::string {
            const auto* p = sqlite3_column_text(stmt, col);
            return p
                ? reinterpret_cast<const char*>(p)
                : "";
        };

        r.courseCode = str(0);
        r.assignmentTitle = str(1);
        r.versionNumber = sqlite3_column_int(stmt, 2);
        r.rawMark = sqlite3_column_double(stmt, 3);
        r.finalMark = sqlite3_column_double(stmt, 4);
        r.maxMark = sqlite3_column_double(stmt, 5);
        r.feedback = str(6);
        r.markedAt = str(7);

        marks.push_back(r);
    }

    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        std::cerr << "Student marks query failed: "
                  << sqlite3_errmsg(db) << '\n';
        return false;
    }

    return true;
}

bool Database::getSubmissionMarkingContext(
    int lecturerId,
    int submissionId,
    SubmissionMarkContext& context,
    std::string& error
) {
    context = {};
    error.clear();

    if (db == nullptr || lecturerId <= 0 || submissionId <= 0) {
        error = "Invalid submission context.";
        return false;
    }

    const char* sql = R"SQL(
        SELECT
            s.submission_id,
            s.assignment_id,
            a.course_id,
            MAX(
                a.deadline,
                COALESCE(ce.revised_deadline, a.deadline),
                COALESCE(e.revised_deadline, a.deadline)
            ),
            s.submitted_at,
            s.status,
            a.max_mark,
            COALESCE(m.raw_mark, 0.0),
            COALESCE(m.final_mark, 0.0),
            CASE WHEN m.mark_id IS NOT NULL THEN 1 ELSE 0 END,
            (
                SELECT mr.decision
                FROM moderation_records mr
                WHERE mr.mark_id = m.mark_id
                ORDER BY mr.decided_at DESC, mr.moderation_id DESC
                LIMIT 1
            ),
            (
                SELECT mr.comment
                FROM moderation_records mr
                WHERE mr.mark_id = m.mark_id
                ORDER BY mr.decided_at DESC, mr.moderation_id DESC
                LIMIT 1
            ),
            s.version_number = (
                SELECT MAX(s2.version_number)
                FROM submissions s2
                WHERE s2.assignment_id = s.assignment_id
                  AND s2.student_id = s.student_id
            )
        FROM submissions s
        JOIN assignments a ON a.assignment_id = s.assignment_id
        JOIN course_lecturers cl ON cl.course_id = a.course_id
        LEFT JOIN extensions e
            ON e.assignment_id = a.assignment_id
           AND e.student_id = s.student_id
        LEFT JOIN assignment_class_extensions ce
            ON ce.assignment_id = a.assignment_id
        LEFT JOIN marks m ON m.submission_id = s.submission_id
        WHERE s.submission_id = ?
          AND cl.lecturer_id = ?
        LIMIT 1;
    )SQL";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        error = sqlite3_errmsg(db);
        return false;
    }

    sqlite3_bind_int(stmt, 1, submissionId);
    sqlite3_bind_int(stmt, 2, lecturerId);

    const int rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        error = "This submission is not assigned to the lecturer or no longer exists.";
        return false;
    }

    context.submissionId = sqlite3_column_int(stmt, 0);
    context.assignmentId = sqlite3_column_int(stmt, 1);
    context.courseId = sqlite3_column_int(stmt, 2);
    context.deadline = sqlite3_column_text(stmt, 3)
        ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)) : "";
    context.submittedAt = sqlite3_column_text(stmt, 4)
        ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)) : "";
    context.status = sqlite3_column_text(stmt, 5)
        ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5)) : "";
    context.maxMark = sqlite3_column_double(stmt, 6);
    context.currentRawMark = sqlite3_column_double(stmt, 7);
    context.currentFinalMark = sqlite3_column_double(stmt, 8);
    context.hasExistingMark = sqlite3_column_int(stmt, 9) != 0;
    context.latestDecision = sqlite3_column_text(stmt, 10)
        ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 10)) : "";
    context.moderationComment = sqlite3_column_text(stmt, 11)
        ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 11)) : "";
    context.isCurrentSubmission = sqlite3_column_int(stmt, 12) != 0;
    context.isLate = context.status == "LATE" ||
        (!context.deadline.empty() && !context.submittedAt.empty() &&
         context.submittedAt > context.deadline);

    sqlite3_finalize(stmt);
    error.clear();
    return true;
}

bool Database::markSubmission(
    int lecturerId,
    int submissionId,
    double rawMark,
    double finalMark,
    const std::string& feedback,
    const std::string& reason,
    std::string& error
) {
    error.clear();

    if (db == nullptr || lecturerId <= 0 || submissionId <= 0) {
        error = "Invalid mark submission request.";
        return false;
    }

    if (feedback.empty() || reason.empty()) {
        error = "Feedback and reason are required.";
        return false;
    }

    if (rawMark < 0.0 || finalMark < 0.0) {
        error = "Marks cannot be negative.";
        return false;
    }

    if (!execute("BEGIN IMMEDIATE TRANSACTION;")) {
        error = "Could not start a database transaction for marking.";
        return false;
    }

    const char* authorizeSql = R"SQL(
        SELECT m.mark_id, s.status, a.max_mark
        FROM submissions s
        JOIN assignments a ON a.assignment_id = s.assignment_id
        JOIN course_lecturers cl ON cl.course_id = a.course_id
        LEFT JOIN marks m ON m.submission_id = s.submission_id
        WHERE s.submission_id = ?
          AND cl.lecturer_id = ?
          AND s.status <> 'RELEASED'
          AND s.version_number = (
              SELECT MAX(s2.version_number)
              FROM submissions s2
              WHERE s2.assignment_id = s.assignment_id
                AND s2.student_id = s.student_id
          );
    )SQL";

    sqlite3_stmt* authorize = nullptr;
    if (sqlite3_prepare_v2(db, authorizeSql, -1, &authorize, nullptr) != SQLITE_OK) {
        execute("ROLLBACK;");
        error = sqlite3_errmsg(db);
        return false;
    }
    sqlite3_bind_int(authorize, 1, submissionId);
    sqlite3_bind_int(authorize, 2, lecturerId);

    const int authRc = sqlite3_step(authorize);
    if (authRc != SQLITE_ROW) {
        sqlite3_finalize(authorize);
        execute("ROLLBACK;");
        error = "The lecturer is not authorised to mark this submission.";
        return false;
    }

    int existingMarkId = sqlite3_column_type(authorize, 0) != SQLITE_NULL
        ? sqlite3_column_int(authorize, 0)
        : -1;
    const bool isNewMark = existingMarkId <= 0;
    const auto* statusText = sqlite3_column_text(authorize, 1);
    const std::string existingStatus = statusText ? reinterpret_cast<const char*>(statusText) : "";
    const double maxMark = sqlite3_column_double(authorize, 2);
    sqlite3_finalize(authorize);

    if (rawMark > maxMark) {
        execute("ROLLBACK;");
        error = "The mark exceeds the assignment maximum mark.";
        return false;
    }

    double previousRawMark = 0.0;
    double previousFinalMark = 0.0;
    if (existingMarkId > 0) {
        const char* previousSql = R"SQL(
            SELECT raw_mark, final_mark
            FROM marks
            WHERE mark_id = ?;
        )SQL";
        sqlite3_stmt* previous = nullptr;
        if (sqlite3_prepare_v2(db, previousSql, -1, &previous, nullptr) != SQLITE_OK) {
            execute("ROLLBACK;");
            error = sqlite3_errmsg(db);
            return false;
        }
        sqlite3_bind_int(previous, 1, existingMarkId);
        const int previousRc = sqlite3_step(previous);
        if (previousRc != SQLITE_ROW) {
            sqlite3_finalize(previous);
            execute("ROLLBACK;");
            error = previousRc == SQLITE_DONE
                ? "The previous mark could not be found."
                : sqlite3_errmsg(db);
            return false;
        }
        previousRawMark = sqlite3_column_double(previous, 0);
        previousFinalMark = sqlite3_column_double(previous, 1);
        sqlite3_finalize(previous);
    }

    const char* sql = R"SQL(
        INSERT INTO marks (submission_id, marked_by, raw_mark, final_mark, feedback, marked_at)
        VALUES (?, ?, ?, ?, ?, datetime('now'));
    )SQL";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        execute("ROLLBACK;");
        error = sqlite3_errmsg(db);
        return false;
    }

    if (existingMarkId > 0) {
        const char* updateSql = R"SQL(
            UPDATE marks
            SET raw_mark = ?, final_mark = ?, feedback = ?, marked_at = datetime('now')
            WHERE mark_id = ?;
        )SQL";
        sqlite3_stmt* update = nullptr;
        if (sqlite3_prepare_v2(db, updateSql, -1, &update, nullptr) != SQLITE_OK) {
            sqlite3_finalize(stmt);
            execute("ROLLBACK;");
            error = sqlite3_errmsg(db);
            return false;
        }

        sqlite3_bind_double(update, 1, rawMark);
        sqlite3_bind_double(update, 2, finalMark);
        sqlite3_bind_text(update, 3, feedback.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(update, 4, existingMarkId);

        const int updateRc = sqlite3_step(update);
        sqlite3_finalize(update);

        if (updateRc != SQLITE_DONE) {
            sqlite3_finalize(stmt);
            execute("ROLLBACK;");
            error = sqlite3_errmsg(db);
            return false;
        }

        const char* auditSql = R"SQL(
            INSERT INTO mark_audit (mark_id, changed_by, old_mark, new_mark, reason, changed_at)
            VALUES (?, ?, ?, ?, ?, datetime('now'));
        )SQL";
        sqlite3_stmt* audit = nullptr;
        if (sqlite3_prepare_v2(db, auditSql, -1, &audit, nullptr) != SQLITE_OK) {
            sqlite3_finalize(stmt);
            execute("ROLLBACK;");
            error = sqlite3_errmsg(db);
            return false;
        }
        sqlite3_bind_int(audit, 1, existingMarkId);
        sqlite3_bind_int(audit, 2, lecturerId);
        sqlite3_bind_double(audit, 3, previousFinalMark);
        sqlite3_bind_double(audit, 4, finalMark);
        sqlite3_bind_text(audit, 5, reason.c_str(), -1, SQLITE_TRANSIENT);
        const int auditRc = sqlite3_step(audit);
        sqlite3_finalize(audit);
        if (auditRc != SQLITE_DONE) {
            sqlite3_finalize(stmt);
            execute("ROLLBACK;");
            error = sqlite3_errmsg(db);
            return false;
        }

        sqlite3_finalize(stmt);
    } else {
        sqlite3_bind_int(stmt, 1, submissionId);
        sqlite3_bind_int(stmt, 2, lecturerId);
        sqlite3_bind_double(stmt, 3, rawMark);
        sqlite3_bind_double(stmt, 4, finalMark);
        sqlite3_bind_text(stmt, 5, feedback.c_str(), -1, SQLITE_TRANSIENT);

        const int insertRc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        if (insertRc != SQLITE_DONE) {
            execute("ROLLBACK;");
            error = sqlite3_errmsg(db);
            return false;
        }
        existingMarkId = static_cast<int>(sqlite3_last_insert_rowid(db));
    }

    const std::string oldValue = isNewMark
        ? "none"
        : "raw=" + std::to_string(previousRawMark) +
            ";final=" + std::to_string(previousFinalMark);
    const std::string newValue =
        "raw=" + std::to_string(rawMark) +
        ";final=" + std::to_string(finalMark);
    const std::string markDetails =
        "submission_id=" + std::to_string(submissionId) +
        ";submission_status=" + existingStatus;
    if (!writeAuditEvent(
            lecturerId,
            isNewMark ? "MARK_CREATED" : "MARK_CHANGED",
            "MARK",
            existingMarkId,
            oldValue,
            newValue,
            reason,
            markDetails,
            error)) {
        execute("ROLLBACK;");
        return false;
    }

    const char* statusSql = R"SQL(
        UPDATE submissions
        SET status = 'MARKED'
        WHERE submission_id = ?
          AND status <> 'RELEASED';
    )SQL";
    sqlite3_stmt* status = nullptr;
    if (sqlite3_prepare_v2(db, statusSql, -1, &status, nullptr) != SQLITE_OK) {
        execute("ROLLBACK;");
        error = sqlite3_errmsg(db);
        return false;
    }
    sqlite3_bind_int(status, 1, submissionId);
    const int statusRc = sqlite3_step(status);
    sqlite3_finalize(status);

    if (statusRc != SQLITE_DONE) {
        execute("ROLLBACK;");
        error = "Submission status could not be updated after marking.";
        return false;
    }

    if (!execute("COMMIT;")) {
        execute("ROLLBACK;");
        error = "Could not commit the mark record.";
        return false;
    }

    error.clear();
    return true;
}

bool Database::getUsers(
    std::vector<UserRecord>& users,
    std::string& error)
{
    users.clear();

    const char* sql =
        "SELECT user_id, username, role, active "
        "FROM users "
        "ORDER BY username COLLATE NOCASE;";

    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        error = sqlite3_errmsg(db);
        return false;
    }

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        UserRecord user{};

        user.userId = sqlite3_column_int(stmt, 0);

        const unsigned char* username =
            sqlite3_column_text(stmt, 1);

        const unsigned char* role =
            sqlite3_column_text(stmt, 2);

        user.username = username
            ? reinterpret_cast<const char*>(username)
            : "";

        user.role = role
            ? reinterpret_cast<const char*>(role)
            : "";

        user.active = sqlite3_column_int(stmt, 3) != 0;

        users.push_back(user);
    }

    if (rc != SQLITE_DONE) {
        error = sqlite3_errmsg(db);
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    error.clear();
    return true;
}

bool Database::getAdminAuditTrail(
    int adminId,
    const AuditTrailFilters& filters,
    std::vector<AuditTrailRecord>& records,
    std::string& error
) {
    records.clear();
    error.clear();
    if (db == nullptr || adminId <= 0) {
        error = "An active administrator is required to view the audit trail.";
        return false;
    }

    sqlite3_stmt* authorization = nullptr;
    const char* authorizationSql =
        "SELECT 1 FROM users "
        "WHERE user_id = ? AND role = 'ADMIN' AND active = 1;";
    if (sqlite3_prepare_v2(
            db, authorizationSql, -1, &authorization, nullptr) != SQLITE_OK) {
        error = sqlite3_errmsg(db);
        return false;
    }
    sqlite3_bind_int(authorization, 1, adminId);
    const int authorizationRc = sqlite3_step(authorization);
    sqlite3_finalize(authorization);
    if (authorizationRc != SQLITE_ROW) {
        error = authorizationRc == SQLITE_DONE
            ? "Only an active administrator may view the audit trail."
            : sqlite3_errmsg(db);
        return false;
    }

    const char* sql = R"SQL(
        SELECT a.occurred_at, COALESCE(a.actor_id, 0),
               COALESCE(u.username, ''), COALESCE(u.role, ''),
               a.action, a.entity_type, a.entity_id,
               a.old_value, a.new_value, a.reason, a.details
        FROM system_audit a
        LEFT JOIN users u ON u.user_id = a.actor_id
        WHERE (? = '' OR a.action = ?)
          AND (? = '' OR a.entity_type = ?)
          AND (? = '' OR u.username LIKE '%' || ? || '%')
          AND (? = '' OR date(a.occurred_at) >= date(?))
          AND (? = '' OR date(a.occurred_at) <= date(?))
          AND (
              ? = '' OR a.action LIKE '%' || ? || '%'
              OR a.entity_type LIKE '%' || ? || '%'
              OR a.old_value LIKE '%' || ? || '%'
              OR a.new_value LIKE '%' || ? || '%'
              OR a.reason LIKE '%' || ? || '%'
              OR a.details LIKE '%' || ? || '%'
              OR COALESCE(u.username, '') LIKE '%' || ? || '%'
          )
        ORDER BY a.occurred_at DESC, a.audit_id DESC
        LIMIT 1000;
    )SQL";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        error = sqlite3_errmsg(db);
        return false;
    }

    int bindIndex = 1;
    const auto bindPair = [&stmt, &bindIndex](const std::string& value) {
        sqlite3_bind_text(
            stmt, bindIndex++, value.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(
            stmt, bindIndex++, value.c_str(), -1, SQLITE_TRANSIENT);
    };
    bindPair(filters.action);
    bindPair(filters.entityType);
    bindPair(filters.actor);
    bindPair(filters.startDate);
    bindPair(filters.endDate);
    for (int i = 0; i < 8; ++i) {
        sqlite3_bind_text(
            stmt, bindIndex++, filters.searchText.c_str(), -1,
            SQLITE_TRANSIENT);
    }

    int rc;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        auto text = [stmt](int column) {
            const auto* value = sqlite3_column_text(stmt, column);
            return value
                ? std::string(reinterpret_cast<const char*>(value))
                : std::string();
        };
        AuditTrailRecord record;
        record.occurredAt = text(0);
        record.actorId = sqlite3_column_int(stmt, 1);
        record.actorUsername = text(2);
        record.actorRole = text(3);
        record.action = text(4);
        record.entityType = text(5);
        record.entityId = sqlite3_column_int(stmt, 6);
        record.oldValue = text(7);
        record.newValue = text(8);
        record.reason = text(9);
        record.details = text(10);
        records.push_back(record);
    }

    if (rc != SQLITE_DONE) {
        error = sqlite3_errmsg(db);
        sqlite3_finalize(stmt);
        records.clear();
        return false;
    }
    sqlite3_finalize(stmt);
    return true;
}

bool Database::setUserActive(
    int userId,
    bool active,
    std::string& error)
{
    const char* sql =
        "UPDATE users "
        "SET active = ? "
        "WHERE user_id = ?;";

    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        error = sqlite3_errmsg(db);
        return false;
    }

    sqlite3_bind_int(stmt, 1, active ? 1 : 0);
    sqlite3_bind_int(stmt, 2, userId);

    rc = sqlite3_step(stmt);

    if (rc != SQLITE_DONE) {
        error = sqlite3_errmsg(db);
        sqlite3_finalize(stmt);
        return false;
    }

    int affectedRows = sqlite3_changes(db);

    sqlite3_finalize(stmt);

    if (affectedRows == 0) {
        error = "User not found, or the account already has that status.";
        return false;
    }

    error.clear();
    return true;
}

bool Database::getEnrolments(
    std::vector<EnrolmentRecord>& enrolments,
    std::string& error)
{
    enrolments.clear();

    const char* sql =
        "SELECT e.student_id, u.username, "
        "e.course_id, c.course_code, c.name, e.enrolled_at "
        "FROM enrolments e "
        "JOIN users u ON u.user_id = e.student_id "
        "JOIN courses c ON c.course_id = e.course_id "
        "ORDER BY c.course_code, u.username;";

    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        error = sqlite3_errmsg(db);
        return false;
    }

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        EnrolmentRecord record{};

        record.studentId = sqlite3_column_int(stmt, 0);
        record.courseId = sqlite3_column_int(stmt, 2);

        const unsigned char* username =
            sqlite3_column_text(stmt, 1);

        const unsigned char* courseCode =
            sqlite3_column_text(stmt, 3);

        const unsigned char* courseName =
            sqlite3_column_text(stmt, 4);

        const unsigned char* enrolledAt =
            sqlite3_column_text(stmt, 5);

        record.studentUsername = username
            ? reinterpret_cast<const char*>(username) : "";

        record.courseCode = courseCode
            ? reinterpret_cast<const char*>(courseCode) : "";

        record.courseName = courseName
            ? reinterpret_cast<const char*>(courseName) : "";

        record.enrolledAt = enrolledAt
            ? reinterpret_cast<const char*>(enrolledAt) : "";

        enrolments.push_back(record);
    }

    if (rc != SQLITE_DONE) {
        error = sqlite3_errmsg(db);
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    error.clear();
    return true;
}

bool Database::enrolStudent(
    int studentId,
    int courseId,
    std::string& error)
{
    const char* sql =
        "INSERT INTO enrolments (student_id, course_id) "
        "SELECT ?, ? "
        "WHERE EXISTS ("
        "    SELECT 1 FROM users "
        "    WHERE user_id = ? AND role = 'STUDENT' AND active = 1"
        ") "
        "AND EXISTS ("
        "    SELECT 1 FROM courses WHERE course_id = ?"
        ");";

    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        error = sqlite3_errmsg(db);
        return false;
    }

    sqlite3_bind_int(stmt, 1, studentId);
    sqlite3_bind_int(stmt, 2, courseId);
    sqlite3_bind_int(stmt, 3, studentId);
    sqlite3_bind_int(stmt, 4, courseId);

    rc = sqlite3_step(stmt);

    if (rc != SQLITE_DONE) {
        error = sqlite3_errmsg(db);
        sqlite3_finalize(stmt);

        if (rc == SQLITE_CONSTRAINT) {
            error = "The student is already enrolled in this course.";
        }

        return false;
    }

    int affectedRows = sqlite3_changes(db);

    sqlite3_finalize(stmt);

    if (affectedRows == 0) {
        error =
            "Enrolment failed. Check that the student and course exist "
            "and that the student account is active.";
        return false;
    }

    error.clear();
    return true;
}

bool Database::removeEnrolment(
    int studentId,
    int courseId,
    std::string& error)
{
    const char* sql =
        "DELETE FROM enrolments "
        "WHERE student_id = ? AND course_id = ?;";

    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        error = sqlite3_errmsg(db);
        return false;
    }

    sqlite3_bind_int(stmt, 1, studentId);
    sqlite3_bind_int(stmt, 2, courseId);

    rc = sqlite3_step(stmt);

    if (rc != SQLITE_DONE) {
        error = sqlite3_errmsg(db);
        sqlite3_finalize(stmt);
        return false;
    }

    int affectedRows = sqlite3_changes(db);

    sqlite3_finalize(stmt);

    if (affectedRows == 0) {
        error = "The enrolment does not exist.";
        return false;
    }

    error.clear();
    return true;
}

bool Database::getCourseLecturers(
    std::vector<CourseLecturerRecord>& assignments,
    std::string& error)
{
    assignments.clear();

    const char* sql =
        "SELECT cl.course_id, c.course_code, c.name, "
        "cl.lecturer_id, u.username "
        "FROM course_lecturers cl "
        "JOIN courses c ON c.course_id = cl.course_id "
        "JOIN users u ON u.user_id = cl.lecturer_id "
        "ORDER BY c.course_code, u.username;";

    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        error = sqlite3_errmsg(db);
        return false;
    }

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        CourseLecturerRecord record{};

        record.courseId = sqlite3_column_int(stmt, 0);
        record.lecturerId = sqlite3_column_int(stmt, 3);

        const unsigned char* code =
            sqlite3_column_text(stmt, 1);

        const unsigned char* name =
            sqlite3_column_text(stmt, 2);

        const unsigned char* username =
            sqlite3_column_text(stmt, 4);

        record.courseCode = code
            ? reinterpret_cast<const char*>(code) : "";

        record.courseName = name
            ? reinterpret_cast<const char*>(name) : "";

        record.lecturerUsername = username
            ? reinterpret_cast<const char*>(username) : "";

        assignments.push_back(record);
    }

    if (rc != SQLITE_DONE) {
        error = sqlite3_errmsg(db);
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    error.clear();
    return true;
}

bool Database::assignLecturerToCourse(
    int courseId,
    int lecturerId,
    std::string& error)
{
    const char* sql =
        "INSERT INTO course_lecturers (course_id, lecturer_id) "
        "SELECT ?, ? "
        "WHERE EXISTS ("
        "    SELECT 1 FROM courses WHERE course_id = ?"
        ") "
        "AND EXISTS ("
        "    SELECT 1 FROM users "
        "    WHERE user_id = ? AND role = 'LECTURER' AND active = 1"
        ");";

    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        error = sqlite3_errmsg(db);
        return false;
    }

    sqlite3_bind_int(stmt, 1, courseId);
    sqlite3_bind_int(stmt, 2, lecturerId);
    sqlite3_bind_int(stmt, 3, courseId);
    sqlite3_bind_int(stmt, 4, lecturerId);

    rc = sqlite3_step(stmt);

    if (rc != SQLITE_DONE) {
        error = sqlite3_errmsg(db);
        sqlite3_finalize(stmt);

        if (rc == SQLITE_CONSTRAINT) {
            error = "This lecturer is already assigned to the course.";
        }

        return false;
    }

    int affectedRows = sqlite3_changes(db);

    sqlite3_finalize(stmt);

    if (affectedRows == 0) {
        error =
            "Assignment failed. Check that the course exists and "
            "the lecturer account is active.";
        return false;
    }

    error.clear();
    return true;
}

bool Database::removeLecturerFromCourse(
    int courseId,
    int lecturerId,
    std::string& error)
{
    const char* sql =
        "DELETE FROM course_lecturers "
        "WHERE course_id = ? AND lecturer_id = ?;";

    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        error = sqlite3_errmsg(db);
        return false;
    }

    sqlite3_bind_int(stmt, 1, courseId);
    sqlite3_bind_int(stmt, 2, lecturerId);

    rc = sqlite3_step(stmt);

    if (rc != SQLITE_DONE) {
        error = sqlite3_errmsg(db);
        sqlite3_finalize(stmt);
        return false;
    }

    int affectedRows = sqlite3_changes(db);

    sqlite3_finalize(stmt);

    if (affectedRows == 0) {
        error = "The lecturer is not assigned to this course.";
        return false;
    }

    error.clear();
    return true;
}

bool Database::assignHodToDepartment(
    int departmentId,
    int hodUserId,
    std::string& error)
{
    const char* sql =
        "UPDATE departments "
        "SET hod_user_id = ? "
        "WHERE department_id = ? "
        "AND EXISTS ("
        "    SELECT 1 FROM users "
        "    WHERE user_id = ? AND role = 'HOD' AND active = 1"
        ");";

    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        error = sqlite3_errmsg(db);
        return false;
    }

    sqlite3_bind_int(stmt, 1, hodUserId);
    sqlite3_bind_int(stmt, 2, departmentId);
    sqlite3_bind_int(stmt, 3, hodUserId);

    rc = sqlite3_step(stmt);

    if (rc != SQLITE_DONE) {
        error = sqlite3_errmsg(db);
        sqlite3_finalize(stmt);
        return false;
    }

    int affectedRows = sqlite3_changes(db);

    sqlite3_finalize(stmt);

    if (affectedRows == 0) {
        error =
            "HoD assignment failed. Check that the department exists "
            "and the selected HoD account is active.";
        return false;
    }

    error.clear();
    return true;
}

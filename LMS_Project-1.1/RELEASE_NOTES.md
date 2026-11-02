# Release notes

## v1.1 — 2026-11-02

Maintenance release implementing change request CR1.

### Added

- Lecturers can grant a deadline extension to an individual student or to
  everyone enrolled in the assignment's course.
- Student assignment lists show the effective deadline and whether it is
  standard, individually extended, class extended, or covered by both.
- Class-wide extensions are persisted separately from individual extensions
  and included in the administrative audit trail.
- Existing v1.0 databases receive the new class-extension table
  automatically at startup; existing individual extension records are
  preserved.

### Deadline rules

- Class extensions apply to all students enrolled in the assignment's course,
  including students enrolled after the extension is granted.
- If an individual and a class extension both apply, the later valid deadline
  is the effective deadline.
- The effective deadline is used for assignment display, submission and
  resubmission validation, and late-status calculation.

### Regression testing

See [CR1-ANALYSIS.md](./CR1-ANALYSIS.md) for the change analysis, affected
requirements/design, added test cases, and v1.1 regression test results
(73/73 passed; 78.6% business-logic line coverage, including `Database.cpp`).

## v1.0 — 2026-11-02

Initial tagged release of the Learning Management System desktop application.

### Included

- Qt desktop sign-in and role-specific dashboards for students, lecturers,
  Heads of Department, and administrators.
- Administrator workflows for managing user accounts, departments, courses,
  student enrolments, lecturer-course assignments, and HOD assignments.
- Lecturer workflows for creating assignments, setting attempt limits,
  granting individual deadline extensions, reviewing submissions, and
  recording or correcting marks and feedback.
- Student workflows for viewing courses and assignments, submitting work,
  reviewing submission history, and viewing released marks and feedback.
- HOD workflows for reviewing marks, approving or returning marks with a
  required comment, and releasing approved marks.
- Administrator audit-trail search and filters for supported events.
- SQLite persistence, password hashing, unit tests, and workflow integration
  tests.

### Test and coverage snapshot

The available GCC coverage report records **188/212 executable lines
(88.7%, approximately 89%)** across the business-logic files:
`AuthService`, `CourseService`, `LatePenaltyStrategy`, `MarkingService`, and
`PasswordUtils`. This is not the whole-project coverage figure; database and
Qt presentation code are outside this metric.

### Known limitations

- A fresh database has no user accounts, and the application has no first-run
  administrator setup screen or supported account-provisioning command.
  Obtain a secure first-admin provisioning procedure from the maintainer
  before attempting a new operational installation.
- Assignment submissions are stored as file references, not uploaded to a
  central file service. Staff need access to the referenced files on the
  computer where the application runs.
- The application uses a local SQLite database and is not a web or
  multi-site client/server system.
- No general database migration, password reset, or backup/restore tool is
  provided. v1.1 adds only the class-extension table described above.
- Windows builds currently expect MSYS2 UCRT64 dependencies under
  `C:\msys64`; the executable requires the matching runtime libraries.

See [INSTALLATION.md](./INSTALLATION.md) for technician setup instructions
and [USER_MANUAL.md](./USER_MANUAL.md) for role-based workflows.

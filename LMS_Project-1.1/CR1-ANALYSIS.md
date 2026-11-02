# CR1 — Maintenance Analysis and Regression Results

## Change request

> A lecturer should be able to extend the deadline for a whole class, not just
> an individual student, and extensions should be shown on the student's
> assignment list.

## Classification and impact

CR1 is **perfective (enhancement) maintenance**: it adds a requested lecturer
capability and makes existing assignment-list information clearer. It is not
corrective maintenance because it does not repair a failure against the
released v1.0 requirements, nor adaptive maintenance because no external
platform or environment changed.

### Requirements trace

The original requirement statements are in `Requirement Engineering.pdf`.
The following requirements and rules are directly affected:

| Requirement or rule | CR1 impact |
|---|---|
| FR11 — lecturer grants an individual extension and records its revised deadline | Retain individual extensions and extend the requirement to support a whole-class scope, with lecturer, assignment, revised deadline, and reason recorded. |
| FR7 — student views enrolled courses and associated assignments | Include the effective deadline and an extension indicator in each assignment row. |
| FR8 — student submits or resubmits before the applicable deadline | Resolve the applicable deadline from the assignment, class extension, and any individual extension before validating a submission or resubmission. |
| Business rule — an individual extension applies only to its named student | Preserve this rule. A separate class extension applies to all students enrolled in the assignment's course, including students enrolled later. |
| Business rule — submissions use the applicable deadline | Preserve validation and late-status behavior using the latest effective deadline. Existing recorded submission statuses are not rewritten when a later class extension is granted. |
| Assignment-list performance requirement for a dataset of 800 students | The assignment query adds a class-extension lookup. It remains indexed by the class-extension primary key (`assignment_id`); this change does not alter the stated dataset or response-time target. The target still requires performance validation on representative data. |

### Affected diagrams and design documents

| Existing design item | Impact and v1.1 update |
|---|---|
| `System Design.pdf`, class diagram / assignment-service responsibilities | Add lecturer scope selection, class-extension persistence, and extension metadata returned for student assignment rows. The PDF is retained as the v1.0 baseline; editable v1.1 diagrams are supplied in `CR1-DESIGN.md` and the linked PlantUML files. |
| `System Design.pdf`, database schema / `EXTENSIONS` table | Keep `EXTENSIONS` for per-student records and add `ASSIGNMENT_CLASS_EXTENSIONS`, keyed by assignment. See `cr1-data-model.puml`. |
| `System Design.pdf`, assignment-submission sequence | Update the effective-deadline read used before submission and late-status calculation to include class and individual extensions. See `cr1-deadline-sequence.puml`. |
| `deployment-diagram.puml` | No deployment node or connection changes: the app and SQLite database remain local to the Windows workstation. Its database note now identifies the new table. |

### Affected implementation classes and operations

| Class / operation | Change |
|---|---|
| `LecturerWindow::grantDeadlineExtension` | Add Individual student / Whole class scope selection and call the matching database operation. |
| `Database::grantAssignmentExtension` | Preserve individual behavior; use the effective class/individual deadline when validating the revised date. |
| `Database::grantClassAssignmentExtension` | New authorized, transactional class extension operation with future/later deadline validation and an audit event. |
| `Database::createTables` | Create the additive class-extension table for both new and existing databases. |
| `Database::getStudentAssignments` | Return the effective deadline and an extension label (`Standard`, `Individual extension`, `Class extension`, or `Class + individual`). |
| `Database::submitAssignment` | Enforce the effective deadline for submissions and resubmissions. |
| `Database::getSubmissionMarkingContext` | Calculate the marking/late context against the same effective deadline. |
| `StudentAssignmentRecord` | Add `deadlineExtension` display metadata. |
| `StudentWindow::loadAssignments` | Display the extension label beside the effective deadline. |

### Affected database tables

| Table | Impact |
|---|---|
| `assignments` | Existing base deadline remains unchanged and is the lower bound for an extension. |
| `extensions` | Existing per-student rows and uniqueness remain unchanged; individual extensions continue to apply only to their student. |
| `assignment_class_extensions` | New table with one current row per assignment: revised deadline, granting lecturer, reason, and timestamp. The primary key is also the assignment lookup index. |
| `course_lecturers` | Read to authorize that the requesting lecturer teaches the assignment's course. |
| `enrolments` | Determines which students receive a class extension in assignment listings and deadline validation. |
| `submissions` | Existing submission history/status continues to determine attempts and recorded late status. |
| `system_audit` | Receives the existing deadline-extension action with `scope=CLASS` for the new operation. |

### Affected and added tests

Existing regression coverage retained:

- `LMS_Workflow_Integration` / `testAssessmentWorkflow`: individual extension,
  student assignment deadline, submission validation, and late/marking context.
- `LMS_AuditTrail_Integration` / `testAuditTrailAccessAndFiltering`.
- All GoogleTest cases in `LatePenaltyTests`, `AuthServiceTests`,
  `CourseServiceTests`, and `DatabaseQueryTests`.

New CR1 tests in `DatabaseQueryTests`:

- `ClassExtensionAppliesToCurrentAndFutureEnrolments`
- `IndividualExtensionCanExtendClassDeadline`
- `RejectsClassExtensionByUnassignedLecturer`
- `RejectsClassDeadlineThatDoesNotExtendCurrentDeadline`
- `CreatesClassExtensionTableForExistingDatabase`

## v1.1 regression test results

Recorded after implementation in the v1.1 build. The suite is run with:

```powershell
cmake -S . -B build -DBUILD_TESTING=ON -DENABLE_COVERAGE=ON
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
```

**Result:** build succeeded and **73/73 tests passed** (100%) on the v1.1
build, including all GoogleTest unit cases and both CTest integration suites.
The two focused authorization/deadline-validation tests also passed when
rerun after correcting their fixture setup.

The measured business-logic line coverage is **78.6% (1,356/1,725 lines)**,
including `Database.cpp` with the CR1 deadline-extension logic. It was
measured after the full test run with:

```powershell
gcovr --root . --object-directory build --filter 'src/(AuthService|CourseService|Database|LatePenaltyStrategy|MarkingService|PasswordUtils)\.cpp$' --txt --print-summary
```

# CR1 — v1.1 Design Update

This addendum updates the v1.0 system design for class-wide deadline
extensions. It describes the implementation in the repository; the original
`System Design.pdf` remains the v1.0 baseline.

## Design decision

Keep individual extensions in `extensions`. Store the single current
class-wide extension for an assignment in a separate
`assignment_class_extensions` table. A class extension is assignment-scoped
and is resolved for every currently or subsequently enrolled student in the
assignment's course. This avoids copying a class decision into a potentially
large set of per-student rows.

The effective deadline is the latest of:

1. The assignment's original deadline.
2. The assignment's class-wide revised deadline, if present.
3. The student's individual revised deadline, if present.

Both grant operations require the requester to teach the assignment's course,
a non-empty reason, a parseable deadline later than the current applicable
deadline, and a deadline in the future. Granting is transactional and audited.
Existing submission status is a record of the decision made when the
submission was accepted; a later extension does not rewrite submission
history.

`Database::createTables()` creates the new table with `IF NOT EXISTS`.
Therefore, opening a v1.0 database upgrades it additively without changing or
deleting existing individual extension rows.

## Updated diagrams

- [Implementation class diagram](./cr1-class-diagram.puml)
- [Deadline-extension data model](./cr1-data-model.puml)
- [Class-extension and submission sequence](./cr1-deadline-sequence.puml)
- [Deployment diagram](./deployment-diagram.puml) — topology unchanged; the
  local SQLite node now includes the class-extension table.

## Data model

`assignment_class_extensions.assignment_id` is both the primary key and a
foreign key to `assignments.assignment_id`; it permits at most one current
class-wide extension per assignment. A later grant updates that row and adds
another `system_audit` record. `granted_by` references the lecturer account.
The existing `extensions` table remains unique by
`(assignment_id, student_id)`.

The assignment-list query joins both extension tables and returns the
effective deadline plus a human-readable extension label. The same effective
deadline is used by submission validation and lecturer marking context.

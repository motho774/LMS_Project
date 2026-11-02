# LMS v1.0 — User Manual

This manual covers the application functions currently exposed to the four
account roles: **Student**, **Lecturer**, **Head of Department (HOD)**, and
**Administrator**.

## Sign in and sign out

1. Start `lms_gui.exe`.
2. Enter the username and password issued by the school administrator and
   select **Login**. Pressing Enter in the password field also submits the
   login form.
3. If login fails, check the spelling and ask an administrator to confirm
   that the account is active. The password field is cleared after a failed
   attempt.
4. Select **Logout** on the dashboard to return to the login screen.

The dashboard displays the signed-in username, role, and user ID. Your role
determines which actions appear.

## Student

The Student dashboard provides **My Courses**, **View Assignments**,
**My Submissions**, and **My Marks and Feedback**.

### View courses and assignments

1. Open **My Courses** and select **Refresh Courses** to reload your
   enrolments.
2. Select a course and choose **View Selected Course Assignments**. You can
   also open **View Assignments** to see assignments across your courses.
3. Select an assignment to read its description, deadline, maximum mark, and
   attempts used/allowed.
4. Use **Refresh Assignments** to reload the list or **Show All Courses** to
   clear a course-specific filter.

### Submit an assignment

1. In **View Assignments**, select an eligible assignment.
2. Choose **Submit Selected Assignment** and select the file in the file
   picker.
3. Check the confirmation message. The assignment's attempts and applicable
   deadline determine whether another submission is allowed.
4. Open **My Submissions** and choose **Refresh Submissions** to see the
   recorded version, submission time, and status.

The application records a file reference rather than transferring the file
to a server. Keep the selected file in place and readable; deleting or moving
it may prevent staff from opening it. The program does not provide a way to
withdraw a submission.

### View marks and feedback

Open **My Marks and Feedback** and choose **Refresh Released Marks**. Marks
appear after the HOD releases them. Contact the lecturer or HOD through the
school's normal channels if feedback is missing or needs clarification.

## Lecturer

The Lecturer dashboard provides **My Courses**, **Manage Assignments**, and
**View Submissions**. The dashboard's **Mark Submissions** button opens the
same submissions tab.

### View assigned courses

Open **My Courses** and select **Refresh Courses**. If no course appears,
contact an administrator to check the course assignment.

### Create an assignment

1. Open **Manage Assignments**.
2. Select an assigned course and enter a title, description, future deadline,
   positive maximum mark, and maximum submission count.
3. Choose **Create Assignment**. A successful assignment appears in the
   assignments list.
4. Use **Refresh Assignments** to reload the list.

### Grant an individual extension

1. In **Manage Assignments**, choose **Grant Individual Extension**.
2. Select an assignment and eligible student, set the revised deadline, and
   provide a reason.
3. Save the extension. The revised deadline applies to that student; keep the
   reason clear for the audit record.

### Review submissions and mark work

1. Open **View Submissions** and select **Refresh Submissions**.
2. Select a row and choose **Download Selected File** to open/save the
   referenced file. The file must be accessible on the computer running LMS.
3. Choose **Mark / Correct Selected Submission**, enter the mark and feedback,
   and confirm. Use the same action to correct a mark when necessary.
4. Refresh the list to check the updated status. Marks are subject to HOD
   moderation before release to students.

## Head of Department (HOD)

The HOD dashboard provides **Department Courses**, **Review Marks**,
**Moderate Marks**, and **Release Approved Marks**. Review and Moderate open
the same moderation tab.

### Review courses and marks

- **Department Courses** lists courses assigned to your department. Select
  **Refresh Courses** to reload them.
- **Review / Moderate Marks** lists marks for your department. Select
  **Refresh Marks** before reviewing if the list may have changed.

### Approve or return a mark

1. In **Review / Moderate Marks**, select the mark row.
2. Choose **Approve Selected Mark** or **Return to Lecturer**.
3. Enter a non-empty comment explaining the decision.
4. Confirm the saved decision. A returned mark goes back to the lecturer for
   correction; an approved mark becomes eligible for release.

### Release approved marks

1. Open **Release Approved Marks** and choose **Refresh Queue**.
2. Review the approved, unreleased marks listed for your department.
3. Choose **Release All Approved Marks** and confirm. This action releases
   every currently approved, unreleased mark in your department, not only a
   selected row.
4. Students can then see the released marks and feedback in their workspace.

## Administrator

The Administrator dashboard provides **Manage Users**, **Manage Departments**,
**Manage Courses**, **Manage Enrolments**, and **Audit Trail**. These actions
open tabs in the administration workspace.

### Set up school structure

1. In **Departments & Courses**, enter a department name and choose
   **Add Department**.
2. Enter a course code and course name, select its department, then choose
   **Add Course**.
3. Use **Refresh All** to reload administration data.

### Create and maintain accounts

1. Open **User Management** and choose **Create User**.
2. Enter a username and password, select one role (`STUDENT`, `LECTURER`,
   `HOD`, or `ADMIN`), and save.
3. To change a user, select the row and choose **Edit Selected User**. Leave
   the password field blank to keep the current password.
4. Select a row and choose **Activate / Deactivate** to change account
   access. Inactive users cannot log in.
5. Use **Refresh Users** after changes.

Use individual accounts and issue passwords privately. There is no password
reset workflow in the application; follow the school's approved process for
account recovery.

### Enrol students and assign staff

- In **Student Enrolments**, select a student and course, then choose **Enrol
  Student**. To remove an enrolment, select it and choose **Remove Selected
  Enrolment**.
- In **Lecturer Assignments**, select a lecturer and course, then choose
  **Assign Lecturer**. Select an existing assignment and choose **Remove
  Selected Assignment** to remove it.
- In **HOD Assignments**, select a department and HOD account, then choose
  **Assign HOD**. Assigning a new HOD replaces the current department HOD.

### Review the audit trail

Open **Audit Trail** and optionally filter by action, entity, actor username,
search text, or date range. Choose **Apply Filters** to refresh the results.
The trail records supported account, assignment, submission, marking,
moderation, extension, and release events; it is not a general log of every
screen interaction.

## All users

- Use the **Refresh** buttons when another staff member may have changed data.
- Close a workspace to return to the dashboard. Use **Logout** to end the
  session and return to sign-in.
- Report database errors, inaccessible submission files, or unexpected
  account access to the school administrator.
- The application is a desktop program backed by a local SQLite file. It
  does not provide web access, email notifications, or file transfer between
  separate computers.

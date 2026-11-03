Learning Management System (LMS)
A desktop Learning Management System developed using C++17, Qt 6
Widgets and SQLite. It supports four roles: Administrator, Head of
Department (HoD), Lecturer and Student.
Repository: LMS_Project  
Releases: Download the two available
versions
---
Releases and source code
This project has two versions published through GitHub Releases. The
source files and version-specific assets are available from the Releases
page.
Open the GitHub Releases
page.
Select the version you want.
Download its `Source code (zip)` or `Source code (tar.gz)` archive.
If a packaged application is attached, download that asset instead.
Keep each version in a separate folder. Do not mix source files from
different releases.
Read the release notes for version-specific changes and
instructions.
The Releases page is the authoritative location for the source snapshot
and application assets belonging to each version.
Main functionality
Administrator
Manage users, departments and courses.
Enrol students in courses.
Assign lecturers and Heads of Department.
View the administrative audit trail.
Lecturer
View assigned courses.
Manage assignments and view student submissions.
Mark submissions and provide feedback.
Head of Department
View departmental courses and marks.
Review and moderate marks.
Return marks for correction or approve and release marks.
Student
View enrolled courses and assignments.
Submit and resubmit assignments where permitted.
View submission status, released marks and feedback.
Default demonstration login
The default demonstration administrator account is:
Field      Default value
---
Username   `admin`
Password   `admin123`
Role       Administrator
Security warning: These are demonstration credentials. Change the
password immediately after the first login if the application provides
that option. Do not deploy the system with the default password or use
this account for real institutional data. Anyone who knows these
credentials may be able to access administrator functions.
If the credentials do not work with a particular release, check that
release's notes and database initialization instructions. The account
may depend on the database supplied with or initialized for that
release.
Technology stack
Language: C++17
GUI: Qt 6 Widgets
Database: SQLite
Password hashing: Argon2id
Build system: CMake
Development toolchain: GCC with MSYS2 UCRT64 on Windows
Automated testing: GoogleTest and CTest
Coverage: GCC coverage tools and gcovr
Running a released application on Windows
Download the packaged Windows application from the selected GitHub
Release, if an
executable package is provided.
Extract the archive.
Keep the executable and its required Qt/runtime libraries together.
Follow the release-specific database initialization instructions.
Launch `lms_gui.exe` if that executable is included.
Sign in using the demonstration credentials above, if the account is
included in that release's database.
If a release contains source code only, build it using the instructions
below.
Building from source (Windows / MSYS2 UCRT64)
Install the required UCRT64 compiler, CMake, Ninja, Qt 6 Widgets,
SQLite, Argon2 and GoogleTest development packages.
Open the MSYS2 UCRT64 terminal, navigate to the extracted project
directory, and run:
``` bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```
If CMake reports a missing dependency, install the corresponding
development package in the UCRT64 environment and rerun the
configuration command.
Run the application:
``` bash
./build/lms_gui.exe
```
The executable's exact location may vary depending on the build
directory and release packaging.
Run automated tests
``` bash
ctest --test-dir build --output-on-failure
```
Available test targets depend on the selected source release and its
CMake configuration.
Recorded testing results
In the recorded project verification run:
68 of 68 automated tests passed.
78% business-logic line coverage was measured using GCC coverage
instrumentation and gcovr.
Administrator login and access to the administration interface were
also manually tested.
These results describe the recorded test run. They do not guarantee that
every release, environment or workflow has been independently tested.
Performance with 800 student records, broad GUI acceptance testing
across all roles, and deployment on independent machines require
separate verification.
Database and data safety
The application uses SQLite for local data persistence.
Back up the database before replacing, upgrading or modifying it.
Do not delete a database containing records you need.
Use a separate database copy when experimenting with test data.
Protect the database because it may contain user details, assessment
records and audit information.
Never publish a production database, real student records, passwords
or other personal information in a public repository or release
asset.
Known limitations and future work
Complete GUI and acceptance testing for all four user roles.
Performance testing against the 800-student requirement.
Verification of offline operation on the institution's target
computers.
Continuous-integration workflow and repeatable release-build
evidence.
Deployment, backup/restore and usability testing on independent
machines.
Further assessment of class-wide deadline extensions and other
future change requests.
Repository structure
The exact files may differ between releases. The project generally
separates:
`include/` --- C++ header files.
`src/` --- implementation files.
`tests/` --- automated unit and integration tests.
`CMakeLists.txt` --- CMake build configuration.
`gui_main.cpp` --- GUI application entry point, where present.
Refer to the selected release's source archive for its exact structure.
Licence and contributions
This repository is provided for the associated academic software
engineering project. Check the repository for an explicit licence before
redistributing or reusing the source code. Third-party libraries remain
subject to their respective licences.
For issues, questions or suggestions, use the GitHub repository's Issues
page.

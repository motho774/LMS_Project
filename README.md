<div align="center">
🎓 Learning Management System (LMS)
A desktop Learning Management System for managing courses, assignments, marking and moderation across four user roles.
![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus&logoColor=white)
![Qt 6](https://img.shields.io/badge/Qt-6%20Widgets-41CD52?logo=qt&logoColor=white)
![SQLite](https://img.shields.io/badge/SQLite-local%20database-003B57?logo=sqlite&logoColor=white)
![CMake](https://img.shields.io/badge/build-CMake-064F8C?logo=cmake&logoColor=white)
![Tests](https://img.shields.io/badge/tests-68%2F68%20passing-brightgreen)
![Coverage](https://img.shields.io/badge/business--logic%20coverage-78%25-yellowgreen)
Releases · Features · Getting Started · Build from Source · Testing · Issues
</div>
---
📖 Overview
This LMS is a desktop application built with C++17, Qt 6 Widgets and SQLite. It supports four roles (Administrator, Head of Department, Lecturer and Student) and covers the full assessment workflow from enrolment and submission through to marking, moderation and release of results.
✨ Features by Role
Role	Capabilities
🛠️ Administrator	Manage users, departments and courses<br>Enrol students in courses<br>Assign lecturers and Heads of Department<br>View the administrative audit trail
👩‍🏫 Lecturer	View assigned courses<br>Manage assignments and view student submissions<br>Mark submissions and provide feedback
🏛️ Head of Department	View departmental courses and marks<br>Review and moderate marks<br>Return marks for correction, or approve and release them
🎒 Student	View enrolled courses and assignments<br>Submit and resubmit assignments where permitted<br>View submission status, released marks and feedback
🧰 Technology Stack
Area	Technology
Language	C++17
GUI	Qt 6 Widgets
Database	SQLite
Password hashing	Argon2id
Build system	CMake (with Ninja)
Toolchain	GCC via MSYS2 UCRT64 on Windows
Testing	GoogleTest and CTest
Coverage	GCC coverage tools and gcovr
📦 Releases
Two versions of this project are published through GitHub Releases. The Releases page is the authoritative location for each version's source snapshot and application assets.
Open the GitHub Releases page.
Select the version you want.
Download its Source code (`.zip` or `.tar.gz`) archive, or, if a packaged application is attached, download that asset instead.
> [!TIP]
> Keep each version in its own folder and never mix source files from different releases. Read the release notes for version-specific changes and instructions.
🚀 Getting Started
Run a packaged release (Windows)
Download the packaged Windows application from your chosen release, if one is provided.
Extract the archive, keeping the executable and its required Qt/runtime libraries together.
Follow the release-specific database initialisation instructions.
Launch `lms_gui.exe`.
Sign in with the demonstration credentials below.
If a release contains source code only, build it from source.
🔑 Default demonstration login
Field	Value
Username	`admin`
Password	`admin123`
Role	Administrator
> [!WARNING]
> These are **demonstration credentials only**. Change the password immediately after first login if the application provides that option. Never deploy with the default password or use this account for real institutional data, as anyone who knows these credentials may gain access to administrator functions.
> [!NOTE]
> If the credentials do not work for a particular release, check that release's notes and database initialisation instructions. The account may depend on the database supplied with, or initialised for, that release.
🔨 Build from Source
Platform: Windows with MSYS2 UCRT64
Prerequisites (install the UCRT64 packages for each):
GCC compiler
CMake and Ninja
Qt 6 Widgets
SQLite
Argon2
GoogleTest
Open the MSYS2 UCRT64 terminal, navigate to the extracted project directory, then run:
```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```
If CMake reports a missing dependency, install the corresponding development package in the UCRT64 environment and re-run the configuration command.
Run the application:
```bash
./build/lms_gui.exe
```
> The executable's exact location may vary depending on the build directory and release packaging.
🧪 Testing
```bash
ctest --test-dir build --output-on-failure
```
Available test targets depend on the selected release and its CMake configuration.
Recorded results
Check	Result
Automated tests	✅ 68 of 68 passed
Business-logic line coverage	📊 78% (GCC instrumentation + gcovr)
Administrator login and admin interface	✅ Manually tested
These results describe the recorded verification run only. They do not guarantee that every release, environment or workflow has been independently tested.
🔒 Database and Data Safety
The application stores its data locally in SQLite.
Back up the database before replacing, upgrading or modifying it.
Never delete a database containing records you need.
Use a separate copy when experimenting with test data.
Protect the database: it may contain user details, assessment records and audit information.
Never publish a production database, real student records, passwords or other personal information in a public repository or release asset.
🗺️ Known Limitations and Future Work
[ ] Complete GUI and acceptance testing for all four user roles
[ ] Performance testing against the 800-student requirement
[ ] Verification of offline operation on the institution's target computers
[ ] Continuous-integration workflow and repeatable release-build evidence
[ ] Deployment, backup/restore and usability testing on independent machines
[ ] Assessment of class-wide deadline extensions and other future change requests
📁 Repository Structure
The exact layout may differ between releases; refer to the selected release's source archive.
```text
LMS_Project/
├── include/          # C++ header files
├── src/              # Implementation files
├── tests/            # Automated unit and integration tests
├── gui_main.cpp      # GUI entry point (where present)
└── CMakeLists.txt    # CMake build configuration
```
📄 Licence and Contributions
This repository is provided for the associated academic software engineering project. Check the repository for an explicit licence before redistributing or reusing the source code. Third-party libraries remain subject to their respective licences.
For issues, questions or suggestions, please use the Issues page.

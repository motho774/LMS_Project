# LMS v1.0 — Installation Guide

This guide is for a technician building and installing the Windows desktop
application from source. The current CMake configuration expects MSYS2 at
`C:\msys64` and uses its UCRT64 libraries.

## Requirements

- A supported 64-bit Windows PC.
- Network access to install the build dependencies.
- MSYS2 installed at `C:\msys64`.
- A writable application-data directory with enough space for the SQLite
  database and submitted assignment files.

## Install build dependencies

Open the **MSYS2 UCRT64** terminal and update the package database:

```sh
pacman -Syu
```

If MSYS2 asks you to close the terminal, close it, reopen **MSYS2 UCRT64**, and
run the update command again. Then install the compiler, CMake, Qt, SQLite,
Argon2, and GoogleTest:

```sh
pacman -S --needed \
  mingw-w64-ucrt-x86_64-toolchain \
  mingw-w64-ucrt-x86_64-cmake \
  mingw-w64-ucrt-x86_64-ninja \
  mingw-w64-ucrt-x86_64-qt6-base \
  mingw-w64-ucrt-x86_64-sqlite3 \
  mingw-w64-ucrt-x86_64-argon2 \
  mingw-w64-ucrt-x86_64-gtest \
  mingw-w64-ucrt-x86_64-gcovr
```

## Build and test

In the UCRT64 terminal, clone the repository and configure a test-enabled
build:

```sh
git clone https://github.com/motho774/LMS_Project.git
cd LMS_Project
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DBUILD_TESTING=ON \
  -DENABLE_COVERAGE=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

The test suite includes GoogleTest unit tests and CTest workflow integration
tests. To report business-logic line coverage:

```sh
gcovr --root . --object-directory build \
  --filter 'src/(AuthService|CourseService|LatePenaltyStrategy|MarkingService|PasswordUtils)\.cpp$' \
  --txt --print-summary
```

The v1.0 coverage snapshot reports **188 of 212 executable business-logic
lines covered (88.7%, approximately 89%)**. This figure excludes the database
and Qt presentation layers.

## Build the application

For an application build without test targets, configure a separate build
directory:

```sh
cmake -S . -B build-app -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=OFF \
  -DENABLE_COVERAGE=OFF
cmake --build build-app --parallel
```

The executable is `build-app/lms_gui.exe`. Run it from the UCRT64 environment
on the build PC so the Qt, SQLite, Argon2, and UCRT runtime libraries are
available. A standalone school deployment needs those runtime dependencies
packaged or installed on the target PC; the executable alone is not a
self-contained installer.

## Install on the school PC

1. Copy the application executable and its required runtime DLLs to the
   school PC. Use the same MSYS2 UCRT64 runtime or a technician-prepared
   deployment package with the required Qt and library dependencies.
2. Put the executable in a `bin` subfolder, for example
   `C:\SchoolLMS\bin\lms_gui.exe`.
3. Ensure `C:\SchoolLMS` is a writable, backed-up data directory. The
   application looks for `lms.db` one directory above the executable first,
   so the expected database path for this layout is
   `C:\SchoolLMS\lms.db`. On first launch the application creates the schema
   if the database file does not exist.
4. Grant write access to the application-data directory only to the staff
   accounts that need to run or maintain the application. Do not put the
   database on an untested network share; SQLite is a local-file database,
   not a multi-site database server.
5. Launch `lms_gui.exe`, confirm the login window opens, and verify that the
   database can be opened and written.

## First administrator account — deployment blocker

A new database contains the schema but **no user accounts**. The application
does not provide a first-run administrator setup screen or a supported
command-line account-provisioning tool. Since creating accounts is an
administrator function, a fresh installation cannot be used to log in until
the project maintainer supplies a secure, supported way to provision the
first administrator.

Do not guess default credentials, distribute a database containing an
unknown administrator password, or manually edit password hashes in SQLite.
Treat first-admin provisioning as a go-live requirement.

## Backups and upgrades

- Close the application before copying `lms.db` for a backup. Keep backups in
  a restricted location and test restoring one before relying on it.
- Back up submitted assignment files separately. The database records file
  references; it is not a file-storage service.
- Before replacing an application build, back up both the database and
  referenced submission files.
- The project does not currently document an automatic database migration or
  rollback procedure. Test upgrades against a copy of the school database
  before upgrading production.

## Troubleshooting

| Symptom | Checks |
|---|---|
| Application reports a database error at startup | Confirm the selected database directory exists and is writable; verify that SQLite and its runtime DLL are available. |
| Qt platform/plugin error | Confirm the Qt runtime and platform plugins are installed and discoverable by the executable. |
| Missing DLL on launch | Run from the UCRT64 environment or install the matching UCRT64, Qt, SQLite, and Argon2 runtime dependencies. |
| Login fails on a new installation | A new database has no users. Obtain the supported first-administrator provisioning procedure from the project maintainer. |
| Submission cannot be opened by a lecturer | The saved file reference must still point to a readable file on the same PC. The application does not transfer files between separate computers. |

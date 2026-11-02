
cmake_minimum_required(VERSION 3.16)

project(LMS LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(CTest)

option(ENABLE_COVERAGE "Enable GCC code coverage for tests" OFF)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

# -------------------------
# GUI APPLICATION
# -------------------------

qt_add_executable(lms_gui
    include/Dashboard.h
    gui_main.cpp
    src/Database.cpp
    src/PasswordUtils.cpp
    src/AuthService.cpp
    src/Dashboard.cpp
    src/CourseService.cpp
    src/LecturerWindow.cpp
    src/CourseManagementWindow.cpp
    src/HodWindow.cpp
    src/StudentWindow.cpp
    src/LatePenaltyStrategy.cpp
    src/MarkingService.cpp
)

target_include_directories(lms_gui PRIVATE
    include
    C:/msys64/ucrt64/include
)

target_link_directories(lms_gui PRIVATE
    C:/msys64/ucrt64/lib
)

target_link_libraries(lms_gui PRIVATE
    Qt6::Widgets
    sqlite3
    argon2
)

# -------------------------
# TEST CONFIGURATION
# -------------------------

if(BUILD_TESTING)

    find_package(GTest CONFIG REQUIRED)

    # -------------------------
    # WORKFLOW INTEGRATION TEST
    # -------------------------

    add_executable(lms_workflow_test
        tests/lms_workflow_test.cpp
        src/Database.cpp
        src/PasswordUtils.cpp
        src/AuthService.cpp
        src/CourseService.cpp
        src/LatePenaltyStrategy.cpp
        src/MarkingService.cpp
    )

    target_include_directories(lms_workflow_test PRIVATE
        include
        C:/msys64/ucrt64/include
    )

    target_link_directories(lms_workflow_test PRIVATE
        C:/msys64/ucrt64/lib
    )

    target_link_libraries(lms_workflow_test PRIVATE
        sqlite3
        argon2
    )

    add_test(
        NAME LMS_Workflow_Integration
        COMMAND lms_workflow_test
    )

    # -------------------------
    # GOOGLE TEST UNIT TESTS
    # -------------------------

    add_executable(lms_unit_tests
        tests/unit/LatePenaltyTests.cpp
        src/LatePenaltyStrategy.cpp
    )

    target_include_directories(lms_unit_tests PRIVATE
        include
    )

    target_link_libraries(lms_unit_tests PRIVATE
        GTest::gtest_main
    )

    include(GoogleTest)

    gtest_discover_tests(lms_unit_tests)

    # -------------------------
    # CODE COVERAGE
    # -------------------------

    if(ENABLE_COVERAGE)

        if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            message(FATAL_ERROR
                "Coverage configuration currently supports GCC only."
            )
        endif()

        message(STATUS "GCC code coverage enabled")

        foreach(target lms_unit_tests lms_workflow_test)

            target_compile_options(${target} PRIVATE
                -O0
                -g
                --coverage
            )

            target_link_options(${target} PRIVATE
                --coverage
            )

        endforeach()

    endif()

endif()
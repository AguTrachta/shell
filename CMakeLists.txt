cmake_minimum_required(VERSION 3.10)
project(MiShell VERSION 1.0 LANGUAGES C)

# Set the C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Include the toolchain from Conan (adjust path if necessary)
include(${CMAKE_BINARY_DIR}/Release/generators/conan_toolchain.cmake)

# Configure CMAKE_PREFIX_PATH for dependencies
set(CMAKE_PREFIX_PATH "${CMAKE_BINARY_DIR}/Release/generators" ${CMAKE_PREFIX_PATH})

# Find the cJSON dependency managed by Conan
find_package(cJSON REQUIRED)

# Add header directories
include_directories(include)

# Set the output directory for executables
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}")

# Add Monitoring project as a subdirectory
add_subdirectory(monitor_program)

# Add the main Shell executable
add_executable(shell
    src/main.c
    src/shell.c
    src/commands.c
    src/parse.c
)

# Link necessary libraries for Shell
target_link_libraries(shell
    cjson::cjson
    readline
    m
)

# Coverage flags in Release mode
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -fprofile-arcs -ftest-coverage")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -fprofile-arcs -ftest-coverage")

# Add test executable
add_executable(test_commands
    tests/test_commands.c
    src/commands.c
    src/parse.c
    src/shell.c
)

# Set output directory for test_commands
set_target_properties(test_commands PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}")

# Link necessary libraries to the test executable
target_link_libraries(test_commands
    cjson::cjson
    readline
    m
    gcov # Required for code coverage
)



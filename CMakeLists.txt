# Project Information & Setup
cmake_minimum_required(VERSION 3.15)
project(AutomonScript LANGUAGES CXX)

# Project Follows C++17 Standards (Required for std::variant and std::filesystem)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Packs ANTLR4 Runtime in ams.exe (Language Compiler)
add_definitions(-DANTLR4CPP_STATIC)

# Verify ANTLR4 Dependency Available
set(ANTLR_JAR "${CMAKE_SOURCE_DIR}/external/antlr-4.13.1-complete.jar")
set(GEN_DIR "${CMAKE_SOURCE_DIR}/generated")

if(NOT EXISTS ${ANTLR_JAR})
    message(FATAL_ERROR "ANTLR JAR not found at ${ANTLR_JAR}. Please check the external folder.")
endif()

# Generate Language Files From AMS.g4
# We use execute_process to ensure files exist before the build starts
message(STATUS "Generating ANTLR4 Parser/Lexer files...")
execute_process(
    COMMAND java -jar ${ANTLR_JAR} 
            -Dlanguage=Cpp 
            -visitor 
            -no-listener 
            -lib "${CMAKE_SOURCE_DIR}/grammar" 
            -o "${GEN_DIR}" 
            "${CMAKE_SOURCE_DIR}/grammar/AMS.g4"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/grammar"
    RESULT_VARIABLE ANTLR_RESULT
)

if(NOT ANTLR_RESULT EQUAL 0)
    message(FATAL_ERROR "ANTLR4 Generation failed.")
endif()

# Define Source Files for the Compiler (ams.exe)
file(GLOB_RECURSE SRC_FILES 
    "src/*.cpp" 
    "generated/*.cpp"
    "external/antlr4-runtime/runtime/src/*.cpp"
)

# Create the Language Compiler Executable
add_executable(ams ${SRC_FILES})

# Include Path Management
# PRIVATE ensures these paths are used to build ams, but not leaked elsewhere
target_include_directories(ams PRIVATE 
    "${CMAKE_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/include"
    "${GEN_DIR}"
    "${CMAKE_SOURCE_DIR}/external/antlr4-runtime/runtime/src"
)

# Link Required Libraries
# 1. Threads: Required for the Generator and Runtime components
find_package(Threads REQUIRED)
target_link_libraries(ams PRIVATE Threads::Threads)

# 2. Filesystem: Required for older GCC versions (Linux) to use std::filesystem
if(NOT MSVC)
    target_link_libraries(ams PRIVATE stdc++fs)
endif()

# Platform-specific definitions for ams itself
if(WIN32)
    target_compile_definitions(ams PRIVATE _WIN32)
else()
    target_compile_definitions(ams PRIVATE __linux__)
endif()

message(STATUS "AutomonScript CMake Configuration Complete.")
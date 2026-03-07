# Project Information & Setup
cmake_minimum_required(VERSION 3.15)
project(AutomonScript LANGUAGES CXX)

# Project Follows C++17 Standards
set(CMAKE_CXX_STANDARD 17)

# Packs ANTLR4 Runtime in ams.exe (Language Compiler)
add_definitions(-DANTLR4CPP_STATIC)

# Verify ANTLR4 Dependency Available Both .JAR(Default Required) and ./generated(CPP Use Required)
set(ANTLR_JAR "${CMAKE_SOURCE_DIR}/external/antlr-4.13.1-complete.jar")
set(GEN_DIR "${CMAKE_SOURCE_DIR}/generated")

# Generate Language Files Form AMS.g4 (ANTLR4 Grammer File)
execute_process(
    COMMAND java -jar ${ANTLR_JAR} -Dlanguage=Cpp -visitor -no-listener -o ${GEN_DIR} ${CMAKE_SOURCE_DIR}/grammar/AMS.g4
)

# Define Source Files
file(GLOB_RECURSE SRC_FILES 
    "src/*.cpp" 
    "generated/*.cpp"
    "external/antlr4-runtime/runtime/src/*.cpp"
)

# Link All Source Files to produce ams.exe (Language Compiler
add_executable(ams ${SRC_FILES})

# Include Path Management : Helps Access Header Files Directly Anywhere in Project
target_include_directories(ams PRIVATE 
    "${CMAKE_SOURCE_DIR}/include"
    "${GEN_DIR}"
    "${CMAKE_SOURCE_DIR}/external/antlr4-runtime/runtime/src"
)

# Link Required Libraries
find_package(Threads REQUIRED)
target_link_libraries(ams PRIVATE Threads::Threads)
target_link_libraries(ams PRIVATE stdc++fs)

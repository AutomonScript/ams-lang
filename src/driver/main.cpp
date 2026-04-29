#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <memory>



#include "antlr4-runtime.h"
#include "AMSLexer.h"
#include "AMSParser.h"   

#include "core/supports/caseCaptilizeInputStream.hpp"
#include "core/builder/AST.hpp"       
#include "core/builder/Builder.hpp"
#include "core/compiler/Generator.hpp" 

<<<<<<< HEAD
=======
#ifdef _WIN32
    #include <windows.h>
#elif __linux__
    #include <limits.h>
    #include <unistd.h>
#endif


>>>>>>> b7b3605a0ebc96e43758260dc77452ebf3180bb7
using namespace antlr4;
using namespace ams;

std::filesystem::path getExecutablePath() {
#ifdef _WIN32
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    return std::filesystem::path(path);
#elif __linux__
    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    return std::filesystem::path(std::string(result, (count > 0) ? count : 0));
#else
    return std::filesystem::current_path();
#endif
}

int main(int argc, const char* argv[]) {
    if (argc < 3) {
        std::cout << "Usage: ams build <file.ams>" << std::endl;
        return 1;
    }

    std::string mode = argv[1];     
    std::string filePath = argv[2]; 
    std::filesystem::path inputPath(filePath);
    std::ifstream stream(filePath);
    
    if (!stream.good()) {
        std::cerr << "Error: Could not open file " << filePath << std::endl;
        return 1;
    }

    CaseCaptilizeInputStream input(stream);
    AMSLexer lexer(&input);
    CommonTokenStream tokens(&lexer);
    AMSParser parser(&tokens);

    tree::ParseTree* parseTree = parser.program();

    if (parser.getNumberOfSyntaxErrors() > 0) {
        return 1;
    }

    Builder astBuilder;
    auto ast = std::any_cast<std::shared_ptr<ProgramNode>>(astBuilder.visit(parseTree));

    if (mode == "build") {
        std::string tempCpp = "temp_output.cpp";
        std::filesystem::path exePath = inputPath.filename();
        
        #ifdef _WIN32
            exePath.replace_extension(".exe");
        #elif __linux__
            exePath.replace_extension("");
        #endif

<<<<<<< HEAD
        // These brackets force the Generator to flush its buffer and close 
        // the file before moving on to the std::system compile command.
        {
            Generator generator(tempCpp);
            generator.generate(ast);
        }
       
        
        std::string compileCmd = "g++ -O2 -I .. " + tempCpp + 
                                 " -o " + exePath.string() + " -pthread -mconsole";
=======
        Generator generator(tempCpp);
        generator.generate(ast);
        generator.close();

        // Determine include path
        std::filesystem::path enginePath = getExecutablePath().parent_path();
        std::filesystem::path includePath = enginePath / "include";
        if (!std::filesystem::exists(includePath)) {
            includePath = enginePath / ".." / "include"; // For local dev
            if (!std::filesystem::exists(includePath)) {
                includePath = enginePath / ".." / "share" / "ams-lang" / "include"; // For Linux install
            }
        }

        std::string includeFlag = " -I\"" + includePath.string() + "\"";

        // Compile to Executable
        std::string compileCmd = "g++ " + tempCpp + " -o \"" + exePath.string() + "\"" + flag + includeFlag;
>>>>>>> b7b3605a0ebc96e43758260dc77452ebf3180bb7

        if (std::system(compileCmd.c_str()) == 0) {
            std::filesystem::remove(tempCpp); 
            std::cout << "[SUCCESS] Created: " << exePath.filename().string() << std::endl;
        } else {
            std::cerr << "[ERROR] Compilation failed." << std::endl;
        }
    }
   
    return 0;
}
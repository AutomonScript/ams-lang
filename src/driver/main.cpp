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
#include "core/compiler/SemanticAnalyzer.hpp"

using namespace antlr4;
using namespace ams;

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

    // Perform semantic analysis
    SemanticAnalyzer semanticAnalyzer;
    if (!semanticAnalyzer.analyze(ast)) {
        std::cerr << "[ERROR] Semantic analysis failed. Aborting compilation." << std::endl;
        return 1;
    }

    if (mode == "build") {
        std::string tempCpp = "temp_output.cpp";
        std::filesystem::path exePath = inputPath.filename();
        
        #ifdef _WIN32
            exePath.replace_extension(".exe");
        #elif __linux__
            exePath.replace_extension("");
        #endif

        // These brackets force the Generator to flush its buffer and close 
        // the file before moving on to the std::system compile command.
        {
            Generator generator(tempCpp);
            generator.generate(ast);
        }
       
        
        std::string compileCmd = "g++ -O2 -I .. " + tempCpp + 
                                 " -o " + exePath.string() + " -pthread -mconsole";

        if (std::system(compileCmd.c_str()) == 0) {
            std::filesystem::remove(tempCpp); 
            std::cout << "[SUCCESS] Created: " << exePath.filename().string() << std::endl;
        } else {
            std::cerr << "[ERROR] Compilation failed." << std::endl;
        }
    }
   
    return 0;
}
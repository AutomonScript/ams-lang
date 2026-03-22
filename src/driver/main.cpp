#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>

#include "antlr4-runtime.h"
#include "AMSLexer.h"
#include "AMSParser.h"   

#include "core/supports/caseCaptilizeInputStream.hpp"
#include "core/builder/AST.hpp"       
#include "core/builder/Builder.hpp"
#include "core/compiler/Generator.hpp" 


using namespace antlr4;

int main(int argc, const char* argv[]) {
    // Help Verbose for the dempnstration of Usage Options Available in AMS-Lang Engine 
    if (argc < 3) {
        std::cout << "AutomonScript Language Engine Usage:" << std::endl<<std::endl;
        std::cout << "******************** Executing .ams Programes ***************" << std::endl;
        std::cout << "ams build <file.ams> (Compile the Code to Executable)" << std::endl;
        std::cout << "**************** View AMS Engine Language Working ***********" << std::endl;
        std::cout << "ams lexer <file.ams> (View Tokens At LEXICAL ANALYSIS)" << std::endl;
        std::cout << "ams parser <file.ams> (View ParseTree At SYNTAX ANALYSIS)" << std::endl;
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

    // Using Insted of ANTLRInputStream (Extended Class of ANTLRInputStream)
    // Defined in ./include/supports/caseCaptilizeInputStream.hpp
    // Used to make Language Case Independent (Run, run, RUN, ruN are all same)
    CaseCaptilizeInputStream input(stream);

    AMSLexer lexer(&input);
    CommonTokenStream tokens(&lexer);
    AMSParser parser(&tokens);

    // Build The Parse Tree of Source Code Using ANTLR4
    tree::ParseTree* parseTree = parser.program();

    if (parser.getNumberOfSyntaxErrors() > 0) {
        std::cerr << "Syntax Error!" << std::endl;
        return 1;
    }

    // Build The Abstract Syntax Tree of  Code Using IR Builder
    Builder astBuilder;
    auto ast = std::any_cast<std::shared_ptr<ProgramNode>>(astBuilder.visit(parseTree));

  
    if (mode == "build") {
        //Compiled :  Produces an executable file
        std::string tempCpp = "temp_output.cpp";
        std::filesystem::path exePath = inputPath;
        
        #ifdef _WIN32
            exePath.replace_extension(".exe");
            std::string flag = " -mconsole";
        #elif __linux__
            exePath.replace_extension("");
            std::string flag = "";
        #endif

        Generator generator(tempCpp);
        generator.generate(ast);
        generator.close();

        // Compile to Executable
        std::string compileCmd = "g++ " + tempCpp + " -o \"" + exePath.string() + "\"" + flag;

        if (std::system(compileCmd.c_str()) == 0) {
            // Clean up intermediate code
            std::filesystem::remove(tempCpp); 
            std::cout << "[SUCCESS] Created: " << exePath.filename().string() << std::endl;
        } else {
            std::cerr << "[ERROR] Compilation failed." << std::endl;
        }
    }
    else if(mode=="lexer" || mode=="parser"){
        if(mode == "lexer"){
            tokens.fill();
            for (auto token : tokens.getTokens()) {
                std::cout << token->toString() << std::endl;
            }
        }

        else if(mode == "parser"){
            std::cout << "Program Parse Tree: " << std::endl;
            std::cout << parseTree->toStringTree(&parser) << std::endl;
        }
    }
    else {
        //Unknown Execution Mode : ERROR! Verbose
        std::cerr << "Unknown mode: " << mode << ". Use 'build'." << std::endl;
        return 1;
    }

    return 0;
}
#pragma once
#include "core/builder/AST.hpp"
#include <fstream>
#include <string>
#include <iostream>
#include <algorithm>

//::::::::::::::::::::::::::::::::: Helpers ::::::::::::::::::::::::::::::::::::
// Helper to keep C++ names in sync with capitalized Lexer
std::string normalize(std::string name) {
    std::transform(name.begin(), name.end(), name.begin(), ::toupper);
    return name;
}

//::::::::::::::::::::::::::::::::: Generator ::::::::::::::::::::::::::::::::::
class Generator : public ASTVisitor {
public:
    Generator(const std::string& outputFile) {
        out.open(outputFile);
        if (!out.is_open()) {
            std::cerr << "[ERROR] Could not open output file: " << outputFile << "\n";
        }
    }

    ~Generator() { close(); }

    //######################################### Code Generator #############################################
    void generate(std::shared_ptr<ProgramNode> program) {
        out << "#include <iostream>\n#include <string>\n#include <functional>\n\n";
        out << "#include <cmath>\n\n";
        out << "#include \"include/stdlib/io/console.hpp\"\n\n";
        
        out << "int main() {\n";
        
        // PASS 1: Define all Functions
        out << "    // --- Function & Observer Definitions ---\n";
        if (program) {
            for (const auto& stmt : program->programBlocks) {
                if (std::dynamic_pointer_cast<FunctionDefinitionNode>(stmt)) {
                    stmt->accept(this);
                }
            }
        }

        // PASS 2: Execution Logic
        out << "\n    // --- Execution Logic ---\n";
        if (program) {
            for (const auto& stmt : program->programBlocks) {
                if (!std::dynamic_pointer_cast<FunctionDefinitionNode>(stmt)) {
                    stmt->accept(this);
                }
            }
        }
        
        out << "\n    return 0;\n}\n";
    }
    
    void close() { if (out.is_open()) out.close(); }

    //######################################## Program Visitor  ############################################
    void visit(ProgramNode* node) override {
        for (const auto& stmt : node->programBlocks) stmt->accept(this);
    }
    
    //####################################### Common Statements  ###########################################
    void visit(LiteralNode* node) override {
        std::string val = node->value;
        // Translate boolean literals to C++ lowercase
        if (val == "TRUE") out << "true";
        else if (val == "FALSE") out << "false";
        else out << val;
    }

    void visit(VariableNode* node) override {
        // Output the variable name (e.g., price)
        out << node->name;
    }

    void visit(VariableDeclarationNode* node) override {
        // Map your DSL types to strict C++ types
        std::string cppType = "auto"; 
        if (node->dataType == "INT") cppType = "int";
        else if (node->dataType == "FLOAT") cppType = "double";
        else if (node->dataType == "STRING") cppType = "std::string";
        else if (node->dataType == "BOOL") cppType = "bool";

        out << "        " << cppType << " " << node->varName;
        
        // If they assigned a value (e.g. INT score = 100;)
        if (node->value) {
            out << " = ";
            node->value->accept(this); 
        }
        out << ";\n";
    }

    void visit(AssignmentNode* node) override {
        out << "        " << node->varName << " = ";
        node->expression->accept(this);
        out << ";\n";
    }
    //######################################## Global Section  #############################################
    void visit(GlobalSectionNode* node) override {
        out << "    // --- GLOBAL BLOCK ---\n";
        for (const auto& item : node->statements) item->accept(this);
    }

    void visit(ImportNode* node) override {
        out << "    // Import: " << node->moduleName << "\n";
    }

    void visit(MergeNode* node) override {
        out << "    // Merge: " << node->targetName << "\n";
    }

    //####################################### Source Definations  ##########################################
    void visit(SourceDefinitionNode* node) override {
        out << "    // Source Block: " << normalize(node->sourceName) << "\n";
        for (const auto& item : node->statements) item->accept(this); 
    }

    //######################################## Event Definations  ##########################################
    void visit(EventDefinitionNode* node) override {
        std::string name = normalize(node->eventName);
        out << "    // Event: " << name << "\n";
        out << "    auto EVENT_" << name << " = [&]() {\n";
        for (const auto& stmt : node->statements) stmt->accept(this); 
        out << "    };\n";
        out << "    EVENT_" << name << "();\n\n";
    }

    //####################################### Observer Definations  ########################################
    void visit(ObserverDefinitionNode* node) override {
        std::string name = normalize(node->observerName);
        out << "    auto OBSERVER_" << name << " = [&]() {\n"; // Use [&] to capture other functions
        for (const auto& stmt : node->statements) stmt->accept(this);
        out << "    };\n";
        out << "    OBSERVER_" << name << "();\n\n";
    }

    
    //######################################## Function Definations #########################################
    void visit(FunctionDefinitionNode* node) override {
        std::string name = normalize(node->functionName);
        
        // Write parameters into the lambda signature: auto FUNC_AAA = [&](auto x, auto y) { ... }
        out << "    auto FUNC_" << name << " = [&]( ";
        for (size_t i = 0; i < node->parameters.size(); ++i) {
            out << "auto " << node->parameters[i] << (i == node->parameters.size() - 1 ? "" : ", ");
        }
        out << ") {\n";

        for (const auto& stmt : node->statements) stmt->accept(this);
        out << "    };\n\n";
    }

    void visit(FunctionCallNode* node) override {
        std::string name = normalize(node->functionName);
    
        out << "    FUNC_" << name << "("; 

        for (size_t i = 0; i < node->arguments.size(); ++i) {
            node->arguments[i]->accept(this);
            if (i < node->arguments.size() - 1) out << ", ";
        }
        out << ");\n"; // Ensure semicolon is here for standalone calls
    }
    
    //####################################### Operators & Expressions  #########################################
    void visit(UnaryOperatorNode* node) override {
        std::string op = node->op;
        if (op == "NOT" || op == "!") op = "!";
        if (op == "SUBTRACT" || op == "MINUS") op = "-";
        
        out << "(" << op;
        node->right->accept(this);
        out << ")";
    }

    void visit(BinaryOperatorNode* node) override {
        std::string op = node->op;

        // Handle Power first (Function call)
        if (op == "^" || op == "POWER") {
            out << "std::pow(";
            node->left->accept(this);
            out << ", ";
            node->right->accept(this);
            out << ")";
            return;
        }

        out << "(";
        node->left->accept(this);
        
        // Translation Layer: AMS Alias -> C++ Operator
        if (op == "ADD" || op == "+") op = "+";
        else if (op == "SUBTRACT" || op == "-") op = "-";
        else if (op == "MULTIPLY" || op == "*") op = "*";
        else if (op == "DIVIDE" || op == "/") op = "/";
        else if (op == "REMAINDER" || op == "%") op = "%";
        else if (op == "EQUALS" || op == "==") op = "==";
        else if (op == "AND" || op == "&") op = "&&";
        else if (op == "OR" || op == "|") op = "||";
        else if (op.find("GREATER") != std::string::npos) {
            op = (op.find("EQUAL") != std::string::npos) ? ">=" : ">";
        }
        else if (op.find("LESS") != std::string::npos) {
            op = (op.find("EQUAL") != std::string::npos) ? "<=" : "<";
        }

        out << " " << op << " ";
        node->right->accept(this);
        out << ")";
    }

private:
    std::ofstream out;
};
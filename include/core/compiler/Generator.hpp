#pragma once
#include "core/builder/AST.hpp"
#include <fstream>
#include <sstream>
#include <string>
#include <iostream>
#include <algorithm>
#include <memory>

inline std::string normalize(std::string name) {
    std::transform(name.begin(), name.end(), name.begin(), ::toupper);
    return name;
}

namespace ams {

class Generator : public ASTVisitor {
public:
    Generator(const std::string& outputFile) : outPtr(&fileOut) {
        fileOut.open(outputFile);
        if (!fileOut.is_open()) {
            std::cerr << "[ERROR] Could not open output file: " << outputFile << "\n";
        }
    }

    ~Generator() { if (fileOut.is_open()) fileOut.close(); }

    void generate(std::shared_ptr<ProgramNode> program) {
<<<<<<< HEAD
        *outPtr << "#include <iostream>\n#include <string>\n#include <functional>\n#include <cmath>\n";
        *outPtr << "#include <thread>\n#include <chrono>\n"; 
        *outPtr << "#include \"include/stdlib/io/console.hpp\"\n";
        *outPtr << "#include \"include/runtime/runtime.hpp\"\n\n";
=======
        out << "#include <iostream>\n#include <string>\n#include <functional>\n\n";
        out << "#include <cmath>\n\n";
        out << "#include <stdlib/io/console.hpp>\n\n";
>>>>>>> b7b3605a0ebc96e43758260dc77452ebf3180bb7
        
        *outPtr << "int main() {\n";
        *outPtr << "    ams::Runtime rt;\n";
        
        if (program) {
            bool hasRuntime = false;
            for (const auto& stmt : program->programBlocks) {
                if (std::dynamic_pointer_cast<SourceDefinitionNode>(stmt)
                    || std::dynamic_pointer_cast<EventDefinitionNode>(stmt)
                    || std::dynamic_pointer_cast<ObserverDefinitionNode>(stmt)) {
                    hasRuntime = true;
                }
            }
            if (hasRuntime) *outPtr << "    rt.init();\n\n";
            
            // Declare global variables first
            for (const auto& stmt : program->programBlocks) {
                if (auto global = std::dynamic_pointer_cast<GlobalSectionNode>(stmt)) {
                    global->accept(this);
                }
            }
            
            // Declare functions
            for (const auto& stmt : program->programBlocks) {
                if (auto func = std::dynamic_pointer_cast<FunctionDefinitionNode>(stmt)) {
                    func->accept(this);
                }
            }
            
            for (const auto& stmt : program->programBlocks) {
                if (auto src = std::dynamic_pointer_cast<SourceDefinitionNode>(stmt))
                    emitSourceRegistration(src);
            }
            for (const auto& stmt : program->programBlocks) {
                if (auto evt = std::dynamic_pointer_cast<EventDefinitionNode>(stmt))
                    emitEventRegistration(evt);
            }
            for (const auto& stmt : program->programBlocks) {
                if (auto obs = std::dynamic_pointer_cast<ObserverDefinitionNode>(stmt))
                    emitObserverRegistration(obs);
            }
            
            *outPtr << "\n    // --- Execution Logic ---\n";
            for (const auto& stmt : program->programBlocks) {
                if (!std::dynamic_pointer_cast<FunctionDefinitionNode>(stmt)
                    && !std::dynamic_pointer_cast<SourceDefinitionNode>(stmt)
                    && !std::dynamic_pointer_cast<EventDefinitionNode>(stmt)
                    && !std::dynamic_pointer_cast<ObserverDefinitionNode>(stmt)
                    && !std::dynamic_pointer_cast<GlobalSectionNode>(stmt)) {
                    stmt->accept(this);
                }
            }
        }
        
        *outPtr << "\n    while(true) { std::this_thread::sleep_for(std::chrono::seconds(1)); }\n";
        *outPtr << "    rt.shutdown();\n    return 0;\n}\n";
    }

    // --- Mandatory ASTVisitor Overrides ---
    void visit(ProgramNode* node) override {
        for (const auto& stmt : node->programBlocks) stmt->accept(this);
    }

    void visit(LiteralNode* node) override {
        if (node->value == "TRUE") *outPtr << "true";
        else if (node->value == "FALSE") *outPtr << "false";
        else *outPtr << node->value;
    }

    void visit(VariableNode* node) override { *outPtr << node->name; }

    void visit(VariableDeclarationNode* node) override {
        std::string cppType = (node->dataType == "INT") ? "int" : 
                             (node->dataType == "FLOAT") ? "double" :
                             (node->dataType == "STRING") ? "std::string" : "bool";

        *outPtr << "        " << cppType << " " << node->varName;
        if (node->value) {
            *outPtr << " = ";
            node->value->accept(this);
        } else {
            *outPtr << ( (cppType == "std::string") ? " = \"\"" : " = 0" );
        }
        *outPtr << ";\n";

        if (node->isTrack && !currentSourceContext.empty()) {
            *outPtr << "        rt.updateVar(\"" << currentSourceContext << "\", \"" << node->varName << "\", " << node->varName << ");\n";
        }
    }

    void visit(AssignmentNode* node) override {
        *outPtr << "        " << node->varName << " = ";
        node->expression->accept(this);
        *outPtr << ";\n";
        if (!currentSourceContext.empty()) {
            *outPtr << "        rt.updateVar(\"" << currentSourceContext << "\", \"" << node->varName << "\", " << node->varName << ");\n";
        }
    }

    void visit(GlobalSectionNode* node) override {
        for (const auto& stmt : node->statements) stmt->accept(this);
    }

    void visit(ImportNode* node) override { *outPtr << "// Import " << node->moduleName << "\n"; }
    void visit(MergeNode* node) override { *outPtr << "// Merge " << node->targetName << "\n"; }
    void visit(SourceDefinitionNode* node) override {} // Handled by emit
    void visit(EventDefinitionNode* node) override {}  // Handled by emit
    void visit(ObserverDefinitionNode* node) override {} // Handled by emit

    void visit(FunctionDefinitionNode* node) override {
        *outPtr << "    auto FUNC_" << normalize(node->functionName) << " = [&]( ";
        for (size_t i = 0; i < node->parameters.size(); ++i) {
            *outPtr << "auto " << node->parameters[i] << (i == node->parameters.size() - 1 ? "" : ", ");
        }
        *outPtr << ") {\n";
        for (const auto& stmt : node->statements) stmt->accept(this);
        *outPtr << "    };\n";
    }

    void visit(FunctionCallNode* node) override {
        *outPtr << "FUNC_" << normalize(node->functionName) << "(";
        for (size_t i = 0; i < node->arguments.size(); ++i) {
            node->arguments[i]->accept(this);
            if (i < node->arguments.size() - 1) *outPtr << ", ";
        }
        *outPtr << ");\n";
    }

    void visit(UnaryOperatorNode* node) override {
        *outPtr << "(" << node->op;
        node->right->accept(this);
        *outPtr << ")";
    }

    void visit(BinaryOperatorNode* node) override {
        if (node->op == "^") {
            *outPtr << "std::pow(";
            node->left->accept(this); *outPtr << ", ";
            node->right->accept(this); *outPtr << ")";
        } else {
            *outPtr << "("; node->left->accept(this); 
            *outPtr << " " << node->op << " "; 
            node->right->accept(this); *outPtr << ")";
        }
    }

    void visit(IfStatementNode* node) override {
        for (size_t i = 0; i < node->branches.size(); ++i) {
            auto& branch = node->branches[i];
            if (i == 0) *outPtr << "        if (";
            else if (branch.condition) *outPtr << " else if (";
            else *outPtr << " else {\n";
            if (branch.condition) { branch.condition->accept(this); *outPtr << ") {\n"; }
            for (auto& stmt : branch.body) stmt->accept(this);
            *outPtr << "        }\n";
        }
    }

    void visit(TimeStatementNode* node) override {}
    void visit(DataAccessNode* node) override {
        *outPtr << "rt.getSnapshot(\"" << normalize(node->sourceName) << "\").getDouble(\"" << node->varName << "\")";
    }
    void visit(SignalNode* node) override {}

private:
    std::ofstream fileOut;
    std::ostream* outPtr;
    std::string currentSourceContext = "";

    void emitSourceRegistration(std::shared_ptr<SourceDefinitionNode> node) {
        std::string name = normalize(node->sourceName);
        std::stringstream body;
        std::ostream* saved = outPtr;
        outPtr = &body;
        currentSourceContext = name;
        for (const auto& stmt : node->statements) stmt->accept(this);
        currentSourceContext = "";
        outPtr = saved;
        int ms = (node->schedule) ? node->schedule->value : 60000;
        *outPtr << "    rt.schedulePeriodic(\"" << name << "\", " << ms << ", \"MS\", [&]() {\n" << body.str() << "    });\n";
    }

    void emitEventRegistration(std::shared_ptr<EventDefinitionNode> node) {
        std::string name = normalize(node->eventName);
        std::stringstream body;
        std::ostream* saved = outPtr;
        outPtr = &body;
        for (const auto& stmt : node->statements) stmt->accept(this);
        outPtr = saved;
        *outPtr << "    rt.setSignalCondition(\"" << name << "\", \"" << normalize(node->sourceName) << "\", [&](const ams::Snapshot& snap) {\n" << body.str();
        if (node->signalCondition) { *outPtr << "        return "; node->signalCondition->accept(this); *outPtr << ";\n"; }
        else { *outPtr << "        return true;\n"; }
        *outPtr << "    });\n";
    }

    void emitObserverRegistration(std::shared_ptr<ObserverDefinitionNode> node) {
        std::stringstream body;
        std::ostream* saved = outPtr;
        outPtr = &body;
        for (const auto& stmt : node->statements) stmt->accept(this);
        outPtr = saved;
        *outPtr << "    rt.registerObserver(\"" << normalize(node->observerName) << "\", \"" << normalize(node->observesEvent) << "\", [&](const ams::Snapshot& snap) {\n" << body.str() << "    });\n";
    }
};

} // namespace ams
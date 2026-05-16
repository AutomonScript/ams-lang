#pragma once
#include "core/builder/AST.hpp"
#include <iostream>
#include <string>
#include <map>
#include <set>
#include <memory>
#include <vector>

namespace ams {

/**
 * @brief Semantic analyzer for AMS programs
 * 
 * Performs semantic validation including:
 * - Type checking for LOG_SOURCE, LOG_DATA, LOG_RECORD
 * - Validation of LOG_SOURCE operations (e.g., insert in READ mode)
 * - Variable scope and usage validation
 */
class SemanticAnalyzer : public ASTVisitor {
public:
    SemanticAnalyzer() : hasErrors_(false) {}
    
    bool analyze(std::shared_ptr<ProgramNode> program) {
        hasErrors_ = false;
        if (program) {
            program->accept(this);
        }
        return !hasErrors_;
    }
    
    bool hasErrors() const { return hasErrors_; }
    
    // ASTVisitor overrides
    void visit(ProgramNode* node) override {
        // First pass: collect all variable declarations
        for (const auto& stmt : node->programBlocks) {
            if (auto global = std::dynamic_pointer_cast<GlobalSectionNode>(stmt)) {
                currentScope_ = "GLOBAL";
                global->accept(this);
            }
        }
        
        // Second pass: validate SOURCE blocks
        for (const auto& stmt : node->programBlocks) {
            if (auto source = std::dynamic_pointer_cast<SourceDefinitionNode>(stmt)) {
                currentScope_ = "SOURCE:" + source->sourceName;
                currentSourceName_ = source->sourceName;
                source->accept(this);
            }
        }
        
        // Third pass: validate EVENT blocks
        for (const auto& stmt : node->programBlocks) {
            if (auto event = std::dynamic_pointer_cast<EventDefinitionNode>(stmt)) {
                currentScope_ = "EVENT:" + event->eventName;
                currentEventName_ = event->eventName;
                event->accept(this);
            }
        }
        
        // Fourth pass: validate OBSERVER blocks
        for (const auto& stmt : node->programBlocks) {
            if (auto observer = std::dynamic_pointer_cast<ObserverDefinitionNode>(stmt)) {
                currentScope_ = "OBSERVER:" + observer->observerName;
                observer->accept(this);
            }
        }
    }
    
    void visit(GlobalSectionNode* node) override {
        for (const auto& stmt : node->statements) {
            stmt->accept(this);
        }
    }
    
    void visit(SourceDefinitionNode* node) override {
        for (const auto& stmt : node->statements) {
            stmt->accept(this);
        }
    }
    
    void visit(EventDefinitionNode* node) override {
        for (const auto& stmt : node->statements) {
            stmt->accept(this);
        }
    }
    
    void visit(ObserverDefinitionNode* node) override {
        for (const auto& stmt : node->statements) {
            stmt->accept(this);
        }
    }
    
    void visit(VariableDeclarationNode* node) override {
        // Track variable type
        variables_[currentScope_ + "::" + node->varName] = node->dataType;
        
        // Validate LOG_SOURCE declaration
        if (node->dataType == "LOG_SOURCE") {
            if (node->value) {
                if (auto openNode = std::dynamic_pointer_cast<LogSourceOpenNode>(node->value)) {
                    // Track LOG_SOURCE access mode
                    logSourceModes_[currentScope_ + "::" + node->varName] = openNode->accessMode;
                }
            }
        }
        
        // Validate value expression if present
        if (node->value) {
            node->value->accept(this);
        }
    }
    
    void visit(AssignmentNode* node) override {
        if (node->expression) {
            node->expression->accept(this);
        }
    }
    
    void visit(MethodCallNode* node) override {
        // Validate LOG_SOURCE method calls
        std::string varKey = currentScope_ + "::" + node->objectName;
        
        // Check if this is a LOG_SOURCE variable
        auto varIt = variables_.find(varKey);
        if (varIt != variables_.end() && varIt->second == "LOG_SOURCE") {
            // Validate insert() method is not called in READ mode
            if (node->methodName == "insert") {
                auto modeIt = logSourceModes_.find(varKey);
                if (modeIt != logSourceModes_.end() && modeIt->second == "READ") {
                    reportError("Cannot call insert() on LOG_SOURCE opened in READ mode: " + node->objectName);
                }
            }
            
            // Validate method exists
            if (node->methodName != "filter" && 
                node->methodName != "match" && 
                node->methodName != "hasUpdated" && 
                node->methodName != "insert" &&
                node->methodName != "getFilePath" &&
                node->methodName != "getMode" &&
                node->methodName != "getRecordCount" &&
                node->methodName != "close" &&
                node->methodName != "isOpen") {
                reportError("Unknown LOG_SOURCE method: " + node->methodName);
            }
        }
        
        // Check if this is a LOG_DATA variable
        if (varIt != variables_.end() && varIt->second == "LOG_DATA") {
            // Validate LOG_DATA methods
            if (node->methodName != "filter" && 
                node->methodName != "match" && 
                node->methodName != "count" &&
                node->methodName != "isSingle" &&
                node->methodName != "isEmpty" &&
                node->methodName != "first" &&
                node->methodName != "toVector") {
                reportError("Unknown LOG_DATA method: " + node->methodName);
            }
        }
        
        // Validate arguments
        for (const auto& arg : node->arguments) {
            arg->accept(this);
        }
    }
    
    void visit(DataAccessNode* node) override {
        // Validate LOG_RECORD field access
        if (node->varName == "timestamp" || node->varName == "level" || node->varName == "message") {
            if (node->sourceName != "SOURCE" && node->sourceName != "EVENT") {
                // Check if the object is a LOG_RECORD
                std::string varKey = currentScope_ + "::" + node->sourceName;
                auto varIt = variables_.find(varKey);
                if (varIt != variables_.end() && varIt->second != "LOG_RECORD") {
                    reportError("Cannot access field '" + node->varName + "' on non-LOG_RECORD variable: " + node->sourceName);
                }
            }
        }
    }
    
    void visit(BinaryOperatorNode* node) override {
        if (node->left) node->left->accept(this);
        if (node->right) node->right->accept(this);
    }
    
    void visit(UnaryOperatorNode* node) override {
        if (node->right) node->right->accept(this);
    }
    
    void visit(IfStatementNode* node) override {
        for (auto& branch : node->branches) {
            if (branch.condition) {
                branch.condition->accept(this);
            }
            for (auto& stmt : branch.body) {
                stmt->accept(this);
            }
        }
    }
    
    void visit(FunctionDefinitionNode* node) override {
        std::string savedScope = currentScope_;
        currentScope_ = "FUNCTION:" + node->functionName;
        
        for (const auto& stmt : node->statements) {
            stmt->accept(this);
        }
        
        currentScope_ = savedScope;
    }
    
    void visit(FunctionCallNode* node) override {
        for (const auto& arg : node->arguments) {
            arg->accept(this);
        }
    }
    
    // Stub implementations for other visitors
    void visit(ImportNode* node) override {}
    void visit(MergeNode* node) override {}
    void visit(LiteralNode* node) override {}
    void visit(VariableNode* node) override {}
    void visit(TimeStatementNode* node) override {}
    void visit(SignalNode* node) override {
        if (node->condition) {
            node->condition->accept(this);
        }
    }
    void visit(LogSourceOpenNode* node) override {}
    
private:
    bool hasErrors_;
    std::string currentScope_;
    std::string currentSourceName_;
    std::string currentEventName_;
    
    // Variable name -> type mapping
    std::map<std::string, std::string> variables_;
    
    // LOG_SOURCE variable name -> access mode (READ/WRITE)
    std::map<std::string, std::string> logSourceModes_;
    
    void reportError(const std::string& message) {
        std::cerr << "[SEMANTIC ERROR] " << message << " (in " << currentScope_ << ")" << std::endl;
        hasErrors_ = true;
    }
};

} // namespace ams

#pragma once
#include "core/builder/AST.hpp"
#include <fstream>
#include <sstream>
#include <string>
#include <iostream>
#include <algorithm>
#include <memory>
#include <map>
#include <vector>
#include <set>

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
        // First pass: collect SOURCE-EVENT and EVENT-OBSERVER relationships
        collectRelationships(program);
        
        // Generate includes
        *outPtr << "#include <iostream>\n#include <string>\n#include <functional>\n#include <cmath>\n";
        *outPtr << "#include <thread>\n#include <chrono>\n#include <map>\n"; 
        *outPtr << "#include \"include/stdlib/io/console.hpp\"\n";
        *outPtr << "#include \"include/runtime/runtime.hpp\"\n";
        *outPtr << "#include \"include/stdlib/source/log_source.hpp\"\n";
        *outPtr << "#include \"include/stdlib/source/log_data.hpp\"\n";
        *outPtr << "#include \"include/stdlib/source/log_record.hpp\"\n";
        *outPtr << "#include \"include/stdlib/source/filter_criteria.hpp\"\n\n";
        
        *outPtr << "using namespace ams::stdlib::source;\n\n";
        
        *outPtr << "int main() {\n";
        *outPtr << "    ams::Runtime rt;\n";
        *outPtr << "    auto& trackCopyManager = rt.getTrackCopyManager();\n";
        *outPtr << "    auto& eventBus = rt.getEventBus();\n";
        *outPtr << "    auto& scheduler = rt.getScheduler();\n\n";
        
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
            
            // Declare global variables
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
            
            // Register EVENT handlers (must be before SOURCE scheduling)
            for (const auto& stmt : program->programBlocks) {
                if (auto evt = std::dynamic_pointer_cast<EventDefinitionNode>(stmt))
                    emitEventHandler(evt);
            }
            
            // Register OBSERVER handlers
            for (const auto& stmt : program->programBlocks) {
                if (auto obs = std::dynamic_pointer_cast<ObserverDefinitionNode>(stmt))
                    emitObserverHandler(obs);
            }
            
            // Schedule SOURCEs
            for (const auto& stmt : program->programBlocks) {
                if (auto src = std::dynamic_pointer_cast<SourceDefinitionNode>(stmt))
                    emitSourceScheduling(src);
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
        std::string cppType = getCppType(node->dataType);
        
        // Handle LOG_SOURCE declarations specially
        if (node->dataType == "LOG_SOURCE" && node->value) {
            // Generate LOG_SOURCE open with error handling
            *outPtr << "        auto " << node->varName << "_result = ";
            node->value->accept(this);
            *outPtr << ";\n";
            
            *outPtr << "        if (" << node->varName << "_result.isErr()) {\n";
            *outPtr << "            std::cerr << \"Failed to open LOG_SOURCE: \" << ";
            *outPtr << node->varName << "_result.getErrorMessage() << std::endl;\n";
            *outPtr << "            return 1;\n";
            *outPtr << "        }\n";
            
            // TRACK variables in SOURCE should be static to persist across invocations
            if (node->isTrack) {
                *outPtr << "        static " << cppType << " " << node->varName;
            } else {
                *outPtr << "        " << cppType << " " << node->varName;
            }
            *outPtr << " = std::move(" << node->varName << "_result.unwrap());\n";
            return;
        }
        
        // TRACK variables in SOURCE should be static to persist across invocations
        if (node->isTrack) {
            *outPtr << "        static " << cppType << " " << node->varName;
        } else {
            *outPtr << "        " << cppType << " " << node->varName;
        }
        
        if (node->value) {
            *outPtr << " = ";
            node->value->accept(this);
        } else {
            *outPtr << getDefaultValue(cppType);
        }
        *outPtr << ";\n";
    }

    void visit(AssignmentNode* node) override {
        *outPtr << "        " << node->varName << " = ";
        node->expression->accept(this);
        *outPtr << ";\n";
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
        // Handle LOG_RECORD field access (record.timestamp, record.level, record.message)
        if (node->varName == "timestamp" || node->varName == "level" || node->varName == "message") {
            // This is likely a LOG_RECORD field access
            if (node->sourceName != "SOURCE" && node->sourceName != "EVENT") {
                // Direct field access on a LOG_RECORD variable
                if (node->varName == "timestamp") {
                    *outPtr << node->sourceName << ".getTimestampString()";
                } else if (node->varName == "level") {
                    *outPtr << node->sourceName << ".getLevelString()";
                } else if (node->varName == "message") {
                    *outPtr << node->sourceName << ".getMessage()";
                }
                return;
            }
        }
        
        // Handle SOURCE/EVENT data access
        if (currentSignalContext.empty()) {
            // Fallback: shouldn't happen in properly generated code
            *outPtr << "0";
            return;
        }
        
        std::string accessor;
        if (node->sourceName == "SOURCE") {
            accessor = currentSignalContext + "->getSourceInt";
        } else if (node->sourceName == "EVENT") {
            accessor = currentSignalContext + "->getEventInt";
        } else {
            *outPtr << "0";
            return;
        }
        
        // For now, assume INT type (proper implementation would use expectedType)
        *outPtr << accessor << "(\"" << node->varName << "\")";
    }
    
    void visit(SignalNode* node) override {
        // SIGNAL code generation is handled in emitSourceScheduling and emitEventHandler
    }
    
    void visit(LogSourceOpenNode* node) override {
        *outPtr << "LOG_SOURCE::open(\"" << node->filepath << "\", AccessMode::" << node->accessMode << ")";
    }
    
    void visit(MethodCallNode* node) override {
        // Handle special LOG_SOURCE method calls that need FilterCriteria construction
        if (node->methodName == "filter" && node->arguments.size() == 1) {
            // Check if argument is a function call like byLevel(ERROR)
            if (auto funcCall = std::dynamic_pointer_cast<FunctionCallNode>(node->arguments[0])) {
                *outPtr << node->objectName << ".filter(FilterCriteria::" << funcCall->functionName << "(";
                for (size_t i = 0; i < funcCall->arguments.size(); ++i) {
                    // Handle LogLevel enum values
                    if (auto varNode = std::dynamic_pointer_cast<VariableNode>(funcCall->arguments[i])) {
                        if (varNode->name == "DEBUG" || varNode->name == "INFO" || 
                            varNode->name == "WARN" || varNode->name == "ERROR" || 
                            varNode->name == "FATAL") {
                            *outPtr << "LogLevel::" << varNode->name;
                        } else {
                            funcCall->arguments[i]->accept(this);
                        }
                    } else {
                        funcCall->arguments[i]->accept(this);
                    }
                    if (i < funcCall->arguments.size() - 1) *outPtr << ", ";
                }
                *outPtr << "))";
                return;
            }
        }
        
        // Standard method call
        *outPtr << node->objectName << "." << node->methodName << "(";
        
        for (size_t i = 0; i < node->arguments.size(); ++i) {
            node->arguments[i]->accept(this);
            if (i < node->arguments.size() - 1) *outPtr << ", ";
        }
        
        *outPtr << ")";
    }

private:
    std::ofstream fileOut;
    std::ostream* outPtr;
    std::string currentSignalContext = "";  // Name of SignalContext variable
    std::map<std::string, std::vector<std::string>> sourceToEvents;
    std::map<std::string, std::vector<std::string>> eventToObservers;
    
    std::string getCppType(const std::string& amsType) {
        if (amsType == "INT") return "int";
        if (amsType == "FLOAT") return "double";
        if (amsType == "STRING") return "std::string";
        if (amsType == "BOOL") return "bool";
        if (amsType == "LOG_SOURCE") return "LOG_SOURCE";
        if (amsType == "LOG_DATA") return "LOG_DATA";
        if (amsType == "LOG_RECORD") return "LOG_RECORD";
        return "int";
    }
    
    std::string getDefaultValue(const std::string& cppType) {
        if (cppType == "std::string") return " = \"\"";
        return " = 0";
    }
    
    void collectRelationships(std::shared_ptr<ProgramNode> program) {
        for (const auto& stmt : program->programBlocks) {
            if (auto evt = std::dynamic_pointer_cast<EventDefinitionNode>(stmt)) {
                sourceToEvents[normalize(evt->sourceName)].push_back(normalize(evt->eventName));
            }
            if (auto obs = std::dynamic_pointer_cast<ObserverDefinitionNode>(stmt)) {
                eventToObservers[normalize(obs->observesEvent)].push_back(normalize(obs->observerName));
            }
        }
    }
    
    std::vector<std::string> getTrackVariables(std::shared_ptr<SourceDefinitionNode> node) {
        std::vector<std::string> trackVars;
        for (const auto& stmt : node->statements) {
            if (auto varDecl = std::dynamic_pointer_cast<VariableDeclarationNode>(stmt)) {
                if (varDecl->isTrack) {
                    trackVars.push_back(varDecl->varName);
                }
            }
        }
        return trackVars;
    }
    
    std::vector<std::string> getShareableEventVariables(std::shared_ptr<EventDefinitionNode> node) {
        std::vector<std::string> shareableVars;
        for (const auto& stmt : node->statements) {
            if (auto varDecl = std::dynamic_pointer_cast<VariableDeclarationNode>(stmt)) {
                if (!varDecl->isUnshare) {
                    shareableVars.push_back(varDecl->varName);
                }
            }
        }
        return shareableVars;
    }
    
    bool hasSignalStatement(const std::vector<std::shared_ptr<ASTNode>>& statements) {
        for (const auto& stmt : statements) {
            if (std::dynamic_pointer_cast<SignalNode>(stmt)) {
                return true;
            }
        }
        return false;
    }
    
    std::shared_ptr<SignalNode> getSignalNode(const std::vector<std::shared_ptr<ASTNode>>& statements) {
        for (const auto& stmt : statements) {
            if (auto signal = std::dynamic_pointer_cast<SignalNode>(stmt)) {
                return signal;
            }
        }
        return nullptr;
    }
    
    void collectUsedVariables(const std::vector<std::shared_ptr<ASTNode>>& statements, std::set<std::string>& vars) {
        for (const auto& stmt : statements) {
            if (auto varNode = std::dynamic_pointer_cast<VariableNode>(stmt)) {
                vars.insert(varNode->name);
            } else if (auto assignNode = std::dynamic_pointer_cast<AssignmentNode>(stmt)) {
                vars.insert(assignNode->varName);
                collectUsedVariablesInExpr(assignNode->expression, vars);
            } else if (auto signalNode = std::dynamic_pointer_cast<SignalNode>(stmt)) {
                if (signalNode->condition) {
                    collectUsedVariablesInExpr(signalNode->condition, vars);
                }
            }
        }
    }
    
    void collectUsedVariablesInExpr(std::shared_ptr<ASTNode> expr, std::set<std::string>& vars) {
        if (!expr) return;
        
        if (auto varNode = std::dynamic_pointer_cast<VariableNode>(expr)) {
            vars.insert(varNode->name);
        } else if (auto binOp = std::dynamic_pointer_cast<BinaryOperatorNode>(expr)) {
            collectUsedVariablesInExpr(binOp->left, vars);
            collectUsedVariablesInExpr(binOp->right, vars);
        } else if (auto unOp = std::dynamic_pointer_cast<UnaryOperatorNode>(expr)) {
            collectUsedVariablesInExpr(unOp->right, vars);
        }
    }
    
    void emitSourceScheduling(std::shared_ptr<SourceDefinitionNode> node) {
        std::string name = normalize(node->sourceName);
        int ms = (node->schedule) ? node->schedule->value : 60000;
        
        *outPtr << "    // SOURCE: " << name << "\n";
        *outPtr << "    scheduler.schedulePeriodic(\"" << name << "\", " << ms << ", [&]() {\n";
        
        // Emit SOURCE body
        std::stringstream body;
        std::ostream* saved = outPtr;
        outPtr = &body;
        
        for (const auto& stmt : node->statements) {
            if (std::dynamic_pointer_cast<SignalNode>(stmt)) {
                // Handle SIGNAL statement
                auto signal = std::dynamic_pointer_cast<SignalNode>(stmt);
                auto trackVars = getTrackVariables(node);
                
                *outPtr << "        // SIGNAL statement\n";
                *outPtr << "        {\n";
                
                // Generate condition check
                if (signal->condition) {
                    *outPtr << "            bool __signal_condition = ";
                    signal->condition->accept(this);
                    *outPtr << ";\n";
                } else {
                    *outPtr << "            bool __signal_condition = true;\n";
                }
                
                *outPtr << "            if (__signal_condition) {\n";
                
                // Capture TRACK variables
                *outPtr << "                std::map<std::string, ams::Snapshot::Value> __track_vars;\n";
                for (const auto& varName : trackVars) {
                    *outPtr << "                __track_vars[\"" << varName << "\"] = " << varName << ";\n";
                }
                
                // Create TRACK copy
                *outPtr << "                std::string __context_id = trackCopyManager.createTrackCopy(\n";
                *outPtr << "                    \"" << name << "\",\n";
                *outPtr << "                    __track_vars\n";
                *outPtr << "                );\n";
                
                // Publish to EventBus for each attached EVENT with context ID
                auto it = sourceToEvents.find(name);
                if (it != sourceToEvents.end()) {
                    // Add references for each EVENT (they will all release it)
                    for (size_t i = 0; i < it->second.size(); ++i) {
                        if (i > 0) {  // First event gets the initial reference
                            *outPtr << "                trackCopyManager.addReference(__context_id);\n";
                        }
                    }
                    // Publish to each EVENT
                    for (const auto& eventName : it->second) {
                        *outPtr << "                eventBus.publish(\"" << eventName << "\", {\"" << name << "\", \"" << eventName << "\", 0, __context_id});\n";
                    }
                }
                
                *outPtr << "            }\n";
                *outPtr << "        }\n";
            } else {
                stmt->accept(this);
            }
        }
        
        outPtr = saved;
        *outPtr << body.str();
        *outPtr << "    });\n\n";
    }
    
    void emitEventHandler(std::shared_ptr<EventDefinitionNode> node) {
        std::string name = normalize(node->eventName);
        std::string sourceName = normalize(node->sourceName);
        
        *outPtr << "    // EVENT: " << name << " ON " << sourceName << "\n";
        *outPtr << "    eventBus.subscribe(\"" << name << "\", \"" << name << "_handler\", [&](const ams::EventPayload& __payload) {\n";
        *outPtr << "        // Get SignalContext from payload\n";
        *outPtr << "        auto __signal_context = trackCopyManager.getContext(__payload.contextId);\n";
        *outPtr << "        if (!__signal_context) return;\n\n";
        
        currentSignalContext = "__signal_context";
        
        // Emit EVENT body
        std::stringstream body;
        std::ostream* saved = outPtr;
        outPtr = &body;
        
        for (const auto& stmt : node->statements) {
            if (std::dynamic_pointer_cast<SignalNode>(stmt)) {
                // Handle SIGNAL in EVENT
                auto signal = std::dynamic_pointer_cast<SignalNode>(stmt);
                auto shareableVars = getShareableEventVariables(node);
                
                *outPtr << "        // SIGNAL statement in EVENT\n";
                *outPtr << "        {\n";
                
                if (signal->condition) {
                    *outPtr << "            bool __signal_condition = ";
                    signal->condition->accept(this);
                    *outPtr << ";\n";
                } else {
                    *outPtr << "            bool __signal_condition = true;\n";
                }
                
                *outPtr << "            if (__signal_condition) {\n";
                
                // Capture EVENT variables
                *outPtr << "                std::map<std::string, ams::Snapshot::Value> __event_vars;\n";
                for (const auto& varName : shareableVars) {
                    *outPtr << "                __event_vars[\"" << varName << "\"] = " << varName << ";\n";
                }
                
                // Create EVENT context with transitive SOURCE data
                *outPtr << "                std::string __new_context_id = trackCopyManager.createEventContext(\n";
                *outPtr << "                    \"" << name << "\",\n";
                *outPtr << "                    __event_vars,\n";
                *outPtr << "                    __payload.contextId\n";
                *outPtr << "                );\n";
                
                // Publish to OBSERVERs with new context ID
                auto it = eventToObservers.find(name);
                if (it != eventToObservers.end()) {
                    // Add references for each OBSERVER (they will all release it)
                    for (size_t i = 0; i < it->second.size(); ++i) {
                        if (i > 0) {  // First observer gets the initial reference
                            *outPtr << "                trackCopyManager.addReference(__new_context_id);\n";
                        }
                    }
                    // Publish to each OBSERVER
                    for (const auto& observerName : it->second) {
                        *outPtr << "                eventBus.publish(\"" << observerName << "\", {\"" << name << "\", \"" << observerName << "\", 0, __new_context_id});\n";
                    }
                }
                
                *outPtr << "            }\n";
                *outPtr << "        }\n";
            } else {
                stmt->accept(this);
            }
        }
        
        outPtr = saved;
        *outPtr << body.str();
        
        currentSignalContext = "";
        
        *outPtr << "        trackCopyManager.releaseContext(__payload.contextId);\n";
        *outPtr << "    });\n\n";
    }
    
    void emitObserverHandler(std::shared_ptr<ObserverDefinitionNode> node) {
        std::string name = normalize(node->observerName);
        std::string eventName = normalize(node->observesEvent);
        
        *outPtr << "    // OBSERVER: " << name << " OBSERVS " << eventName << "\n";
        *outPtr << "    eventBus.subscribe(\"" << name << "\", \"" << name << "_handler\", [&](const ams::EventPayload& __payload) {\n";
        *outPtr << "        // Get SignalContext from payload\n";
        *outPtr << "        auto __signal_context = trackCopyManager.getContext(__payload.contextId);\n";
        *outPtr << "        if (!__signal_context) return;\n\n";
        
        currentSignalContext = "__signal_context";
        
        std::stringstream body;
        std::ostream* saved = outPtr;
        outPtr = &body;
        
        for (const auto& stmt : node->statements) {
            stmt->accept(this);
        }
        
        outPtr = saved;
        *outPtr << body.str();
        
        currentSignalContext = "";
        
        *outPtr << "        trackCopyManager.releaseContext(__payload.contextId);\n";
        *outPtr << "    });\n\n";
    }
};

} // namespace ams

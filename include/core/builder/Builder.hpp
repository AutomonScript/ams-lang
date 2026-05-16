#pragma once
#include "AMSBaseVisitor.h"
#include "AST.hpp"
#include <iostream>
#include <memory>
#include <any>
#include <vector> // FIX: Required for std::vector casting in conditional blocks
#include <string> // FIX: Required for string manipulation

//:::::::::::::::::::::::::::::::::::::::::: Helpers :::::::::::::::::::::::::::::::::::::::::::::::::

template<typename T>
std::shared_ptr<T> get_node(antlrcpp::Any res) {
    if (!res.has_value()) return nullptr;

    try {
        return std::any_cast<std::shared_ptr<T>>(res);
    } 
    catch (const std::bad_any_cast&) {
        try {
            auto basePtr = std::any_cast<std::shared_ptr<ASTNode>>(res);
            return std::dynamic_pointer_cast<T>(basePtr);
        } 
        catch (const std::bad_any_cast&) {
            return nullptr;
        }
    }
}

//:::::::::::::::::::::::::::::::::::::::::: Builder ::::::::::::::::::::::::::::::::::::::::::::::::::

class Builder : public AMSBaseVisitor {
public:
    //################################### Program Visitor #############################################
    virtual antlrcpp::Any visitProgram(AMSParser::ProgramContext *ctx) override {
        auto program = std::make_shared<ProgramNode>();
        
        if (ctx->globalSection()) {
            if (auto nodePtr = get_node<ASTNode>(visit(ctx->globalSection())))
                program->programBlocks.push_back(nodePtr);
        }
        
        if (ctx->sourcesSection()) {
            for (auto* d : ctx->sourcesSection()->sourceDefinition())
                if (auto nodePtr = get_node<ASTNode>(visit(d))) program->programBlocks.push_back(nodePtr);
        }

        if (ctx->eventsSection()) {
            for (auto* d : ctx->eventsSection()->eventDefinition())
                if (auto nodePtr = get_node<ASTNode>(visit(d))) program->programBlocks.push_back(nodePtr);
        }

        if (ctx->observersSection()) {
            for (auto* d : ctx->observersSection()->observerDefinition())
                if (auto nodePtr = get_node<ASTNode>(visit(d))) program->programBlocks.push_back(nodePtr);
        }
        
        if (ctx->functionsSection()) {
            for (auto* d : ctx->functionsSection()->functionDefinition())
                if (auto nodePtr = get_node<ASTNode>(visit(d))) program->programBlocks.push_back(nodePtr);
        }

        return program;
    }

    //#################################### Global Section  ###########################################
    virtual antlrcpp::Any visitGlobalSection(AMSParser::GlobalSectionContext *ctx) override {
        auto node = std::make_shared<GlobalSectionNode>();
        for (auto* item : ctx->globalItem()) {
            if (auto ptr = get_node<ASTNode>(visit(item))) node->statements.push_back(ptr);
        }
        return std::static_pointer_cast<ASTNode>(node);
    }

    virtual antlrcpp::Any visitGlobalItem(AMSParser::GlobalItemContext *ctx) override {
        if (ctx->importStatement()) return visit(ctx->importStatement());
        if (ctx->mergeStatement())  return visit(ctx->mergeStatement());
        return visit(ctx->statement());
    }

    virtual antlrcpp::Any visitImportStatement(AMSParser::ImportStatementContext *ctx) override {
        std::string s = ctx->STRING_L()->getText();
        return std::static_pointer_cast<ASTNode>(std::make_shared<ImportNode>(s.substr(1, s.length()-2)));
    }

    virtual antlrcpp::Any visitMergeStatement(AMSParser::MergeStatementContext *ctx) override {
        std::string s = ctx->STRING_L()->getText();
        return std::static_pointer_cast<ASTNode>(std::make_shared<MergeNode>(s.substr(1, s.length()-2)));
    }

    //##################################### Source Definitions  #######################################
    virtual antlrcpp::Any visitSourceDefinition(AMSParser::SourceDefinitionContext *ctx) override {
        auto node = std::make_shared<SourceDefinitionNode>(ctx->ID()->getText());
        
        if (ctx->sourceScheduleStatement()) {
            if (auto sched = get_node<TimeStatementNode>(visit(ctx->sourceScheduleStatement()))) {
                node->schedule = sched;
            }
        }
        
        for (auto* item : ctx->sourceItem()) {
            if (auto ptr = get_node<ASTNode>(visit(item))) node->statements.push_back(ptr);
        }
        return std::static_pointer_cast<ASTNode>(node);
    }

    virtual antlrcpp::Any visitSourceScheduleStatement(AMSParser::SourceScheduleStatementContext *ctx) override {
        if (ctx->CHECK() && ctx->timeStatement()) {
            return visit(ctx->timeStatement());
        }
        return std::static_pointer_cast<ASTNode>(std::make_shared<TimeStatementNode>(TimeStatementNode::Type::DEFAULT));
    }

    virtual antlrcpp::Any visitTimeStatement(AMSParser::TimeStatementContext *ctx) override {
        if (ctx->EVERY()) {
            int value = std::stoi(ctx->INT_L()->getText());
            std::string unit = ctx->timeUnit()->getText();
            return std::static_pointer_cast<ASTNode>(
                std::make_shared<TimeStatementNode>(TimeStatementNode::Type::EVERY, value, unit)
            );
        }
        else if (ctx->AT_KW()) {
            std::string timeLit = ctx->TIME_LITERAL()->getText();
            int hours = std::stoi(timeLit.substr(0, 2));
            int minutes = std::stoi(timeLit.substr(3, 2));
            int totalMinutes = hours * 60 + minutes;
            return std::static_pointer_cast<ASTNode>(
                std::make_shared<TimeStatementNode>(TimeStatementNode::Type::AT, totalMinutes, "MIN")
            );
        }
        else if (ctx->CONTINUOUSLY()) {
            return std::static_pointer_cast<ASTNode>(
                std::make_shared<TimeStatementNode>(TimeStatementNode::Type::CONTINUOUSLY)
            );
        }
        return std::static_pointer_cast<ASTNode>(
            std::make_shared<TimeStatementNode>(TimeStatementNode::Type::DEFAULT)
        );
    }

    virtual antlrcpp::Any visitSourceItem(AMSParser::SourceItemContext *ctx) override {
        if (ctx->sourceVariableDeclaration()) return visit(ctx->sourceVariableDeclaration());
        if (ctx->assignment()) return visit(ctx->assignment());
        if (ctx->functionCall()) return visit(ctx->functionCall());
        if (ctx->conditionalStatements()) return visit(ctx->conditionalStatements());
        
        // Handle SIGNAL statement in SOURCE
        if (ctx->SIGNAL()) {
            std::shared_ptr<ASTNode> cond = nullptr;
            if (ctx->expression()) {
                cond = get_node<ASTNode>(visit(ctx->expression()));
            }
            return std::static_pointer_cast<ASTNode>(std::make_shared<SignalNode>(cond));
        }
        
        return antlrcpp::Any();
    }

    virtual antlrcpp::Any visitSourceVariableDeclaration(AMSParser::SourceVariableDeclarationContext *ctx) override {
        std::string type = ctx->dataType()->getText();
        std::string name = ctx->ID()->getText();
        std::shared_ptr<ASTNode> val = nullptr;
        bool track = ctx->TRACK() != nullptr;

        if (ctx->EQUAL()) {
            val = get_node<ASTNode>(visit(ctx->expression()));
        }
        auto node = std::make_shared<VariableDeclarationNode>(type, name, val);
        node->isTrack = track;
        return std::static_pointer_cast<ASTNode>(node);
    }

    //##################################### Events Definitions  #######################################
    virtual antlrcpp::Any visitEventDefinition(AMSParser::EventDefinitionContext *ctx) override {
        auto node = std::make_shared<EventDefinitionNode>(ctx->ID()->getText());
        
        if (ctx->eventScheduleStatement()) {
            auto schedCtx = ctx->eventScheduleStatement();
            node->sourceName = schedCtx->ID()->getText();
            if (schedCtx->SIGNAL() && schedCtx->expression()) {
                node->signalCondition = get_node<ASTNode>(visit(schedCtx->expression()));
            }
        }
        
        for (auto* item : ctx->eventItem()) {
            if (auto ptr = get_node<ASTNode>(visit(item))) node->statements.push_back(ptr);
        }
        
        // Don't filter out SignalNode - Generator needs it in statements list
        
        return std::static_pointer_cast<ASTNode>(node);
    }

    virtual antlrcpp::Any visitEventItem(AMSParser::EventItemContext *ctx) override {
        if (ctx->eventVariableDeclaration()) return visit(ctx->eventVariableDeclaration());
        if (ctx->assignment()) return visit(ctx->assignment());
        if (ctx->functionCall()) return visit(ctx->functionCall());
        if (ctx->conditionalStatements()) return visit(ctx->conditionalStatements());
        
        // Handle SIGNAL statement in EVENT
        if (ctx->SIGNAL()) {
            std::shared_ptr<ASTNode> cond = nullptr;
            if (ctx->expression()) {
                cond = get_node<ASTNode>(visit(ctx->expression()));
            }
            return std::static_pointer_cast<ASTNode>(std::make_shared<SignalNode>(cond));
        }
        
        return antlrcpp::Any();
    }

    virtual antlrcpp::Any visitEventVariableDeclaration(AMSParser::EventVariableDeclarationContext *ctx) override {
        std::string type = ctx->dataType()->getText();
        std::string name = ctx->ID()->getText();
        std::shared_ptr<ASTNode> val = nullptr;
        bool unshare = ctx->UNSHARE() != nullptr;

        if (ctx->EQUAL()) {
            val = get_node<ASTNode>(visit(ctx->expression()));
        }
        auto node = std::make_shared<VariableDeclarationNode>(type, name, val);
        node->isUnshare = unshare;
        return std::static_pointer_cast<ASTNode>(node);
    }

    //#################################### Observers Definitions  #####################################
    virtual antlrcpp::Any visitObserverDefinition(AMSParser::ObserverDefinitionContext *ctx) override {
        auto node = std::make_shared<ObserverDefinitionNode>(ctx->ID()->getText());
        
        if (ctx->observerScheduleStatement()) {
            node->observesEvent = ctx->observerScheduleStatement()->ID()->getText();
        }
        
        for (auto* item : ctx->observerItem()) {
            if (auto ptr = get_node<ASTNode>(visit(item))) node->statements.push_back(ptr);
        }
        return std::static_pointer_cast<ASTNode>(node);
    }

    virtual antlrcpp::Any visitObserverItem(AMSParser::ObserverItemContext *ctx) override { return visit(ctx->statement()); }

    //##################################### Function Definitions  #####################################
    virtual antlrcpp::Any visitFunctionDefinition(AMSParser::FunctionDefinitionContext *ctx) override {
        std::string name = ctx->ID()->getText();
        std::vector<std::string> params;

        if (ctx->parameters()) {
            for (auto* paramCtx : ctx->parameters()->parameter()) {
                params.push_back(paramCtx->ID()->getText());
            }
        }

        auto node = std::make_shared<FunctionDefinitionNode>(name, params);

        for (auto* item : ctx->functionItem()) {
            if (auto nodePtr = get_node<ASTNode>(visit(item))) {
                node->statements.push_back(nodePtr);
            }
        }

        return std::static_pointer_cast<ASTNode>(node);
    }

    virtual antlrcpp::Any visitFunctionCall(AMSParser::FunctionCallContext *ctx) override {
        std::string name = ctx->ID()->getText();
        std::vector<std::shared_ptr<ASTNode>> args;

        if (ctx->arguments()) {
            for (auto* exprCtx : ctx->arguments()->expression()) {
                if (auto argNode = get_node<ASTNode>(visit(exprCtx))) {
                    args.push_back(argNode);
                }
            }
        }
        return std::static_pointer_cast<ASTNode>(std::make_shared<FunctionCallNode>(name, args));
    }

    //########################################## Statements & Expressions #################################
    virtual antlrcpp::Any visitStatement(AMSParser::StatementContext *ctx) override {
        if (ctx->functionCall()) return visit(ctx->functionCall());
        if (ctx->variableDeclaration()) return visit(ctx->variableDeclaration());
        if (ctx->assignment()) return visit(ctx->assignment());
        if (ctx->conditionalStatements()) return visit(ctx->conditionalStatements());
        return antlrcpp::Any();
    }

    virtual antlrcpp::Any visitVariableDeclaration(AMSParser::VariableDeclarationContext *ctx) override {
        std::string type = ctx->dataType()->getText();
        std::string name = ctx->ID()->getText();
        std::shared_ptr<ASTNode> val = nullptr;

        if (ctx->EQUAL()) {
            val = get_node<ASTNode>(visit(ctx->expression()));
        }
        return std::static_pointer_cast<ASTNode>(std::make_shared<VariableDeclarationNode>(type, name, val));
    }

    virtual antlrcpp::Any visitAssignment(AMSParser::AssignmentContext *ctx) override {
        std::string name = ctx->ID()->getText();
        auto val = get_node<ASTNode>(visit(ctx->expression()));
        return std::static_pointer_cast<ASTNode>(std::make_shared<AssignmentNode>(name, val));
    }

    virtual antlrcpp::Any visitExpression(AMSParser::ExpressionContext *ctx) override {
        // 1. Parentheses
        if (ctx->LPAREN()) {
            return visit(ctx->expression(0));
        }

        // 2. LOG_SOURCE Open
        if (ctx->logSourceOpen()) {
            return visit(ctx->logSourceOpen());
        }

        // 3. Method Call (object.method())
        if (ctx->methodCall()) {
            return visit(ctx->methodCall());
        }

        // 4. Function Call
        if (ctx->functionCall()) {
            return visit(ctx->functionCall());
        }

        // 5. Data Access (source.var)
        if (ctx->dataAccess()) {
            return visit(ctx->dataAccess());
        }

        // 6. Unary Operator
        if (ctx->expression().size() == 1 && ctx->op) {
            std::string op = ctx->op->getText();
            auto right = get_node<ASTNode>(visit(ctx->expression(0)));
            return std::static_pointer_cast<ASTNode>(std::make_shared<UnaryOperatorNode>(op, right));
        }

        // 7. Binary Operator
        if (ctx->expression().size() == 2) {
            auto left = get_node<ASTNode>(visit(ctx->expression(0)));
            std::string op = ctx->op->getText();
            auto right = get_node<ASTNode>(visit(ctx->expression(1)));
            return std::static_pointer_cast<ASTNode>(std::make_shared<BinaryOperatorNode>(left, op, right));
        }

        // 8. Variables
        if (ctx->ID()) {
            auto node = std::make_shared<VariableNode>(ctx->getText());
            return std::static_pointer_cast<ASTNode>(node);
        }

        // 9. Literals
        auto litNode = std::make_shared<LiteralNode>(ctx->getText());
        return std::static_pointer_cast<ASTNode>(litNode);
    }

    virtual antlrcpp::Any visitDataAccess(AMSParser::DataAccessContext *ctx) override {
        std::string sourceName;
        if (ctx->SOURCE()) {
            sourceName = "SOURCE";
        } else if (ctx->EVENT()) {
            sourceName = "EVENT";
        } else {
            sourceName = ctx->ID(0)->getText();
        }
        std::string varName = ctx->ID(ctx->ID().size() - 1)->getText();  // Last ID is the variable name
        return std::static_pointer_cast<ASTNode>(std::make_shared<DataAccessNode>(sourceName, varName));
    }

    //##################################### Conditional Statements ####################################
    virtual antlrcpp::Any visitConditionalStatements(AMSParser::ConditionalStatementsContext *ctx) override {
        auto node = std::make_shared<IfStatementNode>();
        
        // 1. Handle the main 'IF' branch
        ConditionalBranch ifBranch;
        ifBranch.condition = get_node<ASTNode>(visit(ctx->expression(0)));
        ifBranch.body = std::any_cast<std::vector<std::shared_ptr<ASTNode>>>(visit(ctx->conditionalBlock(0)));
        node->branches.push_back(ifBranch);
        
        // 2. Handle all 'ELSE_IF' branches
        for (size_t i = 0; i < ctx->ELSE_IF().size(); ++i) {
            ConditionalBranch eiBranch;
            eiBranch.condition = get_node<ASTNode>(visit(ctx->expression(i + 1)));
            eiBranch.body = std::any_cast<std::vector<std::shared_ptr<ASTNode>>>(visit(ctx->conditionalBlock(i + 1)));
            node->branches.push_back(eiBranch);
        }
        
        // 3. Handle the final 'ELSE' branch
        if (ctx->ELSE()) {
            ConditionalBranch elseBranch;
            elseBranch.condition = nullptr;
            elseBranch.body = std::any_cast<std::vector<std::shared_ptr<ASTNode>>>(visit(ctx->conditionalBlock().back()));
            node->branches.push_back(elseBranch);
        }
        
        return std::static_pointer_cast<ASTNode>(node);
    }

    virtual antlrcpp::Any visitConditionalBlock(AMSParser::ConditionalBlockContext *ctx) override {
        std::vector<std::shared_ptr<ASTNode>> statements;
        
        for (auto* stmtCtx : ctx->statement()) {
            if (auto ptr = get_node<ASTNode>(visit(stmtCtx))) {
                statements.push_back(ptr);
            }
        }
        
        return statements;
    }

    virtual antlrcpp::Any visitFunctionItem(AMSParser::FunctionItemContext *ctx) override { return visit(ctx->statement()); }

    //##################################### LOG_SOURCE Visitors #######################################
    virtual antlrcpp::Any visitLogSourceOpen(AMSParser::LogSourceOpenContext *ctx) override {
        std::string filepath = ctx->STRING_L()->getText();
        // Remove quotes from string literal
        filepath = filepath.substr(1, filepath.length() - 2);
        
        std::string mode = "READ";  // Default
        if (ctx->READ()) {
            mode = "READ";
        } else if (ctx->WRITE()) {
            mode = "WRITE";
        }
        
        return std::static_pointer_cast<ASTNode>(std::make_shared<LogSourceOpenNode>(filepath, mode));
    }

    virtual antlrcpp::Any visitMethodCall(AMSParser::MethodCallContext *ctx) override {
        std::string objectName = ctx->ID(0)->getText();
        std::string methodName = ctx->ID(1)->getText();
        std::vector<std::shared_ptr<ASTNode>> args;

        if (ctx->arguments()) {
            for (auto* exprCtx : ctx->arguments()->expression()) {
                if (auto argNode = get_node<ASTNode>(visit(exprCtx))) {
                    args.push_back(argNode);
                }
            }
        }
        
        return std::static_pointer_cast<ASTNode>(std::make_shared<MethodCallNode>(objectName, methodName, args));
    }
};
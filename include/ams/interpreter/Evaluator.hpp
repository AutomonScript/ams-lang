#pragma once
#include "AMSBaseVisitor.h"
#include <iostream>
#include <string>

class Evaluator : public AMSBaseVisitor {
public:
    // Handle the entry point
    virtual antlrcpp::Any visitProgram(AMSParser::ProgramContext *ctx) override {
        return visitChildren(ctx);
    }

    // Handle CONSOLE("...") - Executes IMMEDIATELY
    virtual antlrcpp::Any visitStatement(AMSParser::StatementContext *ctx) override {
        if (ctx->STRING()) {
            std::string text = ctx->STRING()->getText();
            // Remove quotes: "Hello" =>  Hello
            if (text.length() >= 2) {
                text = text.substr(1, text.length() - 2);
            }
            std::cout << text << std::endl;
        }
        return nullptr;
    }

    // Handle WATCH blocks for live registration
    virtual antlrcpp::Any visitWatchItem(AMSParser::WatchItemContext *ctx) override {
        std::string varName = ctx->ID(1)->getText();
        std::string url = ctx->STRING()->getText();
        std::cout << "[System] Initializing  Watcher for: " << varName << std::endl;
        return nullptr;
    }
};
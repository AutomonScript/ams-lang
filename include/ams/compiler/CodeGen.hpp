#pragma once
#include "AMSBaseVisitor.h"
#include <fstream>
#include <string>

class CodeGen : public AMSBaseVisitor {
public:
    CodeGen(const std::string& outputFile) : out(outputFile) {}
    
    // Ensure these signatures match the .cpp exactly
    virtual antlrcpp::Any visitProgram(AMSParser::ProgramContext *ctx) override;
    virtual antlrcpp::Any visitWatchItem(AMSParser::WatchItemContext *ctx) override;
    virtual antlrcpp::Any visitStatement(AMSParser::StatementContext *ctx) override;

    void close() { if (out.is_open()) out.close(); }

private:
    std::ofstream out;
};
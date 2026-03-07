#include "ams/compiler/CodeGen.hpp"

antlrcpp::Any CodeGen::visitProgram(AMSParser::ProgramContext *ctx) {
    out << "#include <iostream>\n";
    out << "#include <string>\n\n";
    out << "int main() {\n";
    
    antlrcpp::Any result = visitChildren(ctx);
    
    out << "    return 0;\n";
    out << "}\n";
    return result;
}

antlrcpp::Any CodeGen::visitWatchItem(AMSParser::WatchItemContext *ctx) {
    if (!ctx->ID().empty() && ctx->STRING()) {
        std::string resourceType = ctx->ID(0)->getText();
        std::string variableName = ctx->ID(1)->getText();
        std::string url = ctx->STRING()->getText();
        out << "    std::cout << \"[System] Initializing " << resourceType << " Watcher for: " << variableName << "\" << std::endl;\n";
    }
    return nullptr;
}

antlrcpp::Any CodeGen::visitStatement(AMSParser::StatementContext *ctx) {
    if (ctx->STRING()) {
        out << "    std::cout << " << ctx->STRING()->getText() << " << std::endl;\n";
    }
    return nullptr; 
}
#pragma once
#include <string>
#include <vector>
#include <memory>
#include <utility>
//############################# AST Node Declaration  ##################################
class ProgramNode;

class GlobalSectionNode;
class ImportNode;
class MergeNode;

class EventDefinitionNode;

class SourceDefinitionNode; 

class ObserverDefinitionNode;

class FunctionDefinitionNode;
class FunctionCallNode;

class LiteralNode;
class VariableNode;

class VariableDeclarationNode;
class AssignmentNode;

class UnaryOperatorNode;
class BinaryOperatorNode;
//############################## Visitor Interfacs #####################################
class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;
    virtual void visit(ProgramNode* node) = 0;
    
    virtual void visit(GlobalSectionNode* node) = 0;
    virtual void visit(ImportNode* node) = 0;
    virtual void visit(MergeNode* node) = 0;

    virtual void visit(EventDefinitionNode* node) = 0;

    virtual void visit(SourceDefinitionNode* node) = 0;

    virtual void visit(ObserverDefinitionNode* node) = 0;

    virtual void visit(FunctionDefinitionNode* node) = 0;
    virtual void visit(FunctionCallNode* node) = 0;

    virtual void visit(LiteralNode* node) = 0;
    virtual void visit(VariableNode* node) = 0;

    virtual void visit(VariableDeclarationNode* node) = 0;
    virtual void visit(AssignmentNode* node) = 0;

    virtual void visit(UnaryOperatorNode* node) = 0;
    virtual void visit(BinaryOperatorNode* node) = 0;
};
//############################# AST Node Base Class ####################################
class ASTNode {
public:
    virtual ~ASTNode() = default;
    virtual void accept(ASTVisitor* visitor) = 0; 
};

//############################# Node Implementation ####################################

class ProgramNode : public ASTNode {
public:
    std::vector<std::shared_ptr<ASTNode>> programBlocks;
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

//--------------------------------------------------------------------------------------
class LiteralNode : public ASTNode {
public:
    std::string value;
    LiteralNode(std::string v) : value(std::move(v)) {}
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

class VariableNode : public ASTNode {
public:
    std::string name;
    VariableNode(std::string n) : name(std::move(n)) {}
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

class VariableDeclarationNode : public ASTNode {
public:
    std::string dataType;
    std::string varName;
    std::shared_ptr<ASTNode> value; 

    VariableDeclarationNode(std::string type, std::string name, std::shared_ptr<ASTNode> val = nullptr) 
        : dataType(std::move(type)), varName(std::move(name)), value(std::move(val)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

class AssignmentNode : public ASTNode {
public:
    std::string varName;
    std::shared_ptr<ASTNode> expression;

    AssignmentNode(std::string name, std::shared_ptr<ASTNode> expr) 
        : varName(std::move(name)), expression(std::move(expr)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};
//------------------------------- GLOBAL SECTION ---------------------------------------
class GlobalSectionNode : public ASTNode {
public:
    std::vector<std::shared_ptr<ASTNode>> statements;
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

class ImportNode : public ASTNode {
public:
    std::string moduleName;
    ImportNode(std::string name) : moduleName(std::move(name)) {}
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

class MergeNode : public ASTNode {
public:
    std::string targetName;
    MergeNode(std::string name) : targetName(std::move(name)) {}
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

//------------------------------ SOURCES SECTION ---------------------------------------
class SourceDefinitionNode : public ASTNode {
public:
    std::string sourceName;
    std::vector<std::shared_ptr<ASTNode>> statements;
    SourceDefinitionNode(std::string name) : sourceName(std::move(name)) {}
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

//------------------------------ EVENTS SECTION ---------------------------------------
class EventDefinitionNode : public ASTNode {
public:
    std::string eventName;
    std::vector<std::shared_ptr<ASTNode>> statements;
    EventDefinitionNode(std::string name) : eventName(std::move(name)) {}
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

//----------------------------- OBSERVERS SECTION -------------------------------------
class ObserverDefinitionNode : public ASTNode {
public:
    std::string observerName;
    std::vector<std::shared_ptr<ASTNode>> statements;
    ObserverDefinitionNode(std::string name) : observerName(std::move(name)) {}
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

//----------------------------- FUNCTIONS SECTION -------------------------------------
class FunctionDefinitionNode : public ASTNode {
public:
    std::string functionName;
    
    std::vector<std::string> parameters; 
    std::vector<std::shared_ptr<ASTNode>> statements;

    FunctionDefinitionNode(std::string name, std::vector<std::string> params = {}) 
        : functionName(std::move(name)), parameters(std::move(params)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

class FunctionCallNode : public ASTNode {
public:
    std::string functionName;
    // A list of expressions passed to the function
    std::vector<std::shared_ptr<ASTNode>> arguments;

    // Updated Constructor
    FunctionCallNode(std::string name, std::vector<std::shared_ptr<ASTNode>> args = {}) 
        : functionName(std::move(name)), arguments(std::move(args)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

//--------------------------- OPERATOR NODES -----------------------------------

class UnaryOperatorNode : public ASTNode {
public:
    std::string op;
    std::shared_ptr<ASTNode> right;

    UnaryOperatorNode(std::string o, std::shared_ptr<ASTNode> r) 
        : op(std::move(o)), right(std::move(r)) {}

    void accept(ASTVisitor* visitor) override { 
        visitor->visit(this); 
    }
};

class BinaryOperatorNode : public ASTNode {
public:
    std::shared_ptr<ASTNode> left;
    std::string op;
    std::shared_ptr<ASTNode> right;

    BinaryOperatorNode(std::shared_ptr<ASTNode> l, std::string o, std::shared_ptr<ASTNode> r) 
        : left(std::move(l)), op(std::move(o)), right(std::move(r)) {}

    void accept(ASTVisitor* visitor) override { 
        visitor->visit(this); 
    }
};

//--------------------------------------------------------------------------------------


#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <vector>

// ============================================================================
// AST.hpp  (Árvore Sintática Abstrata)
// ----------------------------------------------------------------------------
// Unidade 2 do pipeline: a *saída* do parser e a *entrada* do analisador
// semântico. É o contrato que desacopla sintaxe de semântica:
//
//   Lexer --(Token)--> Parser --(ProgramASTNode)--> SemanticAnalyzer
//
// Todas as fases posteriores (semântica, geração de código) trabalham sobre
// esta árvore e nunca mais veem o texto-fonte nem os tokens.
//
// Hierarquia:
//   ASTNode
//    ├── ExprNode
//    │    ├── IntASTNode / StringASTNode / BoolASTNode / ObjectASTNode
//    │    ├── AssignASTNode / BinaryOpASTNode / UnaryOpASTNode / NewASTNode
//    │    ├── DispatchASTNode / LocalCallASTNode
//    │    ├── IfASTNode / WhileASTNode / BlockASTNode / LetASTNode / CaseASTNode
//    ├── FormalASTNode
//    ├── FeatureASTNode
//    ├── ClassASTNode
//    └── ProgramASTNode
// ============================================================================

class ASTNode {
public:
    int line = 0;  // Linha do código onde o nó foi criado (para erros)
    virtual ~ASTNode() = default;
    virtual void print(int indent = 0) const = 0;  // Para visualização da árvore
};

// --- NÓS DE EXPRESSÃO ---
class ExprNode : public ASTNode {};

// Escapa \n, \t etc. para que a impressão da árvore caiba em uma linha
// (senão uma string com quebra de linha "quebra" o desenho da árvore).
inline std::string escapeForPrint(const std::string& s) {
    std::string out;
    for (char c : s) {
        switch (c) {
            case '\n': out += "\\n"; break;
            case '\t': out += "\\t"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\\': out += "\\\\"; break;
            case '"':  out += "\\\""; break;
            default:   out += c; break;
        }
    }
    return out;
}

// --- TERMINAIS E LITERAIS ---
class IntASTNode : public ExprNode {
public:
    int value;
    explicit IntASTNode(int val) : value(val) {}
    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ') << "Int(" << value << ")\n";
    }
};

class StringASTNode : public ExprNode {
public:
    std::string value;
    explicit StringASTNode(std::string v) : value(std::move(v)) {}
    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ') << "String(\"" << escapeForPrint(value) << "\")\n";
    }
};

class BoolASTNode : public ExprNode {
public:
    bool value;
    explicit BoolASTNode(bool v) : value(v) {}
    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ') << "Bool(" << (value ? "true" : "false") << ")\n";
    }
};

// Identificador: variável, atributo, parâmetro ou self
class ObjectASTNode : public ExprNode {
public:
    std::string name;
    explicit ObjectASTNode(std::string n) : name(std::move(n)) {}
    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ') << "Object(" << name << ")\n";
    }
};

// --- OPERAÇÕES E ATRIBUIÇÃO ---

// Atribuição (ex: x <- expr)
class AssignASTNode : public ExprNode {
public:
    std::string name;
    std::shared_ptr<ExprNode> value;
    AssignASTNode(std::string n, std::shared_ptr<ExprNode> v)
        : name(std::move(n)), value(std::move(v)) {}
    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ') << "Assign(" << name << " <-)\n";
        if (value) value->print(indent + 2);
    }
};

// Operação Binária (ex: a + b)
class BinaryOpASTNode : public ExprNode {
public:
    std::string op;
    std::shared_ptr<ExprNode> left;
    std::shared_ptr<ExprNode> right;

    BinaryOpASTNode(std::string o, std::shared_ptr<ExprNode> l, std::shared_ptr<ExprNode> r)
        : op(std::move(o)), left(std::move(l)), right(std::move(r)) {}

    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ') << "BinaryOp(" << op << ")\n";
        if (left) left->print(indent + 2);
        if (right) right->print(indent + 2);
    }
};

class UnaryOpASTNode : public ExprNode {
public:
    std::string op;
    std::shared_ptr<ExprNode> operand;
    UnaryOpASTNode(std::string o, std::shared_ptr<ExprNode> opnd)
        : op(std::move(o)), operand(std::move(opnd)) {}
    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ') << "UnaryOp(" << op << ")\n";
        if (operand) operand->print(indent + 2);
    }
};

class NewASTNode : public ExprNode {
public:
    std::string typeName;
    explicit NewASTNode(std::string t) : typeName(std::move(t)) {}
    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ') << "New(" << typeName << ")\n";
    }
};

// --- DISPATCH E CHAMADAS ---

// Dispatch dinâmico: expr[@Type].metodo(args)
class DispatchASTNode : public ExprNode {
public:
    std::shared_ptr<ExprNode> expr;
    std::string typeExpr;  // Para dispatch estático @TYPE (pode ser vazio)
    std::string name;
    std::vector<std::shared_ptr<ExprNode>> args;

    DispatchASTNode(std::shared_ptr<ExprNode> e, std::string t, std::string n,
                    std::vector<std::shared_ptr<ExprNode>> a)
        : expr(std::move(e)), typeExpr(std::move(t)), name(std::move(n)), args(std::move(a)) {}

    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ') << "Dispatch(" << name
                  << (typeExpr.empty() ? "" : " @" + typeExpr) << ")\n";
        if (expr) expr->print(indent + 2);
        for (const auto& arg : args) arg->print(indent + 2);
    }
};

// Chamada local id(args) = self.id(args)
// (o dispatch estático de verdade, e@T.id(), vive em DispatchASTNode::typeExpr)
class LocalCallASTNode : public ExprNode {
public:
    std::string name;
    std::vector<std::shared_ptr<ExprNode>> args;
    LocalCallASTNode(std::string n, std::vector<std::shared_ptr<ExprNode>> a)
        : name(std::move(n)), args(std::move(a)) {}
    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ') << "LocalCall(" << name << ")\n";
        for (const auto& arg : args) arg->print(indent + 2);
    }
};

// --- CONTROLE DE FLUXO ---

class IfASTNode : public ExprNode {
public:
    std::shared_ptr<ExprNode> cond, thenExpr, elseExpr;
    IfASTNode(std::shared_ptr<ExprNode> c, std::shared_ptr<ExprNode> t, std::shared_ptr<ExprNode> e)
        : cond(std::move(c)), thenExpr(std::move(t)), elseExpr(std::move(e)) {}
    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ') << "If\n";
        if (cond) cond->print(indent + 2);
        if (thenExpr) thenExpr->print(indent + 2);
        if (elseExpr) elseExpr->print(indent + 2);
    }
};

class WhileASTNode : public ExprNode {
public:
    std::shared_ptr<ExprNode> cond, body;
    WhileASTNode(std::shared_ptr<ExprNode> c, std::shared_ptr<ExprNode> b)
        : cond(std::move(c)), body(std::move(b)) {}
    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ') << "While\n";
        if (cond) cond->print(indent + 2);
        if (body) body->print(indent + 2);
    }
};

class BlockASTNode : public ExprNode {
public:
    std::vector<std::shared_ptr<ExprNode>> expressions;
    explicit BlockASTNode(std::vector<std::shared_ptr<ExprNode>> exprs)
        : expressions(std::move(exprs)) {}
    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ') << "Block\n";
        for (const auto& e : expressions) e->print(indent + 2);
    }
};

// Binding individual do Let
struct LetBinding {
    std::string name;
    std::string type;
    std::shared_ptr<ExprNode> init;
};

class LetASTNode : public ExprNode {
public:
    std::vector<LetBinding> bindings;
    std::shared_ptr<ExprNode> body;
    LetASTNode(std::vector<LetBinding> b, std::shared_ptr<ExprNode> bodyExpr)
        : bindings(std::move(b)), body(std::move(bodyExpr)) {}
    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ') << "Let\n";
        for (const auto& bind : bindings) {
            std::cout << std::string(indent + 2, ' ') << "Binding(" << bind.name << " : "
                      << bind.type << ")\n";
            if (bind.init) bind.init->print(indent + 4);
        }
        if (body) body->print(indent + 2);
    }
};

struct CaseBranch {
    std::string name;
    std::string type;
    std::shared_ptr<ExprNode> expr;
};

class CaseASTNode : public ExprNode {
public:
    std::shared_ptr<ExprNode> expr;
    std::vector<CaseBranch> branches;
    CaseASTNode(std::shared_ptr<ExprNode> e, std::vector<CaseBranch> b)
        : expr(std::move(e)), branches(std::move(b)) {}
    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ') << "Case\n";
        if (expr) expr->print(indent + 2);
        for (const auto& br : branches) {
            std::cout << std::string(indent + 2, ' ') << "Branch(" << br.name << " : "
                      << br.type << ")\n";
            if (br.expr) br.expr->print(indent + 4);
        }
    }
};

// --- NÓS DE ESTRUTURA ---

// Nó para Parâmetros Formais dos métodos: ID : TYPE
class FormalASTNode : public ASTNode {
public:
    std::string name;
    std::string type;

    FormalASTNode(std::string n, std::string t) : name(std::move(n)), type(std::move(t)) {}

    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ') << "Formal(" << name << " : " << type << ")\n";
    }
};

// Representa um recurso (método ou atributo) de uma classe
class FeatureASTNode : public ASTNode {
public:
    std::string name;
    std::vector<std::shared_ptr<FormalASTNode>> formals;
    std::string returnType;
    std::shared_ptr<ExprNode> body;
    std::shared_ptr<ExprNode> initExpr;
    bool isMethod;

    // Construtor para MÉTODOS
    FeatureASTNode(std::string n, std::vector<std::shared_ptr<FormalASTNode>> f,
                   std::string retType, std::shared_ptr<ExprNode> b)
        : name(std::move(n)), formals(std::move(f)), returnType(std::move(retType)),
          body(std::move(b)), initExpr(nullptr), isMethod(true) {}

    // Construtor para ATRIBUTOS
    FeatureASTNode(std::string n, std::string type, std::shared_ptr<ExprNode> init)
        : name(std::move(n)), returnType(std::move(type)), body(nullptr),
          initExpr(std::move(init)), isMethod(false) {}

    void print(int indent = 0) const override {
        if (isMethod) {
            std::cout << std::string(indent, ' ') << "Method(" << name << " : " << returnType
                      << ")\n";
            for (const auto& f : formals) {
                f->print(indent + 2);
            }
            if (body) body->print(indent + 2);
        } else {
            std::cout << std::string(indent, ' ') << "Attribute(" << name << " : " << returnType
                      << ")\n";
            if (initExpr) initExpr->print(indent + 2);
        }
    }
};

// Representa uma Classe do COOL
class ClassASTNode : public ASTNode {
public:
    std::string name;
    std::string parent;
    std::vector<std::shared_ptr<FeatureASTNode>> features;

    ClassASTNode(std::string n, std::string p) : name(std::move(n)), parent(std::move(p)) {}

    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ') << "Class(" << name << " inherits " << parent
                  << ")\n";
        for (const auto& feat : features) {
            feat->print(indent + 2);
        }
    }
};

// Nó Raiz do Programa
class ProgramASTNode : public ASTNode {
public:
    std::vector<std::shared_ptr<ClassASTNode>> classes;

    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ') << "Program\n";
        for (const auto& cls : classes) {
            cls->print(indent + 2);
        }
    }
};

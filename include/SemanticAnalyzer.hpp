#pragma once

#include <memory>
#include <queue>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "AST.hpp"

// ============================================================================
// SemanticAnalyzer.hpp  (Análise Semântica)
// ----------------------------------------------------------------------------
// Responsabilidade única: receber a AST já construída e verificar as regras de
// tipos de COOL (herança, escopos, conformidade, SELF_TYPE, LUB, assinaturas).
//
// Diferença essencial em relação às versões antigas: NÃO recebe o texto-fonte
// e NÃO constrói um parser próprio. Ele recebe a árvore:
//
//   SemanticAnalyzer sem(ast);   bool ok = sem.analyze();
//
// Antes, `semantico sem(conteudo)` lexava e parseava o mesmo arquivo de novo,
// então o programa era analisado duas vezes para nada.
//
// Fluxo interno (3 passagens, todas sobre a AST):
//   1. buildInheritanceGraph -> registra classes + valida pais e ciclos
//   2. collectAllSymbols     -> coleta atributos/métodos em ordem topológica
//   3. checkTypes            -> percorre corpos inferindo tipos e checando
// ============================================================================

// Tabela de símbolos por escopos empilhados (escopo atual -> global)
class SymbolTable {
private:
    // Lista de escopos (do mais interno ao mais externo)
    std::vector<std::unordered_map<std::string, std::string>> scopes;

public:
    void enterScope();
    void exitScope();

    // Adiciona uma variável ao escopo atual. false = redeclaração no mesmo escopo.
    bool addSymbol(const std::string& name, const std::string& type);

    // Procura uma variável do escopo atual até o escopo global. "" = não encontrado.
    std::string lookup(const std::string& name) const;
};

class SemanticAnalyzer {
private:
    std::shared_ptr<ProgramASTNode> astRoot;
    bool traceEnabled = false;
    bool errorFlag = false;
    int errorCount = 0;  // usado pelo --trace para marcar cada feature

    // Tabela de classes registradas no programa
    std::unordered_map<std::string, std::shared_ptr<ClassASTNode>> classTable;

    // Representação de um método coletado
    struct MethodSymbol {
        std::string name;
        std::string returnType;
        std::vector<std::pair<std::string, std::string>> formals;  // {nome_param, tipo_param}
        std::shared_ptr<FeatureASTNode> node;
    };

    // Representação de um atributo coletado
    struct AttributeSymbol {
        std::string name;
        std::string type;
        std::shared_ptr<FeatureASTNode> node;
    };

    // Tabelas globais de interface: [NomeClasse] -> [NomeMembro -> Símbolo]
    std::unordered_map<std::string, std::unordered_map<std::string, AttributeSymbol>>
        classAttributes;
    std::unordered_map<std::string, std::unordered_map<std::string, MethodSymbol>> classMethods;

    SymbolTable symbolTable;  // Instância única reutilizada durante o percurso da AST

    void logError(int line, const std::string& msg);

    // Passagem 1
    void installBasicClasses();
    void buildInheritanceGraph();

    // Passagem 2
    bool isAttributeInAncestors(const std::string& className, const std::string& attrName);
    MethodSymbol* findMethodInAncestors(const std::string& className,
                                        const std::string& methodName);
    void collectSymbolsForClass(const std::shared_ptr<ClassASTNode>& cls);
    void collectAllSymbols();

    // Passagem 3
    std::string lookupAttributeInHierarchy(const std::string& className,
                                           const std::string& attrName);
    void populateClassScope(const std::string& className);
    std::string resolveType(const std::string& type, const std::string& currentClass) const;
    bool isSubtype(std::string child, std::string parent, const std::string& currentClass);
    std::string lub(std::string type1, std::string type2, const std::string& currentClass);
    std::string checkExprType(const std::shared_ptr<ExprNode>& expr,
                              const std::string& currentClass);
    void checkTypes();

public:
    explicit SemanticAnalyzer(std::shared_ptr<ProgramASTNode> ast, bool trace = false)
        : astRoot(std::move(ast)), traceEnabled(trace) {}

    bool analyze();

    // Imprime a interface coletada (classes, atributos e métodos) para a
    // demonstração/inspeção. Usado pelo modo --trace do main.
    void printSummary() const;
};

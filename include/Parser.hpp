#pragma once

#include <memory>

#include "AST.hpp"
#include "Lexer.hpp"
#include "Token.hpp"

// ============================================================================
// Parser.hpp  (Análise Sintática)
// ----------------------------------------------------------------------------
// Responsabilidade única: consumir Tokens e produzir a AST.
//
// Descida recursiva: cada regra da gramática de COOL vira um método com o
// mesmo nome (program, classe, feature, formal, expr...). A hierarquia de
// expr_* implementa a precedência, do mais fraco ao mais forte:
//
//   expr        <-  ID <- expr | expr_not          (atribuição, à direita)
//   expr_not    <-  not expr_not | expr_cmp
//   expr_cmp    <-  expr_add [ (< | <= | =) expr_add ]   (não associativo)
//   expr_add    <-  expr_mult { (+|-) expr_mult }        (à esquerda)
//   expr_mult   <-  expr_unary { (*|/) expr_unary }      (à esquerda)
//   expr_unary  <-  ~ expr_unary | isvoid expr_unary | new TYPE | expr_dispatch
//   expr_dispatch <- expr_primary { [@TYPE] . id(args) }
//   expr_primary  <- if | while | { } | let | case | ID | literal | ( expr )
//
// Diferença em relação às três versões antigas: o Parser NÃO cria o próprio
// Lexer a partir do texto — ele recebe um Lexer pronto (injeção de
// dependência). Assim o fluxo de dados é explícito no main:
//
//   Lexer lexer(src);  Parser parser(lexer);  auto ast = parser.parse();
// ============================================================================

class Parser {
private:
    Lexer& lexer;
    Token currentToken;
    Token nextTokenBuffer;
    bool hasBufferedToken = false;
    std::shared_ptr<ProgramASTNode> astRoot;  // Raiz da árvore de sintaxe abstrata
    bool errorFlag = false;                   // Indica se ocorreu um erro

    // O lookahead de 1 token mora aqui, não no Lexer. readToken() também
    // centraliza o tratamento de TokenType::ERROR: reporta a causa léxica e
    // pula o token inválido, em vez de deixar o erro se multiplicar pelas
    // regras gramaticais.
    Token readToken();
    void reportLexicalError(const Token& token);

    void advance();
    Token peekNext();

    // TYPEID é o caso comum; SELF_TYPE também é um tipo válido em algumas
    // posições (new, retorno de método, let e atributo). O parser aceita os
    // dois e deixa a semântica decidir se aquela posição é legal.
    bool isTypeToken() const {
        return currentToken.type == TokenType::TYPEID ||
               currentToken.type == TokenType::SELF_TYPE;
    }

    // Recuperação simples (panic mode): depois de um erro em uma classe,
    // avança até a próxima declaração 'class' ou o fim do arquivo.
    void synchronizeToClassOrEOF();

public:
    explicit Parser(Lexer& lex) : lexer(lex) {}

    // Executa a análise sintática e devolve a AST (nullptr se houve erro).
    // Deixou de ser feita no construtor: efeito colateral escondido no
    // construtor impedia o main de decidir a ordem das fases.
    std::shared_ptr<ProgramASTNode> parse();

    bool valid() const { return !errorFlag; }

    std::shared_ptr<ProgramASTNode> program();
    std::shared_ptr<ClassASTNode> classe();
    std::shared_ptr<FeatureASTNode> feature();
    std::shared_ptr<FormalASTNode> formal();

    std::shared_ptr<ExprNode> expr();
    std::shared_ptr<ExprNode> expr_not();
    std::shared_ptr<ExprNode> expr_cmp();
    std::shared_ptr<ExprNode> expr_add();
    std::shared_ptr<ExprNode> expr_mult();
    std::shared_ptr<ExprNode> expr_unary();
    std::shared_ptr<ExprNode> expr_dispatch();
    std::shared_ptr<ExprNode> expr_primary();
};

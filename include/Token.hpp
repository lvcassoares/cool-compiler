#pragma once

#include <string>

// ============================================================================
// Token.hpp
// ----------------------------------------------------------------------------
// Unidade 1 do pipeline: a *saída* do analisador léxico e a *entrada* do
// analisador sintático. Só existe um TokenType para todo o compilador deste
// projeto: se ele mudar, muda num único lugar.
// ============================================================================

// Tipos de Tokens da linguagem COOL
enum class TokenType {
    // Palavras-chave
    KEYWORD,

    // Identificadores e Literais
    TYPEID,     // Nomes de Tipos/Classes (ex: Main, Int) -> Começam com maiúscula
    OBJECTID,   // Nomes de Variáveis/Métodos (ex: x, main) -> Começam com minúscula
    INT,        // Números (ex: 42)
    STRING,     // Texto (ex: "hello")
    BOOLEAN,    // true / false

    // Símbolos e Operadores
    TIL,               // ~
    MENORQ,            // <
    IGUAL,             // =
    ATRIBUIR,          // <-
    SETAD,             // =>
    SETAE,             // <=
    MAIS,              // +
    MENOS,             // -
    ASTERISCO,         // *
    BARRA,             // /
    ABREPARENTESES,    // (
    FECHAPARENTESES,   // )
    ABRECHAVES,        // {
    FECHACHAVES,       // }
    DOISPONTOS,        // :
    PONTOEVIRGULA,     // ;
    PONTO,             // .
    VIRGULA,           // ,
    ARROBA,            // @

    // self e SELF_TYPE
    SELF,       // self
    SELF_TYPE,  // SELF_TYPE

    // Especiais
    END_OF_FILE,  // Fim do arquivo
    ERROR         // Caractere ou sequência inválida
};

// Estrutura que guarda os dados de cada token encontrado
struct Token {
    TokenType type;
    std::string lexeme;  // O texto exato encontrado no código
    int line;            // Linha onde o token foi encontrado (para erros)
};

// Nome legível do token, usado apenas em depuração/impressão (main --tokens).
// Não tem papel na análise: serve para a apresentação e para os testes.
inline const char* tokenTypeName(TokenType type) {
    switch (type) {
        case TokenType::KEYWORD:        return "KEYWORD";
        case TokenType::TYPEID:         return "TYPEID";
        case TokenType::OBJECTID:       return "OBJECTID";
        case TokenType::INT:            return "INT";
        case TokenType::STRING:         return "STRING";
        case TokenType::BOOLEAN:        return "BOOLEAN";
        case TokenType::TIL:            return "TIL";
        case TokenType::MENORQ:         return "MENORQ";
        case TokenType::IGUAL:          return "IGUAL";
        case TokenType::ATRIBUIR:       return "ATRIBUIR";
        case TokenType::SETAD:          return "SETAD";
        case TokenType::SETAE:          return "SETAE";
        case TokenType::MAIS:           return "MAIS";
        case TokenType::MENOS:          return "MENOS";
        case TokenType::ASTERISCO:      return "ASTERISCO";
        case TokenType::BARRA:          return "BARRA";
        case TokenType::ABREPARENTESES: return "ABREPARENTESES";
        case TokenType::FECHAPARENTESES:return "FECHAPARENTESES";
        case TokenType::ABRECHAVES:     return "ABRECHAVES";
        case TokenType::FECHACHAVES:    return "FECHACHAVES";
        case TokenType::DOISPONTOS:     return "DOISPONTOS";
        case TokenType::PONTOEVIRGULA:  return "PONTOEVIRGULA";
        case TokenType::PONTO:          return "PONTO";
        case TokenType::VIRGULA:        return "VIRGULA";
        case TokenType::ARROBA:         return "ARROBA";
        case TokenType::SELF:           return "SELF";
        case TokenType::SELF_TYPE:      return "SELF_TYPE";
        case TokenType::END_OF_FILE:    return "END_OF_FILE";
        case TokenType::ERROR:          return "ERROR";
    }
    return "?";
}

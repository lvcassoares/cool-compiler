#pragma once

#include <string>

#include "Token.hpp"

// ============================================================================
// Lexer.hpp  (Análise Léxica)
// ----------------------------------------------------------------------------
// Responsabilidade única: transformar texto em Tokens, um por vez, via
// nextToken(). Não conhece árvore sintática nem tipos.
//
// A interface é intencionalmente mínima: nextToken() consome e devolve o
// próximo token. O lookahead de 1 token (necessário para distinguir
// "ID <- expr" de "ID ...") mora no Parser, que guarda currentToken e
// nextToken; assim o Lexer tem uma única porta.
// ============================================================================

class Lexer {
private:
    std::string source;
    size_t cursor = 0;
    int line = 1;

    // Verifica o caractere sem avançar o cursor. Devolve -1 no fim do arquivo
    // (e não '\0'), para distinguir EOF de um byte NUL real no meio do texto.
    int peek() const {
        if (cursor >= source.length()) return -1;
        return static_cast<unsigned char>(source[cursor]);
    }

    // Retorna o caractere atual e avança o cursor. -1 = fim do arquivo.
    int advance() {
        if (cursor >= source.length()) return -1;
        int current = static_cast<unsigned char>(source[cursor++]);
        if (current == '\n') line++;
        return current;
    }

    bool atEnd() const { return cursor >= source.length(); }

    void skipWhitespace();

public:
    explicit Lexer(const std::string& src) : source(src) {}

    // Função principal: retorna o próximo token da sequência
    Token nextToken();
};

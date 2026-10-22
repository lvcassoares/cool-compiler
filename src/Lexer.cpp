#include "Lexer.hpp"

#include <cctype>

// ============================================================================
// Lexer.cpp
// ----------------------------------------------------------------------------
// Analisador léxico de COOL. O contrato é apenas Token -> Parser; o lexer não
// conhece AST nem tipos.
//
// Cuidados de robustez (ver RELATORIO-TECNICO.md):
//   - peek()/advance() devolvem int e usam -1 para EOF, então um byte NUL real
//     no arquivo não é confundido com o fim do arquivo.
//   - comentários são pulados com laço, não com recursão.
//   - isalpha/isalnum/isdigit recebem unsigned char (evita UB com bytes >= 0x80).
//   - a string não acumula o texto de comentários em uma std::string inútil.
// ============================================================================

namespace {

bool isAlpha(int c) {
    return c >= 0 && std::isalpha(static_cast<unsigned char>(c)) != 0;
}

bool isDigit(int c) {
    return c >= 0 && std::isdigit(static_cast<unsigned char>(c)) != 0;
}

bool isAlnum(int c) {
    return c >= 0 && std::isalnum(static_cast<unsigned char>(c)) != 0;
}

}  // namespace

void Lexer::skipWhitespace() {
    while (peek() == ' ' || peek() == '\v' || peek() == '\f' || peek() == '\t' ||
           peek() == '\r' || peek() == '\n') {
        advance();
    }
}

Token Lexer::nextToken() {
    // O laço refaz a varredura depois de um comentário. Antes era uma chamada
    // recursiva a nextToken(): N comentários seguidos empilhavam N quadros.
    while (true) {
        skipWhitespace();

        if (atEnd()) {
            return {TokenType::END_OF_FILE, "", line};
        }

        int c = advance();

        // Reconhecimento de Comentários
        if (c == '-') {
            if (peek() == '-') {
                advance();  // Consome o segundo '-'
                while (peek() != '\n' && peek() != -1) {
                    advance();
                }
                continue;  // Ignora o comentário e busca o próximo token
            }
        }
        if (c == '(') {
            if (peek() == '*') {
                advance();  // Consome o '*'
                int nesting_level = 1;
                while (nesting_level > 0) {
                    if (atEnd()) {
                        return {TokenType::ERROR, "EOF in comment", line};
                    }

                    int ca = advance();

                    // Incrementa o nível se encontrar um novo bloco abrindo (*
                    if (ca == '(' && peek() == '*') {
                        advance();  // Consome '*'
                        nesting_level++;
                    }
                    // Decrementa o nível se encontrar um bloco fechando *)
                    else if (ca == '*' && peek() == ')') {
                        advance();  // Consome ')'
                        nesting_level--;
                    }
                }
                continue;  // Ignora o comentário e busca o próximo token
            }
        }
        // Erro: comentário fechado sem abertura
        if (c == '*' && peek() == ')') {
            advance();  // Consome o ')'
            return {TokenType::ERROR, "Unmatched *)", line};
        }

        // Operadores de 1 caractere
        switch (c) {
            case '+': return {TokenType::MAIS, "+", line};
            case '-': return {TokenType::MENOS, "-", line};
            case '*': return {TokenType::ASTERISCO, "*", line};
            case '/': return {TokenType::BARRA, "/", line};
            case '~': return {TokenType::TIL, "~", line};
            case '(': return {TokenType::ABREPARENTESES, "(", line};
            case ')': return {TokenType::FECHAPARENTESES, ")", line};
            case '{': return {TokenType::ABRECHAVES, "{", line};
            case '}': return {TokenType::FECHACHAVES, "}", line};
            case ':': return {TokenType::DOISPONTOS, ":", line};
            case ';': return {TokenType::PONTOEVIRGULA, ";", line};
            case '.': return {TokenType::PONTO, ".", line};
            case ',': return {TokenType::VIRGULA, ",", line};
            case '@': return {TokenType::ARROBA, "@", line};

            // Operador com 2 caracteres ou apenas 1
            case '<':
                if (peek() == '-') {
                    advance();  // Consome o '-'
                    return {TokenType::ATRIBUIR, "<-", line};
                }
                if (peek() == '=') {
                    advance();  // Consome o '='
                    return {TokenType::SETAE, "<=", line};
                }
                return {TokenType::MENORQ, "<", line};
            case '=':
                if (peek() == '>') {
                    advance();  // Consome o '>'
                    return {TokenType::SETAD, "=>", line};
                }
                return {TokenType::IGUAL, "=", line};

            // COOL não tem o operador '>'. Devolver ERROR aqui permite que o
            // parser reporte a causa real em vez de transformar o '>' em um
            // erro sintático genérico.
            case '>':
                return {TokenType::ERROR, "Operador '>' nao existe em COOL", line};

            case 0:
                return {TokenType::ERROR, "Caractere nulo no codigo-fonte", line};
        }

        // Reconhecimento de Identificadores e Palavras-chave
        if (isAlpha(c)) {
            std::string id(1, static_cast<char>(c));
            while (isAlnum(peek()) || peek() == '_') {
                id += static_cast<char>(advance());
            }

            // Cria uma versão totalmente em minúsculas para comparar palavras-chave
            std::string id_lower = id;
            for (char& ch : id_lower) {
                ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
            }

            // self e SELF_TYPE são identificadores especiais (não palavras-chave):
            // o manual os trata como sensíveis a maiúsculas/minúsculas.
            if (id == "self") return {TokenType::SELF, id, line};
            if (id == "SELF_TYPE") return {TokenType::SELF_TYPE, id, line};

            // Tratamento Especial para Booleanos: primeira letra minúscula,
            // o resto pode ser maiúsculo ou minúsculo.
            if (id_lower == "true" || id_lower == "false") {
                if (std::islower(static_cast<unsigned char>(id[0]))) {
                    return {TokenType::BOOLEAN, id, line};
                }
            }

            // Verifica se é uma palavra-chave (case-insensitive, §10.4)
            if (id_lower == "class")    return {TokenType::KEYWORD, id, line};
            if (id_lower == "else")     return {TokenType::KEYWORD, id, line};
            if (id_lower == "fi")       return {TokenType::KEYWORD, id, line};
            if (id_lower == "if")       return {TokenType::KEYWORD, id, line};
            if (id_lower == "in")       return {TokenType::KEYWORD, id, line};
            if (id_lower == "inherits") return {TokenType::KEYWORD, id, line};
            if (id_lower == "isvoid")   return {TokenType::KEYWORD, id, line};
            if (id_lower == "let")      return {TokenType::KEYWORD, id, line};
            if (id_lower == "loop")     return {TokenType::KEYWORD, id, line};
            if (id_lower == "pool")     return {TokenType::KEYWORD, id, line};
            if (id_lower == "then")     return {TokenType::KEYWORD, id, line};
            if (id_lower == "while")    return {TokenType::KEYWORD, id, line};
            if (id_lower == "case")     return {TokenType::KEYWORD, id, line};
            if (id_lower == "esac")     return {TokenType::KEYWORD, id, line};
            if (id_lower == "new")      return {TokenType::KEYWORD, id, line};
            if (id_lower == "of")       return {TokenType::KEYWORD, id, line};
            if (id_lower == "not")      return {TokenType::KEYWORD, id, line};

            // Verifica se é um identificador de tipo ou objeto
            if (std::isupper(static_cast<unsigned char>(id[0]))) {
                return {TokenType::TYPEID, id, line};
            }
            return {TokenType::OBJECTID, id, line};
        }

        // Reconhecimento de Números Inteiros
        if (isDigit(c)) {
            std::string num(1, static_cast<char>(c));
            while (isDigit(peek())) {
                num += static_cast<char>(advance());
            }
            return {TokenType::INT, num, line};
        }

        // Reconhecimento de Strings
        if (c == '"') {
            std::string str;

            while (peek() != '"' && !atEnd()) {
                int nextChar = advance();

                // 1. Erro: Quebra de linha não escapada
                if (nextChar == '\n') {
                    return {TokenType::ERROR, "Unescaped newline in string constant", line};
                }

                // 2. Tratamento de Sequências de Escape (\c)
                if (nextChar == '\\') {
                    if (atEnd()) {
                        return {TokenType::ERROR, "Unterminated string constant", line};
                    }

                    int escaped = advance();
                    switch (escaped) {
                        case 'b': str += '\b'; break;  // Backspace
                        case 't': str += '\t'; break;  // Tab
                        case 'n': str += '\n'; break;  // Newline
                        case 'f': str += '\f'; break;  // Formfeed

                        // §10.2: \c denota o caractere c (exceto \b \t \n \f).
                        // Portanto \0 (barra + zero) denota o caractere '0'.
                        // O que é proibido é o caractere nulo de verdade (byte 0).
                        case '0': str += '0'; break;

                        case '\n':  // Barra seguida de enter: a string continua
                            str += '\n';
                            break;

                        case 0:  // byte NUL real depois da barra
                            return {TokenType::ERROR, "String contains null character", line};

                        default:
                            // Qualquer outro caractere 'c' escapado vira o próprio 'c'
                            str += static_cast<char>(escaped);
                            break;
                    }
                } else if (nextChar == 0) {
                    // Byte NUL real dentro da string (não confundir com EOF)
                    return {TokenType::ERROR, "String contains null character", line};
                } else {
                    str += static_cast<char>(nextChar);
                }
            }

            // Fecha a string se encontrou as aspas finais
            if (peek() == '"') {
                advance();  // Consome as aspas de fechamento
                if (str.size() > 1024) {
                    return {TokenType::ERROR, "String constant too long (max 1024 chars)", line};
                }
                return {TokenType::STRING, str, line};
            }

            // Se saiu do loop sem aspas, o arquivo acabou no meio da string (EOF)
            return {TokenType::ERROR, "Unterminated string constant", line};
        }

        // Se encontrou algo não reconhecido nesta etapa
        return {TokenType::ERROR, std::string(1, static_cast<char>(c)), line};
    }
}

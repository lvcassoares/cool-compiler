#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

// Tipos de Tokens da linguagem COOL
enum class TokenType {
    // Palavras-chave
    KEYWORD,
    
    // Identificadores e Literais
    TYPEID,      // Nomes de Tipos/Classes (ex: Main, Int) -> Começam com maiúscula
    OBJECTID,     // Nomes de Variáveis/Métodos (ex: x, main) -> Começam com minúscula
    INT,  // Números (ex: 42)
    STRING,// Texto (ex: "hello")
    BOOLEAN,      // true / false
    
    // Símbolos e Operadores
    TIL, // ~
    MENORQ,// <
    MAIORQ,// >
    IGUAL,// =
    ATRIBUIR,       // <-
    SETAD,       // =>
    SETAE,       // <=
    MAIS,         // +
    MENOS,        // -
    ASTERISCO,         // *
    BARRA,        // /
    ABREPARENTESES,       // (
    FECHAPARENTESES,       // )
    ABRECHAVES,       // {
    FECHACHAVES,       // }
    DOISPONTOS,        // :
    PONTOEVIRGULA,    // ;
    PONTO,          // .
    VIRGULA,        // ,

    
    //self e SELF TYPE
    SELF,           // self
    SELF_TYPE,      // SELF_TYPE

    // Especiais
    END_OF_FILE,  // Fim do arquivo
    ERROR         // Caractere ou sequência inválida
};

// Estrutura que guarda os dados de cada token encontrado
struct Token {
    TokenType type;
    std::string lexeme; // O texto exato encontrado no código
    int line;           // Linha onde o token foi encontrado (para erros)
};


class Lexer {
private:
    std::string source;
    size_t cursor = 0;
    int line = 1;

    // Métodos auxiliares para navegação na string
    //verifica o caractere sem avançar o cursor
    char peek() const {
        if (cursor >= source.length()) return '\0';
        return source[cursor];
    }

    //retorna o caractere atual e avança o cursor
    char advance() {
        if (cursor >= source.length()) return '\0';
        char current = source[cursor++];
        if (current == '\n') line++;
        return current;
    }

    void skipWhitespace() {
        while (peek() == ' ' || peek() == '\v' || peek() == '\f' || peek() == '\t' || peek() == '\r' || peek() == '\n') {
            advance();
        }
    }

public:
    explicit Lexer(const std::string& src) : source(src) {}

    // Função principal: retorna o próximo token da sequência
    Token nextToken() {
        skipWhitespace();

        if (cursor >= source.length()) {
            return {TokenType::END_OF_FILE, "", line};
        }

        char c = advance();

        // Reconhecimento de Comentários
        if (c == '-') {
            if (peek() == '-') {
                std::string comment(1, c);
                comment += advance(); // Consome o segundo '-'
                while (peek() != '\n' && peek() != '\0') {
                    comment += advance();
                }
                return nextToken(); // Ignora o comentário e busca o próximo token
            }
        }
        if (c == '(') {
            if (peek() == '*') {
                std::string comment(1, c);
                comment += advance(); // Consome o '*'
                int nesting_level = 1;
                while (nesting_level > 0) {
                    char c = advance();

                    // Erro: Fim do arquivo antes de fechar todos os comentários
                    if (c == '\0') {
                    return {TokenType::ERROR, "EOF in comment", line};
                    }

                    // Incrementa o nível se encontrar um novo bloco abrindo (*
                    if (c == '(' && peek() == '*') {
                        advance(); // Consome '*'
                        nesting_level++;
                    }
                    // Decrementa o nível se encontrar um bloco fechando *)
                    else if (c == '*' && peek() == ')') {
                    advance(); // Consome ')'
                        nesting_level--;
                    }
                }
                return nextToken(); // Ignora o comentário e busca o próximo token
            }
        }
        //error: Comentário fechado sem abertura
        if (c == '*' && peek() == ')') {
            advance(); // Consome o ')'
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

            // Operador com 2 caracteres ou apenas 1
            case '<':
                if (peek() == '-') {
                    advance(); // Consome o '-'
                    return {TokenType::ATRIBUIR, "<-", line};
                }
                if (peek() == '=') {
                    advance(); // Consome o '='
                    return {TokenType::SETAE, "<=", line};
                }
                return {TokenType::MENORQ, "<", line};
                break;
            case '=':
                if (peek() == '>') {
                    advance(); // Consome o '>'
                    return {TokenType::SETAD, "=>", line};
                }
                return {TokenType::IGUAL, "=", line};
                break;

        }

        // Reconhecimento de Identificadores e Palavras-chave
        if (std::isalpha(c)) {
            std::string id(1, c);
            while (std::isalnum(peek()) || peek() == '_') {
                id += advance();
            }

            // Cria uma versão totalmente em minúsculas para comparar palavras-chave
            std::string id_lower = id;
            for (char &ch : id_lower) {
                ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
            }

            if (id == "self")     return {TokenType::SELF, id, line};
            if (id == "SELF_TYPE") return {TokenType::SELF_TYPE, id, line};

            // Tratamento Especial para Booleanos
            if (id_lower == "true" || id_lower == "false") {
                if (std::islower(id[0])) { // Primeira letra DEVE ser minúscula
                    return {TokenType::BOOLEAN, id, line};
                }
            }

            // Verifica se é uma palavra-chave
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
            if (std::isupper(id[0])) {
                return {TokenType::TYPEID, id, line};
            } else {
                return {TokenType::OBJECTID, id, line};
            }
        }


        // Reconhecimento de Números Inteiros
        if (std::isdigit(c)) {
            std::string num(1, c);
            while (std::isdigit(peek())) {
                num += advance();
            }
            return {TokenType::INT, num, line};
        }

        // Reconhecimento de Strings
        if (c == '"') {
            std::string str = ""; 
    
            while (peek() != '"' && peek() != '\0') {
                char nextChar = advance();

                // 1. Erro: Quebra de linha não escapada ("Unescaped newline in string constant")
                if (nextChar == '\n') {
                    return {TokenType::ERROR, "Unescaped newline in string constant", line};
                }

                // 2. Tratamento de Sequências de Escape (\c)
                if (nextChar == '\\') {
                    if (peek() == '\0') {
                        return {TokenType::ERROR, "Unterminated string constant", line};
                    }

                    char escaped = advance();
                    switch (escaped) {
                        case 'b':  str += '\b'; break; // Backspace
                        case 't':  str += '\t'; break; // Tab
                        case 'n':  str += '\n'; break; // Newline
                        case 'f':  str += '\f'; break; // Formfeed
                        case '0':  // \0 explícito escapado também é erro no COOL
                            return {TokenType::ERROR, "String contains null character", line};
                        case '\n': // Quebra de linha escapada (\\n no código -> barra seguida de enter):Permite a string continuar na próxima linha.
                            str += '\n'; 
                            break;
                        default:
                            // Qualquer outro caractere 'c' escapado vira o próprio 'c' (ex: \", \\)
                            str += escaped; 
                            break;
                    }
                } 
                else {
                    // Caractere normal
                    str += nextChar;
                }

                // Erro: O COOL proíbe estritamente o caractere nulo em strings
                if (str.back() == '\0') {
                return {TokenType::ERROR, "String contains null character", line};
                }
            }

            // Fecha a string se encontrou as aspas finais
            if (peek() == '"') {
                advance(); // Consome as aspas de fechamento
                return {TokenType::STRING, str, line};
            }
            
            // Se saiu do loop sem aspas, o arquivo acabou no meio da string (EOF)
            return {TokenType::ERROR, "Unterminated string constant", line};
        }

        // Se encontrou algo não reconhecido nesta etapa
        return {TokenType::ERROR, std::string(1, c), line};
    }
};

int main(int argc, char* argv[]) {
    // 1. Verifica se o caminho do arquivo foi passado via linha de comando
    if (argc < 2) {
        std::cerr << "Erro: Nenhum arquivo de entrada fornecido.\n";
        std::cerr << "Uso correto: " << argv[0] << " <caminho_do_arquivo.txt>\n";
        return 1;
    }

    // argv[1] contém o caminho do arquivo passado na execução
    std::string caminhoArquivo = argv[1];
    std::ifstream arquivo(caminhoArquivo);

    // 2. Garante que o arquivo foi aberto com sucesso
    if (!arquivo.is_open()) {
        std::cerr << "Erro ao abrir o arquivo: " << caminhoArquivo << "\n";
        return 1;
    }

    // 3. Leitura eficiente de todo o arquivo para uma string/buffer
    std::stringstream buffer;
    buffer << arquivo.rdbuf();
    std::string conteudo = buffer.str();

    // Fecha o arquivo após a leitura
    arquivo.close();

    // 4. Imprime o conteúdo na tela para validação
    Lexer lex(conteudo);
    Token token;
    std::cout << "--- Tokens Extraídos ---\n";
    do {
        token = lex.nextToken();
        std::cout << "Linha " << token.line 
                  << " | Tipo: " << static_cast<int>(token.type) 
                  << " | Lexema: \"" << token.lexeme << "\"\n";
    } while (token.type != TokenType::END_OF_FILE);
    return 0;
}
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
    ARROBA,      // @

    
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
            case '@': return {TokenType::ARROBA, "@", line};
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

    Token peekToken() {
        size_t savedCursor = cursor;
        int savedLine = line;
        Token token = nextToken();
        cursor = savedCursor;
        line = savedLine;
        return token;
    }
};

class sintatico {
private:
    Lexer lexer; 
    Token currentToken;
    int errorFlag = false; // Flag para indicar se ocorreu um erro

public:
    // Inicializa o atributo 'lexer' passando 'src' para o construtor dele
    explicit sintatico(const std::string& src) : lexer(src) {
        // Carrega o primeiro token para iniciar a descida recursiva 
        currentToken = lexer.nextToken();

        program(); // Inicia a análise sintática a partir do símbolo inicial
    }

    bool parse() {
        return !errorFlag; // Retorna true se não houve erros, false caso contrário
    }

    void program() {
        // consome tokens até o final do arquivo
        while (currentToken.type != TokenType::END_OF_FILE) {
            if (currentToken.type == TokenType::ERROR) {
                std::cerr << "Erro na linha " << currentToken.line 
                          << ": Token inválido \"" << currentToken.lexeme << "\"\n";
                errorFlag = true; // Define a flag de erro
                currentToken = lexer.nextToken(); // Avança para o próximo token
                continue; // Continua a análise mesmo após o erro
            }
            if (currentToken.type == TokenType::KEYWORD && currentToken.lexeme == "class") {
                classe(); // Chama a função para processar a classe
                if (currentToken.type == TokenType::PONTOEVIRGULA) {
                    currentToken = lexer.nextToken(); // Consome o ';' e continua
                } else if (currentToken.type != TokenType::END_OF_FILE) {
                    std::cerr << "Erro na linha " << currentToken.line 
                              << ": Esperado ';' após a classe\n";
                    errorFlag = true; // Define a flag de erro
                }
            }
            else {
                std::cout << "Erro Token encontrado: Tipo = " << static_cast<int>(currentToken.type) 
                          << ", Lexema = \"" << currentToken.lexeme 
                          << "\", Linha = " << currentToken.line 
                          << "Esperado: Palavra-chave 'class'\n";
                errorFlag = true; // Define a flag de erro
            }
            currentToken = lexer.nextToken();
        }
    }
    void classe()
    {
        currentToken = lexer.nextToken();
        if (currentToken.type == TokenType::TYPEID) 
        {
            currentToken = lexer.nextToken();
            if(currentToken.type == TokenType::KEYWORD && currentToken.lexeme == "inherits") 
            {
                currentToken = lexer.nextToken();
                if (currentToken.type != TokenType::TYPEID) 
                {
                    std::cerr << "Erro na linha " << currentToken.line 
                              << ": Esperado identificador de tipo após 'inherits'\n";
                    errorFlag = true; // Define a flag de erro
                }
                currentToken = lexer.nextToken();
            }
            if (currentToken.type == TokenType::ABRECHAVES) 
            {
                currentToken = lexer.nextToken();
                while (currentToken.type != TokenType::FECHACHAVES && currentToken.type != TokenType::END_OF_FILE) {
                    feature(); // Processa cada feature dentro da classe
                    if(currentToken.type == TokenType::PONTOEVIRGULA) {
                        currentToken = lexer.nextToken(); // Consome o ';' e continua
                    }
                    else{
                        std::cerr << "Erro na linha " << currentToken.line 
                                  << ": Esperado ';' após a feature\n";
                        errorFlag = true; // Define a flag de erro
                        currentToken = lexer.nextToken(); // Avança para o próximo token
                    }
                }
                if (currentToken.type == TokenType::FECHACHAVES) 
                {
                    currentToken = lexer.nextToken();
                    // Classe corretamente fechada
                    return;
                } 
                else 
                {
                    std::cerr << "Erro na linha " << currentToken.line 
                              << ": Esperado '}' para fechar a classe\n";
                    errorFlag = true; // Define a flag de erro
                }
            } 
            else 
            {
                std::cerr << "Erro na linha " << currentToken.line 
                          << ": Esperado '{' após o nome da classe\n";
                errorFlag = true; // Define a flag de erro
            }
        } 
        else 
        {
            std::cerr << "Erro na linha " << currentToken.line 
                      << ": Esperado identificador de tipo após 'class'\n";
            errorFlag = true; // Define a flag de erro
        }
    }
    void feature()
    {
        if (currentToken.type == TokenType::OBJECTID) 
        {
            currentToken = lexer.nextToken();
            if(currentToken.type == TokenType::ABREPARENTESES)
            {
                currentToken = lexer.nextToken();
                while (currentToken.type != TokenType::FECHAPARENTESES && currentToken.type != TokenType::END_OF_FILE) {
                    formal(); // Processa cada formal dentro do método
                    if(currentToken.type == TokenType::VIRGULA) {
                        currentToken = lexer.nextToken(); // Consome a ',' e continua
                    }
                    else if(currentToken.type != TokenType::FECHAPARENTESES) {
                        std::cerr << "Erro na linha " << currentToken.line 
                                  << ": Esperado ',' ou ')' após o formal\n";
                        errorFlag = true; // Define a flag de erro
                        currentToken = lexer.nextToken(); // Avança para o próximo token
                    }
                }
                if (currentToken.type == TokenType::FECHAPARENTESES) 
                {
                    currentToken = lexer.nextToken();
                    if(currentToken.type == TokenType::DOISPONTOS)
                    {
                        currentToken = lexer.nextToken();
                        if(currentToken.type == TokenType::TYPEID)
                        {
                            currentToken = lexer.nextToken();
                            if(currentToken.type == TokenType::ABRECHAVES)
                            {
                                currentToken = lexer.nextToken();
                                expr();
                                if(currentToken.type != TokenType::FECHACHAVES)
                                {
                                    std::cerr << "Erro na linha " << currentToken.line 
                                              << ": Esperado '}' para fechar o corpo do método\n";
                                    errorFlag = true; // Define a flag de erro
                                    currentToken = lexer.nextToken(); // Avança para o próximo token
                                }
                                else
                                {
                                    currentToken = lexer.nextToken(); // Consome o '}' e continua
                                }
                            }
                            else
                            {
                                std::cerr << "Erro na linha " << currentToken.line 
                                          << ": Esperado '{' para iniciar o corpo do método\n";
                                errorFlag = true; // Define a flag de erro
                                currentToken = lexer.nextToken(); // Avança para o próximo token
                            }
                        }
                        else
                        {
                            std::cerr << "Erro na linha " << currentToken.line 
                                      << ": Esperado identificador de tipo após ':'\n";
                            errorFlag = true; // Define a flag de erro
                            currentToken = lexer.nextToken(); // Avança para o próximo token
                        }
                    }
                    else 
                    {
                        std::cerr << "Erro na linha " << currentToken.line 
                                  << ": Esperado ':' após a lista de formais\n";
                        errorFlag = true; // Define a flag de erro
                        currentToken = lexer.nextToken(); // Avança para o próximo token
                    }
                }
                else
                {
                    std::cerr << "Erro na linha " << currentToken.line 
                              << ": Esperado ')' para fechar a lista de formais\n";
                    errorFlag = true; // Define a flag de erro
                    currentToken = lexer.nextToken(); // Avança para o próximo token
                }
            }
            else if(currentToken.type == TokenType::TYPEID)
            {
                currentToken = lexer.nextToken();
                if(currentToken.type == TokenType::ATRIBUIR)
                {
                    currentToken = lexer.nextToken();
                    expr();
                    currentToken = lexer.nextToken();
                }
            }
            else
            {
                std::cerr << "Erro na linha " << currentToken.line 
                          << ": Esperado '(' ou ':' após o identificador de feature\n";
                errorFlag = true; // Define a flag de erro
                currentToken = lexer.nextToken(); // Avança para o próximo token
            }
        }
        else
        {
            std::cerr << "Erro na linha " << currentToken.line 
                      << ": Esperado identificador de feature\n";
            errorFlag = true; // Define a flag de erro
        }
    }
    void formal()
    {
        if(currentToken.lexeme == "self")
        {
            std::cerr << "Erro na linha " << currentToken.line 
                      << ": 'self' não pode ser usado como nome de parâmetro\n";
            errorFlag = true; // Define a flag de erro
            currentToken = lexer.nextToken(); // Avança para o próximo token
        }
        else if(currentToken.type == TokenType::OBJECTID)
        {
            currentToken = lexer.nextToken();
            if(currentToken.type == TokenType::DOISPONTOS)
            {
                currentToken = lexer.nextToken();
                if(currentToken.type != TokenType::TYPEID)
                {
                    std::cerr << "Erro na linha " << currentToken.line 
                              << ": Esperado identificador de tipo após ':'\n";
                    errorFlag = true; // Define a flag de erro
                    currentToken = lexer.nextToken(); // Avança para o próximo token
                }
                else
                {
                    currentToken = lexer.nextToken(); // Consome o TYPEID e continua
                }
            }
            else
            {
                std::cerr << "Erro na linha " << currentToken.line 
                          << ": Esperado ':' após o nome do parâmetro\n";
                errorFlag = true; // Define a flag de erro
                currentToken = lexer.nextToken(); // Avança para o próximo token
            }
        }
        else
        {
            std::cerr << "Erro na linha " << currentToken.line 
                      << ": Esperado identificador de parâmetro\n";
            errorFlag = true; // Define a flag de erro
            currentToken = lexer.nextToken(); // Avança para o próximo token
        }
    }
void expr()
{
    // A atribuição (ID <- expr) possui a menor precedência em COOL e exige lookahead 2
    if (currentToken.type == TokenType::OBJECTID && lexer.peekToken().type == TokenType::ATRIBUIR)
    {
        currentToken = lexer.nextToken(); // Consome OBJECTID
        currentToken = lexer.nextToken(); // Consome '<-'
        expr();                           // Processa a expressão atribuída
        return;
    }

    // Caso não seja atribuição, inicia a subida pela hierarquia de precedência
    expr_not();
}

void expr_not()
{
    if (currentToken.type == TokenType::KEYWORD && currentToken.lexeme == "not")
    {
        currentToken = lexer.nextToken(); // Consome "not"
        expr_not();                       // Suporta 'not' aninhado
    }
    else
    {
        expr_cmp();
    }
}

void expr_cmp()
{
    expr_add();
    // Comparação não é associativa em COOL (apenas uma por nível de expressão)
    if (currentToken.type == TokenType::MENORQ || 
        currentToken.type == TokenType::SETAE  || 
        currentToken.type == TokenType::IGUAL)
    {
        currentToken = lexer.nextToken(); // Consome op de comparação
        expr_add();
    }
}

void expr_add()
{
    expr_mult();
    // Laço while garante associação à esquerda (ex: a + b - c)
    while (currentToken.type == TokenType::MAIS || currentToken.type == TokenType::MENOS)
    {
        currentToken = lexer.nextToken(); // Consome '+' ou '-'
        expr_mult();
    }
}

void expr_mult()
{
    expr_unary();
    // Laço while garante associação à esquerda (ex: a * b / c)
    while (currentToken.type == TokenType::ASTERISCO || currentToken.type == TokenType::BARRA)
    {
        currentToken = lexer.nextToken(); // Consome '*' ou '/'
        expr_unary();
    }
}

void expr_unary()
{
    if (currentToken.type == TokenType::TIL)
    {
        currentToken = lexer.nextToken(); // Consome '~'
        expr_unary();
    }
    else if (currentToken.type == TokenType::KEYWORD && currentToken.lexeme == "isvoid")
    {
        currentToken = lexer.nextToken(); // Consome "isvoid"
        expr_unary();
    }
    else if (currentToken.type == TokenType::KEYWORD && currentToken.lexeme == "new")
    {
        currentToken = lexer.nextToken(); // Consome "new"
        if (currentToken.type == TokenType::TYPEID)
        {
            currentToken = lexer.nextToken(); // Consome o TYPEID
        }
        else
        {
            std::cerr << "Erro na linha " << currentToken.line 
                      << ": Esperado identificador de tipo após 'new'\n";
            errorFlag = true;
        }
    }
    else
    {
        expr_dispatch();
    }
}

void expr_dispatch()
{
    expr_primary();

    // Permite chamadas de métodos encadeadas: obj.metodo1().metodo2()
    while (currentToken.type == TokenType::ARROBA || currentToken.type == TokenType::PONTO)
    {
        if (currentToken.type == TokenType::ARROBA)
        {
            currentToken = lexer.nextToken(); // Consome '@'
            if (currentToken.type == TokenType::TYPEID)
            {
                currentToken = lexer.nextToken(); // Consome o tipo do dispatch estático
            }
            else
            {
                std::cerr << "Erro na linha " << currentToken.line 
                          << ": Esperado identificador de tipo após '@'\n";
                errorFlag = true;
            }
        }

        if (currentToken.type == TokenType::PONTO)
        {
            currentToken = lexer.nextToken(); // Consome '.'
            if (currentToken.type == TokenType::OBJECTID)
            {
                currentToken = lexer.nextToken(); // Consome nome do método
                if (currentToken.type == TokenType::ABREPARENTESES)
                {
                    currentToken = lexer.nextToken(); // Consome '('
                    if (currentToken.type != TokenType::FECHAPARENTESES)
                    {
                        expr();
                        while (currentToken.type == TokenType::VIRGULA)
                        {
                            currentToken = lexer.nextToken(); // Consome ','
                            expr();
                        }
                    }

                    if (currentToken.type == TokenType::FECHAPARENTESES)
                    {
                        currentToken = lexer.nextToken(); // Consome ')'
                    }
                    else
                    {
                        std::cerr << "Erro na linha " << currentToken.line 
                                  << ": Esperado ')' após lista de argumentos\n";
                        errorFlag = true;
                    }
                }
                else
                {
                    std::cerr << "Erro na linha " << currentToken.line 
                              << ": Esperado '(' após nome do método\n";
                    errorFlag = true;
                }
            }
            else
            {
                std::cerr << "Erro na linha " << currentToken.line 
                          << ": Esperado identificador de método após '.'\n";
                errorFlag = true;
            }
        }
    }
}

void expr_primary()
{
    if (currentToken.type == TokenType::KEYWORD && currentToken.lexeme == "if")
    {
        currentToken = lexer.nextToken(); // Consome 'if'
        expr();
        if (currentToken.type == TokenType::KEYWORD && currentToken.lexeme == "then")
        {
            currentToken = lexer.nextToken(); // Consome 'then'
            expr();
            if (currentToken.type == TokenType::KEYWORD && currentToken.lexeme == "else")
            {
                currentToken = lexer.nextToken(); // Consome 'else'
                expr();
                if (currentToken.type == TokenType::KEYWORD && currentToken.lexeme == "fi")
                {
                    currentToken = lexer.nextToken(); // Consome 'fi'
                }
                else
                {
                    std::cerr << "Erro na linha " << currentToken.line << ": Esperado 'fi'\n";
                    errorFlag = true;
                }
            }
            else
            {
                std::cerr << "Erro na linha " << currentToken.line << ": Esperado 'else'\n";
                errorFlag = true;
            }
        }
        else
        {
            std::cerr << "Erro na linha " << currentToken.line << ": Esperado 'then'\n";
            errorFlag = true;
        }
    }
    else if (currentToken.type == TokenType::KEYWORD && currentToken.lexeme == "while")
    {
        currentToken = lexer.nextToken(); // Consome 'while'
        expr();
        if (currentToken.type == TokenType::KEYWORD && currentToken.lexeme == "loop")
        {
            currentToken = lexer.nextToken(); // Consome 'loop'
            expr();
            if (currentToken.type == TokenType::KEYWORD && currentToken.lexeme == "pool")
            {
                currentToken = lexer.nextToken(); // Consome 'pool'
            }
            else
            {
                std::cerr << "Erro na linha " << currentToken.line << ": Esperado 'pool'\n";
                errorFlag = true;
            }
        }
        else
        {
            std::cerr << "Erro na linha " << currentToken.line << ": Esperado 'loop'\n";
            errorFlag = true;
        }
    }
    else if (currentToken.type == TokenType::KEYWORD && currentToken.lexeme == "let")
    {
        currentToken = lexer.nextToken(); // Consome 'let'
        
        auto parse_binding = [this]() {
            if (currentToken.type == TokenType::OBJECTID)
            {
                currentToken = lexer.nextToken();
                if (currentToken.type == TokenType::DOISPONTOS)
                {
                    currentToken = lexer.nextToken();
                    if (currentToken.type == TokenType::TYPEID)
                    {
                        currentToken = lexer.nextToken();
                        if (currentToken.type == TokenType::ATRIBUIR)
                        {
                            currentToken = lexer.nextToken();
                            expr();
                        }
                    }
                    else
                    {
                        std::cerr << "Erro na linha " << currentToken.line << ": Esperado TYPEID em 'let'\n";
                        errorFlag = true;
                    }
                }
                else
                {
                    std::cerr << "Erro na linha " << currentToken.line << ": Esperado ':' em 'let'\n";
                    errorFlag = true;
                }
            }
        };

        parse_binding();
        while (currentToken.type == TokenType::VIRGULA)
        {
            currentToken = lexer.nextToken(); // Consome ','
            parse_binding();
        }

        if (currentToken.type == TokenType::KEYWORD && currentToken.lexeme == "in")
        {
            currentToken = lexer.nextToken(); // Consome 'in'
            expr();
        }
        else
        {
            std::cerr << "Erro na linha " << currentToken.line << ": Esperado 'in' em 'let'\n";
            errorFlag = true;
        }
    }
    else if (currentToken.type == TokenType::ABRECHAVES)
    {
        currentToken = lexer.nextToken(); // Consome '{'
        while (currentToken.type != TokenType::FECHACHAVES && currentToken.type != TokenType::END_OF_FILE)
        {
            expr();
            if (currentToken.type == TokenType::PONTOEVIRGULA)
            {
                currentToken = lexer.nextToken(); // Consome ';'
            }
            else
            {
                std::cerr << "Erro na linha " << currentToken.line << ": Esperado ';' no bloco\n";
                errorFlag = true;
                break;
            }
        }
        if (currentToken.type == TokenType::FECHACHAVES)
        {
            currentToken = lexer.nextToken(); // Consome '}'
        }
    }
    else if (currentToken.type == TokenType::KEYWORD && currentToken.lexeme == "case")
    {
        currentToken = lexer.nextToken(); // Consome 'case'
        expr();                            // Processa a expressão analisada pelo case

        if (currentToken.type == TokenType::KEYWORD && currentToken.lexeme == "of")
        {
            currentToken = lexer.nextToken(); // Consome 'of'
            while (currentToken.type != TokenType::KEYWORD && 
                   currentToken.lexeme != "esac" && 
                   currentToken.type != TokenType::END_OF_FILE)
            {
                if (currentToken.type == TokenType::OBJECTID)
                {
                    currentToken = lexer.nextToken(); // Consome ID
                    if (currentToken.type == TokenType::DOISPONTOS)
                    {
                        currentToken = lexer.nextToken(); // Consome ':'
                        if (currentToken.type == TokenType::TYPEID)
                        {
                            currentToken = lexer.nextToken(); // Consome tipo
                            if (currentToken.type == TokenType::SETAD)
                            {
                                currentToken = lexer.nextToken(); // Consome '=>'
                                expr();                           // Processa ramo do case
                                if (currentToken.type == TokenType::PONTOEVIRGULA)
                                {
                                    currentToken = lexer.nextToken(); // Consome ';'
                                }
                                else
                                {
                                    std::cerr << "Erro na linha " << currentToken.line << ": Esperado ';' no caso\n";
                                    errorFlag = true;
                                }
                            }
                        }
                    }
                }
            }
            if (currentToken.type == TokenType::KEYWORD && currentToken.lexeme == "esac")
            {
                currentToken = lexer.nextToken(); // Consome 'esac'
            }
            else
            {
                std::cerr << "Erro na linha " << currentToken.line << ": Esperado 'esac'\n";
                errorFlag = true;
            }
        }
        else
        {
            std::cerr << "Erro na linha " << currentToken.line << ": Esperado 'of' no 'case'\n";
            errorFlag = true;
        }
    }
    else if (currentToken.type == TokenType::OBJECTID)
    {
        currentToken = lexer.nextToken(); // Consome o ID do objeto
        
        // Trata chamada de método local: id(...)
        if (currentToken.type == TokenType::ABREPARENTESES)
        {
            currentToken = lexer.nextToken(); // Consome '('
            if (currentToken.type != TokenType::FECHAPARENTESES)
            {
                expr();
                while (currentToken.type == TokenType::VIRGULA)
                {
                    currentToken = lexer.nextToken(); // Consome ','
                    expr();
                }
            }
            if (currentToken.type == TokenType::FECHAPARENTESES)
            {
                currentToken = lexer.nextToken(); // Consome ')'
            }
            else
            {
                std::cerr << "Erro na linha " << currentToken.line << ": Esperado ')'\n";
                errorFlag = true;
            }
        }
        // Se não for '(', trata-se de uma variável simples. O token já foi consumido.
    }
    else if (currentToken.type == TokenType::INT || 
             currentToken.type == TokenType::STRING || 
             (currentToken.type == TokenType::KEYWORD && 
              (currentToken.lexeme == "true" || currentToken.lexeme == "false" || currentToken.lexeme == "self")))
    {
        currentToken = lexer.nextToken(); // Consome constante ou self
    }
    else if (currentToken.type == TokenType::ABREPARENTESES)
    {
        currentToken = lexer.nextToken(); // Consome '('
        expr();
        if (currentToken.type == TokenType::FECHAPARENTESES)
        {
            currentToken = lexer.nextToken(); // Consome ')'
        }
        else
        {
            std::cerr << "Erro na linha " << currentToken.line << ": Esperado ')'\n";
            errorFlag = true;
        }
    }
    else
    {
        std::cerr << "Erro na linha " << currentToken.line << ": Expressão inválida\n";
        errorFlag = true;
    }
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

    /*
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
    */

    // 4. Cria o analisador léxico e sintático
    sintatico sim(conteudo);
    if(sim.parse())
    {
        printf("Analise sintatica concluida com sucesso. Nenhum erro encontrado.\n");
    }
    else
    {
        printf("Analise sintatica concluida com erros. Verifique as mensagens acima.\n");
    }
    return 0;
}
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <queue>

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
                    char ca = advance();

                    // Erro: Fim do arquivo antes de fechar todos os comentários
                    if (ca == '\0') {
                    return {TokenType::ERROR, "EOF in comment", line};
                    }

                    // Incrementa o nível se encontrar um novo bloco abrindo (*
                    if (ca == '(' && peek() == '*') {
                        advance(); // Consome '*'
                        nesting_level++;
                    }
                    // Decrementa o nível se encontrar um bloco fechando *)
                    else if (ca == '*' && peek() == ')') {
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
            if (id_lower == "class")    return {TokenType::KEYWORD, id_lower, line};
            if (id_lower == "else")     return {TokenType::KEYWORD, id_lower, line};
            if (id_lower == "fi")       return {TokenType::KEYWORD, id_lower, line};
            if (id_lower == "if")       return {TokenType::KEYWORD, id_lower, line};
            if (id_lower == "in")       return {TokenType::KEYWORD, id_lower, line};
            if (id_lower == "inherits") return {TokenType::KEYWORD, id_lower, line};
            if (id_lower == "isvoid")   return {TokenType::KEYWORD, id_lower, line};
            if (id_lower == "let")      return {TokenType::KEYWORD, id_lower, line};
            if (id_lower == "loop")     return {TokenType::KEYWORD, id_lower, line};
            if (id_lower == "pool")     return {TokenType::KEYWORD, id_lower, line};
            if (id_lower == "then")     return {TokenType::KEYWORD, id_lower, line};
            if (id_lower == "while")    return {TokenType::KEYWORD, id_lower, line};
            if (id_lower == "case")     return {TokenType::KEYWORD, id_lower, line};
            if (id_lower == "esac")     return {TokenType::KEYWORD, id_lower, line};
            if (id_lower == "new")      return {TokenType::KEYWORD, id_lower, line};
            if (id_lower == "of")       return {TokenType::KEYWORD, id_lower, line};
            if (id_lower == "not")      return {TokenType::KEYWORD, id_lower, line};

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

// Classe base para todos os nós da árvore
class ASTNode {
public:
    int line; // Linha do código onde o nó foi criado (para erros)
    virtual ~ASTNode() = default;
    virtual void print(int indent = 0) const = 0; // Para visualização da árvore
};

// --- NÓS DE EXPRESSÃO ---
class ExprNode : public ASTNode {};

// --- TERMINAIS E LITERARIAIS ---
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
        std::cout << std::string(indent, ' ') << "String(\"" << value << "\")\n";
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

class ObjectASTNode : public ExprNode {
public:
    std::string name;
    explicit ObjectASTNode(std::string n) : name(std::move(n)) {}
    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ') << "Object(" << name << ")\n";
    }
};

/*
// Variável / Identificador (ex: x)
class VariableASTNode : public ExprNode {
public:
    std::string name;
    explicit VariableASTNode(std::string n) : name(std::move(n)) {}
    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ') << "Variable(" << name << ")\n";
    }
};
*/

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

/*
// Atribuição (ex: x <- expr)
class AssignASTNode : public ExprNode {
public:
    std::string name;
    std::shared_ptr<ExprNode> value;

    AssignASTNode(std::string n, std::shared_ptr<ExprNode> v)
        : name(std::move(n)), value(std::move(v)) {}

    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ') << "Assign(" << name << " <-)\n";
        value->print(indent + 2);
    }
};
*/

// --- DISPATCH E CHAMADAS ---

class DispatchASTNode : public ExprNode {
public:
    std::shared_ptr<ExprNode> expr;
    std::string typeExpr; // Para dispatch estático @TYPE (pode ser vazio)
    std::string name;
    std::vector<std::shared_ptr<ExprNode>> args;

    DispatchASTNode(std::shared_ptr<ExprNode> e, std::string t, std::string n, std::vector<std::shared_ptr<ExprNode>> a)
        : expr(std::move(e)), typeExpr(std::move(t)), name(std::move(n)), args(std::move(a)) {}

    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ') << "Dispatch(" << name 
                  << (typeExpr.empty() ? "" : " @" + typeExpr) << ")\n";
        if (expr) expr->print(indent + 2);
        for (const auto& arg : args) arg->print(indent + 2);
    }
};

class StaticDispatchASTNode : public ExprNode {
public:
    std::string name;
    std::vector<std::shared_ptr<ExprNode>> args;
    StaticDispatchASTNode(std::string n, std::vector<std::shared_ptr<ExprNode>> a)
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
    explicit BlockASTNode(std::vector<std::shared_ptr<ExprNode>> exprs) : expressions(std::move(exprs)) {}
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
            std::cout << std::string(indent + 2, ' ') << "Binding(" << bind.name << " : " << bind.type << ")\n";
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
            std::cout << std::string(indent + 2, ' ') << "Branch(" << br.name << " : " << br.type << ")\n";
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

    FormalASTNode(std::string n, std::string t) 
        : name(std::move(n)), type(std::move(t)) {}

    void print(int indent = 0) const override {
        std::cout << std::string(indent, ' ') << "Formal(" << name << " : " << type << ")\n";
    }
};

// Representa um recurso (método ou atributo)
// Nó para Métodos e Atributos de uma Classe
class FeatureASTNode : public ASTNode {
public:
    /*  
    std::string name;
    std::string returnType;
    bool isMethod;
    
    // Para métodos:
    std::vector<std::shared_ptr<FormalASTNode>> formals;
    std::shared_ptr<ExprNode> body;

    // Para atributos:
    std::shared_ptr<ExprNode> initExpr; // Pode ser nullptr se não houver '<-'
    */

    std::string name;
    std::vector<std::shared_ptr<FormalASTNode>> formals;
    std::string returnType;
    std::shared_ptr<ExprNode> body;
    std::shared_ptr<ExprNode> initExpr;
    bool isMethod;

    // Construtor para MÉTODOS
    FeatureASTNode(std::string n, 
                   std::vector<std::shared_ptr<FormalASTNode>> f, 
                   std::string retType, 
                   std::shared_ptr<ExprNode> b)
        : name(std::move(n)), formals(std::move(f)), returnType(std::move(retType)), 
          body(std::move(b)), isMethod(true), initExpr(nullptr) {}

    // Construtor para ATRIBUTOS
    FeatureASTNode(std::string n, 
                   std::string type, 
                   std::shared_ptr<ExprNode> init)
        : name(std::move(n)), returnType(std::move(type)), 
          initExpr(std::move(init)), isMethod(false), body(nullptr) {}

    void print(int indent = 0) const override {
        if (isMethod) {
            std::cout << std::string(indent, ' ') << "Method(" << name << " : " << returnType << ")\n";
            for (const auto& f : formals) {
                f->print(indent + 2);
            }
            if (body) body->print(indent + 2);
        } else {
            std::cout << std::string(indent, ' ') << "Attribute(" << name << " : " << returnType << ")\n";
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
        std::cout << std::string(indent, ' ') << "Class(" << name << " inherits " << parent << ")\n";
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


class sintatico {
private:
    Lexer lexer; 
    Token currentToken;
    std::shared_ptr<ProgramASTNode> astRoot; // Raiz da árvore de sintaxe abstrata
    int errorFlag = false; // Flag para indicar se ocorreu um erro

public:
    // Inicializa o atributo 'lexer' passando 'src' para o construtor dele
    explicit sintatico(const std::string& src) : lexer(src) {
        // Carrega o primeiro token para iniciar a descida recursiva 
        currentToken = lexer.nextToken();

        astRoot = program(); // Inicia a análise sintática e constrói a árvore
    }

    std::shared_ptr<ProgramASTNode> parse() {
        return astRoot;
    }

    int valid() const {
        return !errorFlag; // Retorna true se não houve erro, false caso contrário
    }

    std::shared_ptr<ProgramASTNode> program() {
        // consome tokens até o final do arquivo
        auto progNode = std::make_shared<ProgramASTNode>();
        progNode->line = currentToken.line; // Define a linha do nó raiz como a linha do primeiro token

        while (currentToken.type != TokenType::END_OF_FILE) {
            if (currentToken.type == TokenType::ERROR) {
                std::cout << "Erro na linha " << currentToken.line 
                          << ": Token inválido \"" << currentToken.lexeme << "\"\n";
                errorFlag = true; // Define a flag de erro
                return nullptr; // Sai da função
            }
            if (currentToken.type == TokenType::KEYWORD && currentToken.lexeme == "class") {
                auto classNode = classe(); // Chama a função para processar a classe
                if (classNode) {
                    progNode->classes.push_back(classNode);
                }

                if (currentToken.type == TokenType::PONTOEVIRGULA) {
                    currentToken = lexer.nextToken(); // Consome o ';' e continua
                } else if (currentToken.type != TokenType::END_OF_FILE) {
                    std::cout << "Erro na linha " << currentToken.line 
                              << ": Esperado ';' após a classe\n";
                    errorFlag = true; // Define a flag de erro
                    return nullptr; // Sai da função
                }
            }
            else {
                std::cout << "Erro Token encontrado: Tipo = " << static_cast<int>(currentToken.type) 
                          << ", Lexema = \"" << currentToken.lexeme 
                          << "\", Linha = " << currentToken.line 
                          << "Esperado: Palavra-chave 'class'\n";
                errorFlag = true; // Define a flag de erro
                return nullptr; // Sai da função
            }
        }
        return progNode;
    }
    std::shared_ptr<ClassASTNode> classe()
    {
        currentToken = lexer.nextToken();// Consome 'class'

        if (currentToken.type == TokenType::TYPEID) 
        {
            std::string className = currentToken.lexeme;
            std::string parentName = "Object";

            currentToken = lexer.nextToken();

            if(currentToken.type == TokenType::KEYWORD && currentToken.lexeme == "inherits") 
            {
                currentToken = lexer.nextToken(); // Consome 'inherits'
            
                if (currentToken.type == TokenType::TYPEID) 
                {
                    parentName = currentToken.lexeme;
                    currentToken = lexer.nextToken(); // Consome o nome da classe pai
                }
                else 
                {
                    std::cout   << "Erro na linha " << currentToken.line 
                                << ": Esperado identificador de tipo após 'inherits'\n";
                    errorFlag = true;
                    return nullptr;
                }
            }

            auto classNode = std::make_shared<ClassASTNode>(className, parentName);
            classNode->line = currentToken.line; // Define a linha do nó de classe

            if (currentToken.type == TokenType::ABRECHAVES) 
            {
                currentToken = lexer.nextToken();// Consome '{'
                while (currentToken.type != TokenType::FECHACHAVES && currentToken.type != TokenType::END_OF_FILE) {
                    auto feat = feature(); // Processa cada feature dentro da classe
                    if (feat) {
                        classNode->features.push_back(feat);
                    }

                    if(currentToken.type == TokenType::PONTOEVIRGULA) {
                        currentToken = lexer.nextToken(); // Consome o ';' e continua
                    }
                    else{
                        std::cout << "Erro na linha " << currentToken.line 
                                  << ": Esperado ';' após a feature\n";
                        errorFlag = true; // Define a flag de erro
                        return nullptr; // Sai da função
                    }
                }
                if (currentToken.type == TokenType::FECHACHAVES) 
                {
                    currentToken = lexer.nextToken();
                    // Classe corretamente fechada
                    return classNode;
                } 
                else 
                {
                    std::cout << "Erro na linha " << currentToken.line 
                              << ": Esperado '}' para fechar a classe\n";
                    errorFlag = true; // Define a flag de erro
                    return nullptr; // Sai da função
                }
            } 
            else 
            {
                std::cout << "Erro na linha " << currentToken.line 
                          << ": Esperado '{' após o nome da classe\n";
                errorFlag = true; // Define a flag de erro
                return nullptr; // Sai da função
            }
        } 
        else 
        {
            std::cout << "Erro na linha " << currentToken.line 
                      << ": Esperado identificador de tipo após 'class'\n";
            errorFlag = true; // Define a flag de erro
            return nullptr; // Sai da função
        }
    }
    std::shared_ptr<FeatureASTNode> feature()
    {
        if (currentToken.type == TokenType::OBJECTID) 
        {
            std::string featureName = currentToken.lexeme;
            currentToken = lexer.nextToken();

            // CASO 1: MÉTODO -> ID( [ formal [[, formal]]* ] ) : TYPE { expr }
            if(currentToken.type == TokenType::ABREPARENTESES)
            {
                currentToken = lexer.nextToken();
                std::vector<std::shared_ptr<FormalASTNode>> formalsList;

                while (currentToken.type != TokenType::FECHAPARENTESES && currentToken.type != TokenType::END_OF_FILE) {
                    
                    auto fNode = formal(); // Processa cada formal dentro do método

                    if (fNode) {
                        formalsList.push_back(fNode);
                    } else {
                        return nullptr; // Falha no parsing do formal
                    }

                    if(currentToken.type == TokenType::VIRGULA) {
                        currentToken = lexer.nextToken(); // Consome a ',' e continua
                    }
                    else if(currentToken.type != TokenType::FECHAPARENTESES) {
                        std::cout << "Erro na linha " << currentToken.line 
                                  << ": Esperado ',' ou ')' após o formal\n";
                        errorFlag = true; // Define a flag de erro
                        return nullptr; // Sai da função
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
                            std::string returnType = currentToken.lexeme;
                            currentToken = lexer.nextToken();

                            if(currentToken.type == TokenType::ABRECHAVES)
                            {
                                currentToken = lexer.nextToken();
                                auto methodBody = expr(); // Processa a expressão do corpo
                                if(currentToken.type != TokenType::FECHACHAVES)
                                {
                                    std::cout << "Erro na linha " << currentToken.line 
                                              << ": Esperado '}' para fechar o corpo do método\n";
                                    errorFlag = true; // Define a flag de erro
                                    return nullptr; // Sai da função
                                }
                                else
                                {
                                    currentToken = lexer.nextToken(); // Consome o '}' e continua
                                        auto methodNode = std::make_shared<FeatureASTNode>(
                                        featureName, formalsList, returnType, methodBody
                                    );
                                    methodNode->line = currentToken.line; // Define a linha do nó de método
                                    return methodNode;
                                }
                            }
                            else
                            {
                                std::cout << "Erro na linha " << currentToken.line 
                                          << ": Esperado '{' para iniciar o corpo do método\n";
                                errorFlag = true; // Define a flag de erro
                                return nullptr; // Sai da função
                            }
                        }
                        else
                        {
                            std::cout << "Erro na linha " << currentToken.line 
                                      << ": Esperado identificador de tipo após ':'\n";
                            errorFlag = true; // Define a flag de erro
                            return nullptr; // Sai da função
                        }
                    }
                    else 
                    {
                        std::cout << "Erro na linha " << currentToken.line 
                                  << ": Esperado ':' após a lista de formais\n";
                        errorFlag = true; // Define a flag de erro
                        return nullptr; // Sai da função
                    }
                }
                else
                {
                    std::cout << "Erro na linha " << currentToken.line 
                              << ": Esperado ')' para fechar a lista de formais\n";
                    errorFlag = true; // Define a flag de erro
                    return nullptr; // Sai da função
                }
            }
            // CASO 2: ATRIBUTO -> ID : TYPE [ <- expr ]
            else if(currentToken.type == TokenType::DOISPONTOS)
            {
                currentToken = lexer.nextToken(); // Consome ':'
                if (currentToken.type == TokenType::TYPEID)
                {
                    std::string attrType = currentToken.lexeme;
                    currentToken = lexer.nextToken(); // Consome o tipo (ex: String)

                    std::shared_ptr<ExprNode> initExpression = nullptr;
                
                    // Inicialização opcional com <-
                    if (currentToken.type == TokenType::ATRIBUIR)
                    {
                        currentToken = lexer.nextToken(); // Consome '<-'
                        initExpression = expr(); // Processa a expressão inicial
                    }
                    // Se não houver '<-', não faz nada a função termina deixando o token no ';'
                    auto attrNode = std::make_shared<FeatureASTNode>(featureName, attrType, initExpression);
                    attrNode->line = currentToken.line; // Define a linha do nó de atributo
                    return attrNode;
                }
                else
                {
                    std::cout << "Erro na linha " << currentToken.line 
                          << ": Esperado identificador de tipo após ':'\n";
                    errorFlag = true;
                    return nullptr;
                }
            }
            else
            {
                std::cout << "Erro na linha " << currentToken.line 
                          << ": Esperado '(' ou ':' após o identificador de feature\n";
                errorFlag = true; // Define a flag de erro
                return nullptr; // Sai da função
            }
        }
        else
        {
            std::cout << "Erro na linha " << currentToken.line 
                      << ": Esperado identificador de feature\n";
            errorFlag = true; // Define a flag de erro
            return nullptr; // Sai da função
        }
    }
    std::shared_ptr<FormalASTNode> formal()
    {
        if(currentToken.lexeme == "self")
        {
            std::cout << "Erro na linha " << currentToken.line 
                      << ": 'self' não pode ser usado como nome de parâmetro\n";
            errorFlag = true; // Define a flag de erro
            return nullptr; // Sai da função
        }
        else if(currentToken.type == TokenType::OBJECTID)
        {
            std::string paramName = currentToken.lexeme;
            currentToken = lexer.nextToken();
            if(currentToken.type == TokenType::DOISPONTOS)
            {
                currentToken = lexer.nextToken();
                if(currentToken.type != TokenType::TYPEID)
                {
                    std::cout << "Erro na linha " << currentToken.line 
                              << ": Esperado identificador de tipo após ':'\n";
                    errorFlag = true; // Define a flag de erro
                    return nullptr; // Sai da função
                }
                else
                {
                    std::string paramType = currentToken.lexeme;
                    currentToken = lexer.nextToken(); // Consome o TYPEID e continua

                    auto formalNode = std::make_shared<FormalASTNode>(paramName, paramType);
                    formalNode->line = currentToken.line; // Define a linha do nó de parâmetro
                    return formalNode;
                }
            }
            else
            {
                std::cout << "Erro na linha " << currentToken.line 
                          << ": Esperado ':' após o nome do parâmetro\n";
                errorFlag = true; // Define a flag de erro
                return nullptr; // Sai da função
            }
        }
        else
        {
            std::cout << "Erro na linha " << currentToken.line 
                      << ": Esperado identificador de parâmetro\n";
            errorFlag = true; // Define a flag de erro
            return nullptr; // Sai da função
        }
    }
std::shared_ptr<ExprNode> expr()
{
    // A atribuição (ID <- expr) possui a menor precedência em COOL e exige lookahead 2
    if (currentToken.type == TokenType::OBJECTID && lexer.peekToken().type == TokenType::ATRIBUIR)
    {
        std::string varName = currentToken.lexeme;
        currentToken = lexer.nextToken(); // Consome OBJECTID
        currentToken = lexer.nextToken(); // Consome '<-'
        auto assignedValue = expr();      // Processa a expressão atribuída
        auto assignNode = std::make_shared<AssignASTNode>(varName, assignedValue);
        assignNode->line = currentToken.line; // Define a linha do nó de atribuição
        return assignNode;
    }

    // Caso não seja atribuição, inicia a subida pela hierarquia de precedência
    return expr_not();
}

std::shared_ptr<ExprNode> expr_not()
{
    if (currentToken.type == TokenType::KEYWORD && currentToken.lexeme == "not")
    {
        std::string op = currentToken.lexeme;
        currentToken = lexer.nextToken(); // Consome "not"
        auto operand = expr_not(); // Suporta 'not' aninhado
        auto unaryNode = std::make_shared<UnaryOpASTNode>(op, operand);
        unaryNode->line = currentToken.line; // Define a linha do nó unário
        return unaryNode;
    }
    else
    {
        return expr_cmp();
    }
}

std::shared_ptr<ExprNode> expr_cmp()
{
    auto left = expr_add();
    // Comparação não é associativa em COOL (apenas uma por nível de expressão)
    if (currentToken.type == TokenType::MENORQ || 
        currentToken.type == TokenType::SETAE  || 
        currentToken.type == TokenType::IGUAL)
    {
        std::string op = currentToken.lexeme;
        currentToken = lexer.nextToken(); // Consome op de comparação
        auto right = expr_add();
        auto binaryNode = std::make_shared<BinaryOpASTNode>(op, left, right);
        binaryNode->line = currentToken.line; // Define a linha do nó binário
        return binaryNode;
    }
    return left;
}

std::shared_ptr<ExprNode> expr_add()
{
    auto left = expr_mult();
    // Laço while garante associação à esquerda (ex: a + b - c)
    while (currentToken.type == TokenType::MAIS || currentToken.type == TokenType::MENOS)
    {
        std::string op = currentToken.lexeme;
        currentToken = lexer.nextToken(); // Consome '+' ou '-'
        auto right = expr_mult();
        left = std::make_shared<BinaryOpASTNode>(op, left, right);
        left->line = currentToken.line; // Define a linha do nó binário
    }
    return left;
}

std::shared_ptr<ExprNode> expr_mult()
{
    auto left = expr_unary();
    // Laço while garante associação à esquerda (ex: a * b / c)
    while (currentToken.type == TokenType::ASTERISCO || currentToken.type == TokenType::BARRA)
    {
        std::string op = currentToken.lexeme;
        currentToken = lexer.nextToken(); // Consome '*' ou '/'
        auto right = expr_unary();
        left = std::make_shared<BinaryOpASTNode>(op, left, right);
        left->line = currentToken.line; // Define a linha do nó binário
    }
    return left;
}

std::shared_ptr<ExprNode> expr_unary()
{
    if (currentToken.type == TokenType::TIL)
    {
        std::string op = currentToken.lexeme;
        currentToken = lexer.nextToken(); // Consome '~'
        auto operand = expr_unary();
        auto unaryNode = std::make_shared<UnaryOpASTNode>(op, operand);
        unaryNode->line = currentToken.line; // Define a linha do nó unário
        return unaryNode;
    }
    else if (currentToken.type == TokenType::KEYWORD && currentToken.lexeme == "isvoid")
    {
        std::string op = currentToken.lexeme;
        currentToken = lexer.nextToken(); // Consome "isvoid"
        auto operand = expr_unary();
        auto unaryNode = std::make_shared<UnaryOpASTNode>(op, operand);
        unaryNode->line = currentToken.line; // Define a linha do nó unário
        return unaryNode;
    }
    else if (currentToken.type == TokenType::KEYWORD && currentToken.lexeme == "new")
    {
        currentToken = lexer.nextToken(); // Consome "new"
        if (currentToken.type == TokenType::TYPEID)
        {
            std::string typeName = currentToken.lexeme;
            currentToken = lexer.nextToken(); // Consome o TYPEID
            auto newNode = std::make_shared<NewASTNode>(typeName);
            newNode->line = currentToken.line; // Define a linha do nó 'new'
            return newNode;
        }
        else
        {
            std::cout << "Erro na linha " << currentToken.line 
                      << ": Esperado identificador de tipo após 'new'\n";
            errorFlag = true;
            return nullptr;
        }
    }
    else
    {
        return expr_dispatch();
    }
}

std::shared_ptr<ExprNode> expr_dispatch()
{
    auto leftExpr = expr_primary();

    // Permite chamadas de métodos encadeadas: obj.metodo1().metodo2()
    while (currentToken.type == TokenType::ARROBA || currentToken.type == TokenType::PONTO)
    {
        std::string staticType = "";
        if (currentToken.type == TokenType::ARROBA)
        {
            currentToken = lexer.nextToken(); // Consome '@'
            if (currentToken.type == TokenType::TYPEID)
            {
                staticType = currentToken.lexeme;
                currentToken = lexer.nextToken(); // Consome o tipo do dispatch estático
            }
            else
            {
                std::cout << "Erro na linha " << currentToken.line 
                          << ": Esperado identificador de tipo após '@'\n";
                errorFlag = true;
                return nullptr;
            }
        }

        if (currentToken.type == TokenType::PONTO)
        {
            currentToken = lexer.nextToken(); // Consome '.'
            if (currentToken.type == TokenType::OBJECTID)
            {
                std::string methodName = currentToken.lexeme;
                currentToken = lexer.nextToken(); // Consome nome do método
                if (currentToken.type == TokenType::ABREPARENTESES)
                {
                    currentToken = lexer.nextToken(); // Consome '('
                    std::vector<std::shared_ptr<ExprNode>> args;
                    if (currentToken.type != TokenType::FECHAPARENTESES)
                    {
                        args.push_back(expr());
                        while (currentToken.type == TokenType::VIRGULA)
                        {
                            currentToken = lexer.nextToken(); // Consome ','
                            args.push_back(expr());
                        }
                    }

                    if (currentToken.type == TokenType::FECHAPARENTESES)
                    {
                        currentToken = lexer.nextToken(); // Consome ')'
                        leftExpr = std::make_shared<DispatchASTNode>(leftExpr, staticType, methodName, args);
                        leftExpr->line = currentToken.line; // Define a linha do nó de dispatch
                    }
                    else
                    {
                        std::cerr << "Erro na linha " << currentToken.line 
                                  << ": Esperado ')' após lista de argumentos\n";
                        errorFlag = true;
                        return nullptr;
                    }
                }
                else
                {
                    std::cerr << "Erro na linha " << currentToken.line 
                              << ": Esperado '(' após nome do método\n";
                    errorFlag = true;
                    return nullptr;
                }
            }
            else
            {
                std::cerr << "Erro na linha " << currentToken.line 
                          << ": Esperado identificador de método após '.'\n";
                errorFlag = true;
                return nullptr;
            }
        }
    }
    return leftExpr;
}

std::shared_ptr<ExprNode> expr_primary()
{
    int line = currentToken.line; // Guarda a linha do token atual para definir nos nós AST
    if (currentToken.type == TokenType::KEYWORD && currentToken.lexeme == "if")
    {
        currentToken = lexer.nextToken(); // Consome 'if'
        auto cond = expr();
        if (currentToken.type == TokenType::KEYWORD && currentToken.lexeme == "then")
        {
            currentToken = lexer.nextToken(); // Consome 'then'
            auto thenExpr = expr();
            if (currentToken.type == TokenType::KEYWORD && currentToken.lexeme == "else")
            {
                currentToken = lexer.nextToken(); // Consome 'else'
                auto elseExpr = expr();
                if (currentToken.type == TokenType::KEYWORD && currentToken.lexeme == "fi")
                {
                    currentToken = lexer.nextToken(); // Consome 'fi'
                    auto ifNode = std::make_shared<IfASTNode>(cond, thenExpr, elseExpr);
                    ifNode->line = line; // Define a linha do nó 'if'
                    return ifNode;
                }
                else
                {
                    std::cerr << "Erro na linha " << currentToken.line << ": Esperado 'fi'\n";
                    errorFlag = true;
                    return nullptr;
                }
            }
            else
            {
                std::cerr << "Erro na linha " << currentToken.line << ": Esperado 'else'\n";
                errorFlag = true;
                return nullptr;
            }
        }
        else
        {
            std::cerr << "Erro na linha " << currentToken.line << ": Esperado 'then'\n";
            errorFlag = true;
            return nullptr;
        }
    }
    else if (currentToken.type == TokenType::KEYWORD && currentToken.lexeme == "while")
    {
        currentToken = lexer.nextToken(); // Consome 'while'
        auto cond = expr();
        if (currentToken.type == TokenType::KEYWORD && currentToken.lexeme == "loop")
        {
            currentToken = lexer.nextToken(); // Consome 'loop'
            auto body = expr();
            if (currentToken.type == TokenType::KEYWORD && currentToken.lexeme == "pool")
            {
                currentToken = lexer.nextToken(); // Consome 'pool'
                auto whileNode = std::make_shared<WhileASTNode>(cond, body);
                whileNode->line = line; // Define a linha do nó 'while'
                return whileNode;
            }
            else
            {
                std::cerr << "Erro na linha " << currentToken.line << ": Esperado 'pool'\n";
                errorFlag = true;
                return nullptr;
            }
        }
        else
        {
            std::cerr << "Erro na linha " << currentToken.line << ": Esperado 'loop'\n";
            errorFlag = true;
            return nullptr;
        }
    }
    else if (currentToken.type == TokenType::KEYWORD && currentToken.lexeme == "let")
    {
        currentToken = lexer.nextToken(); // Consome 'let'
        std::vector<LetBinding> bindings;
        
        auto parse_binding = [this, &bindings]() {
            if (currentToken.type == TokenType::OBJECTID)
            {
                std::string varName = currentToken.lexeme;
                currentToken = lexer.nextToken();
                if (currentToken.type == TokenType::DOISPONTOS)
                {
                    currentToken = lexer.nextToken();
                    if (currentToken.type == TokenType::TYPEID)
                    {
                        std::string varType = currentToken.lexeme;
                        currentToken = lexer.nextToken();
                        std::shared_ptr<ExprNode> initExpr = nullptr;
                        if (currentToken.type == TokenType::ATRIBUIR)
                        {
                            currentToken = lexer.nextToken();
                            initExpr = expr();
                        }
                        bindings.push_back({varName, varType, initExpr});
                    }
                    else
                    {
                        std::cerr << "Erro na linha " << currentToken.line << ": Esperado TYPEID em 'let'\n";
                        errorFlag = true;
                        return;
                    }
                }
                else
                {
                    std::cerr << "Erro na linha " << currentToken.line << ": Esperado ':' em 'let'\n";
                    errorFlag = true;
                    return;
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
            auto body = expr();
            auto letNode = std::make_shared<LetASTNode>(bindings, body);
            letNode->line = line; // Define a linha do nó 'let'
            return letNode;
        }
        else
        {
            std::cerr << "Erro na linha " << currentToken.line << ": Esperado 'in' em 'let'\n";
            errorFlag = true;
            return nullptr;
        }
    }
    else if (currentToken.type == TokenType::ABRECHAVES)
    {
        currentToken = lexer.nextToken(); // Consome '{'
        std::vector<std::shared_ptr<ExprNode>> expressions;
        while (currentToken.type != TokenType::FECHACHAVES && currentToken.type != TokenType::END_OF_FILE)
        {
            expressions.push_back(expr());
            if (currentToken.type == TokenType::PONTOEVIRGULA)
            {
                currentToken = lexer.nextToken(); // Consome ';'
            }
            else
            {
                std::cerr << "Erro na linha " << currentToken.line << ": Esperado ';' no bloco\n";
                errorFlag = true;
                return nullptr;
            }
        }
        if (currentToken.type == TokenType::FECHACHAVES)
        {
            currentToken = lexer.nextToken(); // Consome '}'
            auto blockNode = std::make_shared<BlockASTNode>(expressions);
            blockNode->line = line; // Define a linha do nó de bloco
            return blockNode;
        }
        return nullptr;
    }
    else if (currentToken.type == TokenType::KEYWORD && currentToken.lexeme == "case")
    {
        currentToken = lexer.nextToken(); // Consome 'case'
        auto caseExpr = expr();

        if (currentToken.type == TokenType::KEYWORD && currentToken.lexeme == "of")
        {
            currentToken = lexer.nextToken(); // Consome 'of'
            std::vector<CaseBranch> branches;

            while (currentToken.type == TokenType::OBJECTID)
            {
                std::string varName = currentToken.lexeme;
                currentToken = lexer.nextToken(); // Consome ID

                if (currentToken.type == TokenType::DOISPONTOS)
                {
                    currentToken = lexer.nextToken(); // Consome ':'
                    if (currentToken.type == TokenType::TYPEID)
                    {
                        std::string varType = currentToken.lexeme;
                        currentToken = lexer.nextToken(); // Consome tipo
                        if (currentToken.type == TokenType::SETAD)
                        {
                            currentToken = lexer.nextToken(); // Consome '=>'
                            auto branchExpr = expr();         // Processa ramo do case
                            if (currentToken.type == TokenType::PONTOEVIRGULA)
                            {
                                currentToken = lexer.nextToken(); // Consome ';'
                                branches.push_back({varName, varType, branchExpr});
                            }
                            else
                            {
                                std::cerr << "Erro na linha " << currentToken.line << ": Esperado ';' no caso\n";
                                errorFlag = true;
                                return nullptr;
                            }
                        }
                        else
                        {
                            std::cerr << "Erro na linha " << currentToken.line << ": Esperado '=>' no caso\n";
                            errorFlag = true;
                            return nullptr;
                        }
                    }
                    else
                    {
                        std::cerr << "Erro na linha " << currentToken.line << ": Esperado TYPEID no caso\n";
                        errorFlag = true;
                        return nullptr;
                    }
                }
                else
                {
                    std::cerr << "Erro na linha " << currentToken.line << ": Esperado ':' no caso\n";
                    errorFlag = true;
                    return nullptr;
                }
            }
            if (branches.empty())// Validação da gramática do COOL: ao menos um ramo é obrigatório
            {
                std::cerr << "Erro na linha " << currentToken.line << ": A instrução 'case' exige pelo menos um ramo\n";
                errorFlag = true;
                return nullptr;
            }
            if (currentToken.type == TokenType::KEYWORD && currentToken.lexeme == "esac")
            {
                currentToken = lexer.nextToken(); // Consome 'esac'
                auto caseNode = std::make_shared<CaseASTNode>(caseExpr, branches);
                caseNode->line = line; // Define a linha do nó 'case'
                return caseNode;
            }
            else
            {
                std::cerr << "Erro na linha " << currentToken.line << ": Esperado 'esac'\n";
                errorFlag = true;
                return nullptr;
            }
        }
        else
        {
            std::cerr << "Erro na linha " << currentToken.line << ": Esperado 'of' no 'case'\n";
            errorFlag = true;
            return nullptr;
        }
    }
    else if (currentToken.type == TokenType::OBJECTID)
    {
        std::string name = currentToken.lexeme;
        currentToken = lexer.nextToken(); // Consome o ID do objeto
        
        // Trata chamada de método local: id(...)
        if (currentToken.type == TokenType::ABREPARENTESES)
        {
            currentToken = lexer.nextToken(); // Consome '('
            std::vector<std::shared_ptr<ExprNode>> args;
            if (currentToken.type != TokenType::FECHAPARENTESES)
            {
                args.push_back(expr());
                while (currentToken.type == TokenType::VIRGULA)
                {
                    currentToken = lexer.nextToken(); // Consome ','
                    args.push_back(expr());
                }
            }
            if (currentToken.type == TokenType::FECHAPARENTESES)
            {
                currentToken = lexer.nextToken(); // Consome ')'
                auto dispatchNode = std::make_shared<StaticDispatchASTNode>(name, args);
                dispatchNode->line = line; // Define a linha do nó de dispatch
                return dispatchNode;
            }
            else
            {
                std::cerr << "Erro na linha " << currentToken.line << ": Esperado ')'\n";
                errorFlag = true;
                return nullptr;
            }
        }
        // Se não for '(', trata-se de uma variável simples. O token já foi consumido.
        auto objectNode = std::make_shared<ObjectASTNode>(name);
        objectNode->line = line; // Define a linha do nó de objeto
        return objectNode;
    }
    else if(currentToken.type == TokenType::INT)
    {
        int val = 0;
        try {
            val = std::stoi(currentToken.lexeme);
        } 
        catch (const std::out_of_range&) {
            std::cerr << "Erro na linha " << line << ": Constante inteira fora do limite suportado (overflow/underflow): '" 
                      << currentToken.lexeme << "'\n";
            errorFlag = true;
            return nullptr;
        }
        catch (const std::invalid_argument&) {
            std::cerr << "Erro na linha " << line << ": Literal inteiro inválido: '" 
                      << currentToken.lexeme << "'\n";
            errorFlag = true;
            return nullptr;
        }
        currentToken = lexer.nextToken();
        auto intNode = std::make_shared<IntASTNode>(val);
        intNode->line = line; // Define a linha do nó inteiro
        return intNode;
    }
    else if (currentToken.type == TokenType::STRING)
    {
        std::string str = currentToken.lexeme;
        currentToken = lexer.nextToken();
        auto stringNode = std::make_shared<StringASTNode>(str);
        stringNode->line = line; // Define a linha do nó de string
        return stringNode;
    }
    else if (
             currentToken.type == TokenType::BOOLEAN ||
             currentToken.type == TokenType::SELF ||
             (currentToken.type == TokenType::KEYWORD && 
              (currentToken.lexeme == "true" || currentToken.lexeme == "false" || currentToken.lexeme == "self")))
    {
        std::string lex = currentToken.lexeme;
        currentToken = lexer.nextToken();

        if (lex == "true") 
        {
            auto boolNode = std::make_shared<BoolASTNode>(true);
            boolNode->line = line; // Define a linha do nó booleano
            return boolNode;
        }
        if (lex == "false") 
        {
            auto boolNode = std::make_shared<BoolASTNode>(false);
            boolNode->line = line; // Define a linha do nó booleano
            return boolNode;
        }
        auto selfNode = std::make_shared<ObjectASTNode>(lex);
        selfNode->line = line; // Define a linha do nó 'self'
        return selfNode; // 'self'
    }
    else if (currentToken.type == TokenType::ABREPARENTESES)
    {
        currentToken = lexer.nextToken(); // Consome '('
        auto innerExpr = expr();
        innerExpr->line = line; // Define a linha do nó interno
        if (currentToken.type == TokenType::FECHAPARENTESES)
        {
            currentToken = lexer.nextToken(); // Consome ')'
            return innerExpr;
        }
        else
        {
            std::cerr << "Erro na linha " << currentToken.line << ": Esperado ')'\n";
            errorFlag = true;
            return nullptr;
        }
    }
    else
    {
        std::cerr << "Erro na linha " << currentToken.line << ": Expressão inválida\n";
        errorFlag = true;
        return nullptr;
    }
}
};


class SymbolTable {
private:
    // Lista de escopos (do mais interno ao mais externo)
    std::vector<std::unordered_map<std::string, std::string>> scopes;

public:
    void enterScope() {
        scopes.push_back({});
    }

    void exitScope() {
        if (!scopes.empty()) {
            scopes.pop_back();
        }
    }

    // Adiciona uma variável ao escopo atual
    bool addSymbol(const std::string& name, const std::string& type) {
        if (scopes.empty()) return false;
        if (scopes.back().find(name) != scopes.back().end()) {
            return false; // Redefinição no mesmo escopo
        }
        scopes.back()[name] = type;
        return true;
    }

    // Procura uma variável do escopo atual até o escopo global
    std::string lookup(const std::string& name) const {
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            auto found = it->find(name);
            if (found != it->end()) {
                return found->second;
            }
        }
        return ""; // Não encontrado
    }
};

class semantico {
private:
    sintatico sintaxAnalyzer;
    std::shared_ptr<ProgramASTNode> astRoot;
    bool errorFlag = false;

    // Tabela de classes registradas no programa
    std::unordered_map<std::string, std::shared_ptr<ClassASTNode>> classTable;

    // Representação de um método coletado
    struct MethodSymbol {
        std::string name;
        std::string returnType;
        std::vector<std::pair<std::string, std::string>> formals; // {nome_param, tipo_param}
        std::shared_ptr<FeatureASTNode> node;
    };

    // Representação de um atributo coletado
    struct AttributeSymbol {
        std::string name;
        std::string type;
        std::shared_ptr<FeatureASTNode> node;
    };

    // Mapeamento: [NomeClasse] -> [NomeMembro -> Símbolo] tabelas globais de interface das classes
    std::unordered_map<std::string, std::unordered_map<std::string, AttributeSymbol>> classAttributes;
    std::unordered_map<std::string, std::unordered_map<std::string, MethodSymbol>> classMethods;

    SymbolTable symbolTable; // Instância única reutilizada durante o percurso da AST

    void logError(int line, const std::string& msg) {
        std::cerr << "Erro Semântico: " << msg << "linha " << line << "\n";
        errorFlag = true;
    }

    void installBasicClasses() {
        // 1. Class Object (Sem pai)
        auto objectClass = std::make_shared<ClassASTNode>("Object", "");
        // Métodos nativos de Object: abort() : Object, type_name() : String, copy() : SELF_TYPE
        objectClass->features.push_back(
            std::make_shared<FeatureASTNode>("abort", std::vector<std::shared_ptr<FormalASTNode>>{}, "Object", nullptr)
        );
        objectClass->features.push_back(
            std::make_shared<FeatureASTNode>("type_name", std::vector<std::shared_ptr<FormalASTNode>>{}, "String", nullptr)
        );
        objectClass->features.push_back(
            std::make_shared<FeatureASTNode>("copy", std::vector<std::shared_ptr<FormalASTNode>>{}, "SELF_TYPE", nullptr)
        );
        classTable["Object"] = objectClass;

        // 2. Class IO (inherits Object)
        auto ioClass = std::make_shared<ClassASTNode>("IO", "Object");
        // out_string(x : String) : SELF_TYPE
        ioClass->features.push_back(
            std::make_shared<FeatureASTNode>("out_string", 
                std::vector<std::shared_ptr<FormalASTNode>>{ std::make_shared<FormalASTNode>("x", "String") }, 
                "SELF_TYPE", nullptr)
        );
        // out_int(x : Int) : SELF_TYPE
        ioClass->features.push_back(
            std::make_shared<FeatureASTNode>("out_int", 
                std::vector<std::shared_ptr<FormalASTNode>>{ std::make_shared<FormalASTNode>("x", "Int") }, 
                "SELF_TYPE", nullptr)
        );
        // in_string() : String
        ioClass->features.push_back(
            std::make_shared<FeatureASTNode>("in_string", std::vector<std::shared_ptr<FormalASTNode>>{}, "String", nullptr)
        );
        // in_int() : Int
        ioClass->features.push_back(
            std::make_shared<FeatureASTNode>("in_int", std::vector<std::shared_ptr<FormalASTNode>>{}, "Int", nullptr)
        );
        classTable["IO"] = ioClass;

        // 3. Class Int (inherits Object)
        auto intClass = std::make_shared<ClassASTNode>("Int", "Object");
        classTable["Int"] = intClass;

        // 4. Class Bool (inherits Object)
        auto boolClass = std::make_shared<ClassASTNode>("Bool", "Object");
        classTable["Bool"] = boolClass;

        // 5. Class String (inherits Object)
        auto stringClass = std::make_shared<ClassASTNode>("String", "Object");
        // length() : Int
        stringClass->features.push_back(
            std::make_shared<FeatureASTNode>("length", std::vector<std::shared_ptr<FormalASTNode>>{}, "Int", nullptr)
        );
        // concat(s : String) : String
        stringClass->features.push_back(
            std::make_shared<FeatureASTNode>("concat", 
                std::vector<std::shared_ptr<FormalASTNode>>{ std::make_shared<FormalASTNode>("s", "String") }, 
                "String", nullptr)
        );
        // substr(i : Int, l : Int) : String
        stringClass->features.push_back(
            std::make_shared<FeatureASTNode>("substr", 
                std::vector<std::shared_ptr<FormalASTNode>>{ 
                    std::make_shared<FormalASTNode>("i", "Int"),
                    std::make_shared<FormalASTNode>("l", "Int")
                }, 
                "String", nullptr)
        );
        classTable["String"] = stringClass;
    }

    // Passagem 1: Validação do Grafo de Herança
    void buildInheritanceGraph() {
        // 1. Instala as classes básicas do COOL
        installBasicClasses();

        // 1. Registra as classes do programa
        for (const auto& cls : astRoot->classes) {
            if (classTable.count(cls->name) > 0) {
                logError(cls->line, "Classe " + cls->name + " já foi definida(ou é uma classe básica reservada).");
            } else {
                classTable[cls->name] = cls;
            }
        }

        // 2. Verifica se os pais existem e proíbe herança de tipos primários
        for (const auto& [name, cls] : classTable) {
            if(name == "Object"){
                continue;
            } else if (cls->parent == "Int" || cls->parent == "String" || cls->parent == "Bool") {
                logError(cls->line, "Classe " + name + " não pode herdar de " + cls->parent + ".");
            } else if (cls->parent != "Object" && classTable.count(cls->parent) == 0) {
                logError(cls->line, "Classe " + name + " herda de classe não declarada " + cls->parent + ".");
            }
        }

        // Se já houver erros de herança inexistente ou redefinição, evita procurar ciclos falsos
        if (errorFlag) return;

        // 3. Detecção de Ciclos de Herança
        for (const auto& [name, cls] : classTable) {
            if (name == "Object") continue;

            std::unordered_set<std::string> visited;
            std::string current = name;

            while (current != "Object" && !current.empty()) {
                // Se a classe atual já foi visitada nesta travessia, encontramos um ciclo
                if (visited.count(current) > 0) {
                    logError(cls->line, "Ciclo de herança detectado envolvendo a classe '" + name + "'.");
                    break;
                }

                visited.insert(current);

                // Avança para o pai no grafo
                if (classTable.count(current) > 0) {
                    current = classTable[current]->parent;
                } else {
                    // Pai não pertence às classes do usuário (ex: Object ou erro tratado)
                    break;
                }
            }
        }
    }

    // Passagem 2: Coleta de Atributos e Métodos

    // Busca se um atributo já foi definido em alguma classe ancestral
    bool isAttributeInAncestors(const std::string& className, const std::string& attrName) {
        std::string current = classTable[className]->parent;
        while (!current.empty() && classTable.count(current) > 0) {
            if (classAttributes[current].count(attrName) > 0) {
                return true;
            }
            current = classTable[current]->parent;
        }
        return false;
    }

    // Busca se um método existe em alguma classe ancestral para validar a assinatura do override
    MethodSymbol* findMethodInAncestors(const std::string& className, const std::string& methodName) {
        std::string current = classTable[className]->parent;
        while (!current.empty() && classTable.count(current) > 0) {
            if (classMethods[current].count(methodName) > 0) {
                return &classMethods[current][methodName];
            }
            current = classTable[current]->parent;
        }
        return nullptr;
    }

    void collectSymbolsForClass(const std::shared_ptr<ClassASTNode>& cls) {
        auto& attrs = classAttributes[cls->name];
        auto& methods = classMethods[cls->name];

        for (const auto& feat : cls->features) {
            // -----------------------------------------------------------------
            // 1. PROCESSAMENTO DE ATRIBUTOS
            // -----------------------------------------------------------------
            if (!feat->isMethod) {
                // Regra: Atributo não pode se chamar 'self'
                if (feat->name == "self") {
                    logError(cls->line, "'self' não pode ser usado como nome de atributo na classe '" + cls->name + "'.");
                    continue;
                }

                // Regra: Duplicata na mesma classe
                if (attrs.count(feat->name) > 0) {
                    logError(cls->line, "Atributo '" + feat->name + "' redefinido na classe '" + cls->name + "'.");
                    continue;
                }

                // Regra: Sobrescrita ilegal de atributo herdado
                if (isAttributeInAncestors(cls->name, feat->name)) {
                    logError(cls->line, "Atributo '" + feat->name + "' na classe '" + cls->name + 
                                "' redefine um atributo herdado de uma classe ancestral.");
                    continue;
                }

                attrs[feat->name] = AttributeSymbol{feat->name, feat->returnType, feat};
            }
            // -----------------------------------------------------------------
            // 2. PROCESSAMENTO DE MÉTODOS
            // -----------------------------------------------------------------
            else {
                // Regra: Duplicata na mesma classe
                if (methods.count(feat->name) > 0) {
                    logError(cls->line, "Método '" + feat->name + "' redefinido na classe '" + cls->name + "'.");
                    continue;
                }

                std::vector<std::pair<std::string, std::string>> formalsList;
                std::unordered_set<std::string> paramNames;

                for (const auto& formal : feat->formals) {
                    if (formal->name == "self") {
                        logError(cls->line, "'self' não pode ser usado como nome de parâmetro no método '" + 
                                    feat->name + "' da classe '" + cls->name + "'.");
                    }
                    if (paramNames.count(formal->name) > 0) {
                        logError(cls->line, "Parâmetro formal '" + formal->name + "' duplicado no método '" + 
                                    feat->name + "' da classe '" + cls->name + "'.");
                    }
                    paramNames.insert(formal->name);
                    formalsList.push_back({formal->name, formal->type});
                }

                MethodSymbol newMethod{feat->name, feat->returnType, formalsList, feat};

                // Regra: Validação de Override de Método Herdado
                MethodSymbol* inheritedMethod = findMethodInAncestors(cls->name, feat->name);
                if (inheritedMethod != nullptr) {
                    // Checa tipo de retorno
                    if (inheritedMethod->returnType != newMethod.returnType) {
                        logError(cls->line, "Sobrescrita do método '" + feat->name + "' na classe '" + cls->name + 
                                    "' altera o tipo de retorno de '" + inheritedMethod->returnType + 
                                    "' para '" + newMethod.returnType + "'.");
                    }

                    // Checa quantidade de parâmetros
                    if (inheritedMethod->formals.size() != newMethod.formals.size()) {
                        logError(cls->line, "Sobrescrita do método '" + feat->name + "' na classe '" + cls->name + 
                                    "' altera o número de parâmetros formais.");
                    } else {
                        // Checa os tipos de cada parâmetro
                        for (size_t i = 0; i < newMethod.formals.size(); ++i) {
                            if (inheritedMethod->formals[i].second != newMethod.formals[i].second) {
                                logError(cls->line, "Sobrescrita do método '" + feat->name + "' na classe '" + cls->name + 
                                            "' altera o tipo do parâmetro '" + newMethod.formals[i].first + "'.");
                            }
                        }
                    }
                }

                methods[feat->name] = newMethod;
            }
        }
    }

    void collectAllSymbols() {
        // 1. Constrói a lista de adjacência do grafo invertido (Pai -> Filhos)
        std::unordered_map<std::string, std::vector<std::shared_ptr<ClassASTNode>>> childrenMap;

        for (const auto& [name, cls] : classTable) {
            if (name != "Object") {
                childrenMap[cls->parent].push_back(cls);
            }
        }
        
        // 2. Fila para travessia em largura (BFS) a partir da raiz "Object"
        std::queue<std::string> processQueue;
        processQueue.push("Object");

        // 3. Processa em ordem topológica garantida (Pais sempre antes das Filhas)
        while (!processQueue.empty()) {
            std::string currentClassName = processQueue.front();
            processQueue.pop();

            auto clsNode = classTable[currentClassName];

            // Processa a coleta de símbolos da classe atual (seus pais já foram processados!)
            collectSymbolsForClass(clsNode);

            // Adiciona todas as filhas diretas na fila para processamento posterior
            if (childrenMap.count(currentClassName) > 0) {
                for (const auto& childNode : childrenMap[currentClassName]) {
                    processQueue.push(childNode->name);
                }
            }
        }
    }


    // Função auxiliar para buscar atributo em toda a cadeia de herança
    std::string lookupAttributeInHierarchy(const std::string& className, const std::string& attrName) {
        std::string current = className;
        while (!current.empty() && classTable.count(current) > 0) {
            if (classAttributes[current].count(attrName) > 0) {
                return classAttributes[current][attrName].type;
            }
            current = classTable[current]->parent;
        }
        return "";
    }

    // Carrega todos os atributos herdados e próprios da classe no escopo atual
    void populateClassScope(const std::string& className) {
        std::vector<std::string> hierarchy;
        std::string current = className;

        // Coleta os ancestrais até Object
        while (!current.empty() && classTable.count(current) > 0) {
            hierarchy.push_back(current);
            current = classTable[current]->parent;
        }

        // Injeta os atributos do ancestral mais distante (Object) até a classe atual
        for (auto it = hierarchy.rbegin(); it != hierarchy.rend(); ++it) {
            for (const auto& [attrName, attrSym] : classAttributes[*it]) {
                symbolTable.addSymbol(attrName, attrSym.type);
            }
        }

        // Adiciona a pseudo-variável 'self' apontando para o tipo da própria classe
        symbolTable.addSymbol("self", "SELF_TYPE");
    }

    // Resolve o tipo real se for SELF_TYPE
    std::string resolveType(const std::string& type, const std::string& currentClass) const {
        return (type == "SELF_TYPE") ? currentClass : type;
    }

    // Verifica se child <= parent (Conformidade de Tipos)
    bool isSubtype(std::string child, std::string parent, const std::string& currentClass) {
        child = resolveType(child, currentClass);
        parent = resolveType(parent, currentClass);

        if (child == parent) return true;

        std::string curr = child;
        while (!curr.empty() && classTable.count(curr) > 0) {
            if (curr == parent) return true;
            curr = classTable[curr]->parent;
        }
        return (parent == "Object"); // Todas as classes derivam de Object
    }

    // Calcula o Menor Ancestral Comum (Least Upper Bound - LUB)
    std::string lub(std::string type1, std::string type2, const std::string& currentClass) {
        type1 = resolveType(type1, currentClass);
        type2 = resolveType(type2, currentClass);

        if (type1 == type2) return type1;

        // Coleta o caminho de type1 até a raiz Object
        std::vector<std::string> path1;
        std::string curr = type1;
        while (!curr.empty() && classTable.count(curr) > 0) {
            path1.push_back(curr);
            curr = classTable[curr]->parent;
        }
        path1.push_back("Object");

        // Sobe por type2 até encontrar o primeiro ancestral em comum
        curr = type2;
        while (!curr.empty() && classTable.count(curr) > 0) {
            for (const auto& ancestor : path1) {
                if (curr == ancestor) return curr;
            }
            curr = classTable[curr]->parent;
        }
        return "Object";
    }

    // Função recursiva principal que percorre a AST de expressões e retorna seu tipo inferido
    std::string checkExprType(const std::shared_ptr<ExprNode>& expr, const std::string& currentClass) {
        if (!expr) return "Object";

        // 1. TERMINAIS E LITERAIIS
        if (std::dynamic_pointer_cast<IntASTNode>(expr)) return "Int";
        if (std::dynamic_pointer_cast<StringASTNode>(expr)) return "String";
        if (std::dynamic_pointer_cast<BoolASTNode>(expr)) return "Bool";

        // Identificadores / Variáveis
        if (auto objNode = std::dynamic_pointer_cast<ObjectASTNode>(expr)) {
            if (objNode->name == "self") return "SELF_TYPE";

            std::string type = symbolTable.lookup(objNode->name);
            if (type.empty()) {
                logError(objNode->line, "Identificador '" + objNode->name + "' não foi declarado no escopo.");
                return "Object";
            }
            return type;
        }

        // 2. ATRIBUIÇÃO (x <- expr)
        if (auto assignNode = std::dynamic_pointer_cast<AssignASTNode>(expr)) {
            if (assignNode->name == "self") {
                logError(assignNode->line, "Não é permitido atribuir valor a 'self'.");
                return "Object";
            }

            std::string varType = symbolTable.lookup(assignNode->name);
            if (varType.empty()) {
                logError(assignNode->line, "Variável '" + assignNode->name + "' não declarada.");
                return "Object";
            }

            std::string exprType = checkExprType(assignNode->value, currentClass);
            if (!isSubtype(exprType, varType, currentClass)) {
                logError(assignNode->line, "Tipo incompatível na atribuição de '" + assignNode->name + 
                            "': esperado '" + varType + "', mas obtido '" + exprType + "'.");
            }
            return exprType;
        }

        // 3. OPERAÇÕES ARITMÉTICAS E LÓGICAS (BinaryOpASTNode)
        if (auto binOp = std::dynamic_pointer_cast<BinaryOpASTNode>(expr)) {
            std::string leftType = checkExprType(binOp->left, currentClass);
            std::string rightType = checkExprType(binOp->right, currentClass);

            // Operadores Aritméticos (+, -, *, /)
            if (binOp->op == "+" || binOp->op == "-" || binOp->op == "*" || binOp->op == "/") {
                if (leftType != "Int" || rightType != "Int") {
                    logError(binOp->line, "Operador '" + binOp->op + "' exige operandos do tipo 'Int'.");
                }
                return "Int";
            }

            // Operadores de Comparação Relacional (<, <=)
            if (binOp->op == "<" || binOp->op == "<=") {
                if (leftType != "Int" || rightType != "Int") {
                    logError(binOp->line, "Operador '" + binOp->op + "' exige operandos do tipo 'Int'.");
                }
                return "Bool";
            }

            // Operador de Igualdade (=)
            if (binOp->op == "=") {
                if ((leftType == "Int" || leftType == "String" || leftType == "Bool" ||
                    rightType == "Int" || rightType == "String" || rightType == "Bool") &&
                    (leftType != rightType)) {
                    logError(binOp->line, "Comparação de igualdade '=' entre tipos básicos distintos ('" + 
                                leftType + "' e '" + rightType + "').");
                }
                return "Bool";
            }
        }

        // 4. OPERAÇÕES UNÁRIAS (UnaryOpASTNode)
        if (auto unOp = std::dynamic_pointer_cast<UnaryOpASTNode>(expr)) {
            std::string operandType = checkExprType(unOp->operand, currentClass);

            if (unOp->op == "~") { // Negação de inteiro
                if (operandType != "Int") logError(unOp->line, "Operador '~' exige tipo 'Int'.");
                return "Int";
            }
            if (unOp->op == "not") { // Negação lógica
                if (operandType != "Bool") logError(unOp->line, "Operador 'not' exige tipo 'Bool'.");
                return "Bool";
            }
            if (unOp->op == "isvoid") { // Teste de ponteiro nulo
                return "Bool";
            }
        }

        // 5. INSTANCIAÇÃO (NewASTNode)
        if (auto newObj = std::dynamic_pointer_cast<NewASTNode>(expr)) {
            if (newObj->typeName != "SELF_TYPE" && classTable.count(newObj->typeName) == 0) {
                logError(newObj->line, "Instanciação 'new " + newObj->typeName + "' de classe não declarada.");
                return "Object";
            }
            return newObj->typeName;
        }

        // 6. DISPATCH DINÂMICO E ESTÁTICO (DispatchASTNode)
        if (auto dispatch = std::dynamic_pointer_cast<DispatchASTNode>(expr)) {
            std::string targetType = checkExprType(dispatch->expr, currentClass);
            std::string realTargetType = resolveType(targetType, currentClass);

            // Se for Dispatch Estático (expr@TYPE.method(...))
            if (!dispatch->typeExpr.empty()) {
                if (!isSubtype(targetType, dispatch->typeExpr, currentClass)) {
                    logError(dispatch->line, "Expressão do dispatch estático não é do tipo '" + dispatch->typeExpr + "'.");
                    return "Object";
                }
                realTargetType = dispatch->typeExpr;
            }

            // Procura pelo método na classe ou em seus ancestrais
            MethodSymbol* method = findMethodInAncestors(realTargetType, dispatch->name);
            if (!method && classMethods[realTargetType].count(dispatch->name) > 0) {
                method = &classMethods[realTargetType][dispatch->name];
            }

            if (!method) {
                logError(dispatch->line, "Método '" + dispatch->name + "' não encontrado na classe '" + realTargetType + "'.");
                return "Object";
            }

            // Valida argumentos do dispatch
            if (dispatch->args.size() != method->formals.size()) {
                logError(dispatch->line, "Número incorreto de argumentos para o método '" + dispatch->name + "'.");
            } else {
                for (size_t i = 0; i < dispatch->args.size(); ++i) {
                    std::string argType = checkExprType(dispatch->args[i], currentClass);
                    std::string paramType = method->formals[i].second;
                    if (!isSubtype(argType, paramType, currentClass)) {
                        logError(dispatch->line, "Argumento " + std::to_string(i + 1) + " do método '" + dispatch->name + 
                                    "': incompatibilidade de tipo.");
                    }
                }
            }

            return (method->returnType == "SELF_TYPE") ? targetType : method->returnType;
        }

        // Chamada de Método Local (self.method(...))
        if (auto localCall = std::dynamic_pointer_cast<StaticDispatchASTNode>(expr)) {
            MethodSymbol* method = findMethodInAncestors(currentClass, localCall->name);
            if (!method && classMethods[currentClass].count(localCall->name) > 0) {
                method = &classMethods[currentClass][localCall->name];
            }

            if (!method) {
                logError(localCall->line, "Método local '" + localCall->name + "' não declarado na classe '" + currentClass + "'.");
                return "Object";
            }

            if (localCall->args.size() != method->formals.size()) {
            logError(localCall->line, "Número incorreto de argumentos para o método local '" + localCall->name + "'.");
            } else {
                for (size_t i = 0; i < localCall->args.size(); ++i) {
                    std::string argType = checkExprType(localCall->args[i], currentClass);
                    std::string paramType = method->formals[i].second;
                    if (!isSubtype(argType, paramType, currentClass)) {
                        logError(localCall->line, "Argumento " + std::to_string(i + 1) + " da chamada local '" + 
                                    localCall->name + "': incompatibilidade de tipo.");
                    }
                }
            }

            return (method->returnType == "SELF_TYPE") ? "SELF_TYPE" : method->returnType;
        }

        // 7. CONTROLE DE FLUXO: IF, WHILE, BLOCK
        if (auto ifNode = std::dynamic_pointer_cast<IfASTNode>(expr)) {
            std::string condType = checkExprType(ifNode->cond, currentClass);
            if (condType != "Bool") {
                logError(ifNode->line, "Condição da instrução 'if' deve ser do tipo 'Bool'.");
            }

            std::string thenType = checkExprType(ifNode->thenExpr, currentClass);
            std::string elseType = checkExprType(ifNode->elseExpr, currentClass);

            // O retorno de um IF é o Menor Ancestral Comum das duas ramificações
            return lub(thenType, elseType, currentClass);
        }

        if (auto whileNode = std::dynamic_pointer_cast<WhileASTNode>(expr)) {
            std::string condType = checkExprType(whileNode->cond, currentClass);
            if (condType != "Bool") {
                logError(whileNode->line, "Condição do laço 'while' deve ser do tipo 'Bool'.");
            }

            checkExprType(whileNode->body, currentClass);
            return "Object"; // Laços em COOL sempre retornam tipo Object
        }

        if (auto blockNode = std::dynamic_pointer_cast<BlockASTNode>(expr)) {
            std::string lastType = "Object";
            for (const auto& e : blockNode->expressions) {
                lastType = checkExprType(e, currentClass);
            }
            return lastType; // Retorna o tipo da última expressão do bloco
        }

        // 8. DECLARAÇÃO DE VARIÁVEIS LOCAIS (LetASTNode)
        if (auto letNode = std::dynamic_pointer_cast<LetASTNode>(expr)) {
            int scopesEntered = 0;

            for (const auto& bind : letNode->bindings) {
                if (bind.name == "self") {
                    logError(letNode->line, "'self' não pode ser declarado em um bloco 'let'.");
                    continue;
                }

                // Valida se o tipo declarado para a variável existe no programa
                if (bind.type != "SELF_TYPE" && classTable.count(bind.type) == 0) {
                    logError(letNode->line, "Tipo não declarado '" + bind.type + 
                        "' no 'let' para a variável '" + bind.name + "'.");
                }

                // A expressão de inicialização (se houver) é avaliada NO ESCOPO ANTERIOR
                // (antes que esta nova variável seja adicionada à tabela)
                if (bind.init) {
                    std::string initType = checkExprType(bind.init, currentClass);
                    if (!isSubtype(initType, bind.type, currentClass)) {
                        logError(letNode->line, "Inicialização incompatível no 'let' para a variável '" + 
                            bind.name + "': esperado '" + bind.type + "', mas obtido '" + initType + "'.");
                    }
                }

                // Cria um NOVO subescopo para este binding específico.
                // Isso permite que um binding posterior sombreie (shadow) este identificador.
                symbolTable.enterScope();
                symbolTable.addSymbol(bind.name, bind.type);
                scopesEntered++;
            }

            // Avalia o corpo do let com todos os subescopos empilhados
            std::string bodyType = checkExprType(letNode->body, currentClass);

            // Desempilha exatamente a mesma quantidade de escopos abertos para os bindings
            while (scopesEntered > 0) {
                symbolTable.exitScope();
                scopesEntered--;
            }

            return bodyType;
        }

        // 9. CASE (CaseASTNode)
        if (auto caseNode = std::dynamic_pointer_cast<CaseASTNode>(expr)) {
            checkExprType(caseNode->expr, currentClass);

            std::string resultType = "";
            std::unordered_set<std::string> branchTypes;

            for (const auto& branch : caseNode->branches) {
                // Regra 1: O identificador da variável do ramo não pode ser 'self'
                if (branch.name == "self") {
                    logError(caseNode->line, "'self' não pode ser usado como nome de variável em um ramo de 'case'.");
                }

                // Regra 2: 'SELF_TYPE' é proibido como tipo de ramo no case
                if (branch.type == "SELF_TYPE") {
                    logError(caseNode->line, "O tipo 'SELF_TYPE' não pode ser usado em uma ramificação de 'case'.");
                }

                // Regra 3: Valida se o tipo declarado existe no programa
                else if (classTable.count(branch.type) == 0) {
                    logError(caseNode->line, "Tipo não declarado '" + branch.type + "' na ramificação do 'case'.");
                }

                // Regra 4: Duplicidade de tipos nas ramificações (não pode haver dois ramos com o mesmo tipo)
                if (branchTypes.count(branch.type) > 0) {
                    logError(caseNode->line, "Tipo '" + branch.type + "' duplicado em ramificações do 'case'.");
                }
                branchTypes.insert(branch.type);

                // Cria o escopo isolado do ramo e adiciona a variável
                symbolTable.enterScope();
                symbolTable.addSymbol(branch.name, branch.type);

                std::string branchExprType = checkExprType(branch.expr, currentClass);
                symbolTable.exitScope();

                // Acumula o resultado usando LUB (Least Upper Bound / Menor Ancestral Comum)
                if (resultType.empty()) {
                    resultType = branchExprType;
                } else {
                    resultType = lub(resultType, branchExprType, currentClass);
                }
            }

            return resultType;
        }

        return "Object";
    }
    
    // Checagem de Tipos e Escopos
    void checkTypes() {
        // Garante que a classe Main e o método main() existem
        if (classTable.count("Main") == 0) {
            logError(0, "Classe 'Main' não foi declarada no programa.");
            return;
        }
        if (classMethods["Main"].count("main") == 0) {
            logError(0, "Método 'main' não foi encontrado na classe 'Main'.");
            return;
        } else {
            // Regra do COOL: o método main() da classe Main não pode aceitar parâmetros
            if (!classMethods["Main"]["main"].formals.empty()) {
                logError(0, "O método 'main' na classe 'Main' não deve receber parâmetros formais.");
            }
        }

        // 2. Percorre todas as classes do usuário para checar corpos de atributos e métodos
        for (const auto& [className, clsNode] : classTable) {
            // Ignora as classes básicas estáticas na checagem interna
            if (className == "Object" || className == "IO" || 
                className == "Int" || className == "String" || className == "Bool") {
                continue;
            }

            // Para cada classe, cria o escopo base carregando seus atributos
            symbolTable.enterScope();
            populateClassScope(className);

            // 3. Valida cada feature (atributo ou método) da classe
            for (const auto& feat : clsNode->features) {
                // CASO A: CHECAGEM DE ATRIBUTO (x : Type [<- expr])
                if (!feat->isMethod) {
                    // Valida se o tipo do atributo existe
                    if (feat->returnType != "SELF_TYPE" && classTable.count(feat->returnType) == 0) {
                        logError(feat->line, "Atributo '" + feat->name + "' na classe '" + className + 
                                    "' possui tipo não declarado '" + feat->returnType + "'.");
                    }
                    
                    // Checa a expressão de inicialização do atributo (se houver: x : Int <- expr)
                    if (feat->initExpr) {
                        std::string initType = checkExprType(feat->initExpr, className);
                        
                        // O tipo da expressão de inicialização deve ser conforme ao tipo do atributo
                        if (!isSubtype(initType, feat->returnType, className)) {
                            logError(feat->line, "Tipo de inicialização '" + initType + 
                                    "' incompatível com o tipo do atributo '" + feat->name + 
                                    "' (" + feat->returnType + ") na classe '" + className + "'.");
                        }
                    }
                }
                // CASO B: CHECAGEM DE MÉTODOS (m(f1:T1, ...): T_ret { body })
                else {
                    // Valida se o tipo de retorno do método existe
                    if (feat->returnType != "SELF_TYPE" && classTable.count(feat->returnType) == 0) {
                        logError(feat->line, "Método '" + feat->name + "' na classe '" + className + 
                                    "' possui tipo de retorno não declarado '" + feat->returnType + "'.");
                    }

                    // É um método: entra num novo escopo local para os parâmetros formais
                    symbolTable.enterScope();

                    for (const auto& formal : feat->formals) {
                        // Valida se o tipo do parâmetro formal existe
                        if (formal->type == "SELF_TYPE") {
                            logError(feat->line, "Parâmetro '" + formal->name + "' no método '" + feat->name + 
                                        "' não pode ser do tipo 'SELF_TYPE'.");
                        } else if (classTable.count(formal->type) == 0) {
                            logError(feat->line, "Parâmetro '" + formal->name + "' no método '" + feat->name + 
                                        "' possui tipo não declarado '" + formal->type + "'.");
                        }

                        symbolTable.addSymbol(formal->name, formal->type);
                    }

                    // Valida a expressão do corpo do método
                    if (feat->body) {
                        std::string bodyType = checkExprType(feat->body, className);

                        // O tipo inferido do corpo deve ser conforme ao tipo de retorno declarado
                        if (!isSubtype(bodyType, feat->returnType, className)) {
                                logError(feat->line, "Tipo do corpo do método '" + feat->name + "' ('" + bodyType + 
                                    "') incompatível com o tipo de retorno declarado '" + 
                                    feat->returnType + "' na classe '" + className + "'.");
                        }
                    }

                    symbolTable.exitScope(); // Sai do escopo dos parâmetros formais
                }
            }

            symbolTable.exitScope(); // Sai do escopo dos atributos da classe
        }
    }

public:
    explicit semantico(const std::string& src) : sintaxAnalyzer(src) {
        astRoot = sintaxAnalyzer.parse();

        if (!sintaxAnalyzer.valid()) {
            errorFlag = true;
        }
    }

    bool analyze() {
        if (!astRoot || errorFlag) {
            std::cerr << "Análise semântica abortada devido a erros sintáticos.\n";
            return false;
        }

        buildInheritanceGraph();
        if (errorFlag) return false;

        collectAllSymbols();
        if (errorFlag) return false;

        checkTypes();

        return !errorFlag;
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
    if(sim.valid())
    {
        printf("Analise sintatica concluida com sucesso. Nenhum erro encontrado.\n");
        std::shared_ptr<ProgramASTNode> ast = sim.parse();
        ast->print();
    }
    else
    {
        printf("Analise sintatica concluida com erros. Verifique as mensagens acima.\n");
    }

    semantico sem(conteudo);
    int semResult = sem.analyze();
    if (semResult) {
        std::cout << "Análise semântica concluída com sucesso. Nenhum erro encontrado.\n";
    } else {
        std::cout << "Análise semântica concluída com erros. Verifique as mensagens acima.\n";
    }
    return semResult ? 0 : 1;
}
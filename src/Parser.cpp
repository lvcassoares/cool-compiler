#include "Parser.hpp"

#include <cctype>
#include <iostream>
#include <string>

// ============================================================================
// Parser.cpp
// ----------------------------------------------------------------------------
// Analisador sintático por descida recursiva. Além de construir a AST, ele
// centraliza o avanço de tokens (advance), o lookahead de 1 token (peekNext)
// e o tratamento de TokenType::ERROR. Assim, um caractere inválido é
// reportado como erro léxico no ponto em que aparece, em vez de virar uma
// cascata de erros sintáticos.
// ============================================================================

namespace {

bool isKeyword(const Token& token, const char* keyword) {
    if (token.type != TokenType::KEYWORD) return false;
    const std::string expected(keyword);
    if (token.lexeme.size() != expected.size()) return false;
    for (size_t i = 0; i < expected.size(); ++i) {
        char actual = static_cast<char>(
            std::tolower(static_cast<unsigned char>(token.lexeme[i])));
        if (actual != expected[i]) return false;
    }
    return true;
}

}  // namespace

// Lê o próximo token. Se o lexer devolver ERROR, reporta a causa léxica e
// segue para o próximo token válido (recuperação local). Isso permite que um
// programa com vários caracteres inválidos reporte todos eles.
Token Parser::readToken() {
    Token token = lexer.nextToken();
    while (token.type == TokenType::ERROR) {
        reportLexicalError(token);
        token = lexer.nextToken();
    }
    return token;
}

void Parser::reportLexicalError(const Token& token) {
    std::string message = token.lexeme;
    if (message.size() == 1) {
        message = "Caractere inválido '" + message + "'";
    }
    std::cerr << "Erro léxico na linha " << token.line << ": " << message << "\n";
    errorFlag = true;
}

void Parser::advance() {
    if (hasBufferedToken) {
        currentToken = nextTokenBuffer;
        hasBufferedToken = false;
    } else {
        currentToken = readToken();
    }
}

Token Parser::peekNext() {
    if (!hasBufferedToken) {
        nextTokenBuffer = readToken();
        hasBufferedToken = true;
    }
    return nextTokenBuffer;
}

void Parser::synchronizeToClassOrEOF() {
    while (currentToken.type != TokenType::END_OF_FILE && !isKeyword(currentToken, "class")) {
        advance();
    }
}

std::shared_ptr<ProgramASTNode> Parser::parse() {
    advance();  // Carrega o primeiro token
    astRoot = program();
    return astRoot;
}

std::shared_ptr<ProgramASTNode> Parser::program() {
    auto progNode = std::make_shared<ProgramASTNode>();
    progNode->line = currentToken.line;  // A linha do nó raiz é a do primeiro token

    while (currentToken.type != TokenType::END_OF_FILE) {
        if (isKeyword(currentToken, "class")) {
            auto classNode = classe();
            if (classNode) {
                progNode->classes.push_back(classNode);

                if (currentToken.type == TokenType::PONTOEVIRGULA) {
                    advance();  // Consome o ';' e continua
                } else if (currentToken.type != TokenType::END_OF_FILE) {
                    std::cerr << "Erro sintático na linha " << currentToken.line
                              << ": Esperado ';' após a classe\n";
                    errorFlag = true;
                    synchronizeToClassOrEOF();
                }
            }
            // Se classe() devolveu nullptr, ela já reportou o erro e
            // sincronizou; não invente um segundo erro de ';'.
        } else {
            std::cerr << "Erro sintático na linha " << currentToken.line
                      << ": Esperado a palavra-chave 'class', mas foi encontrado \""
                      << currentToken.lexeme << "\"\n";
            errorFlag = true;
            synchronizeToClassOrEOF();
        }
    }

    // Se houve qualquer erro sintático/léxico, a AST parcial não é confiável:
    // devolvemos nullptr e o main encerra com código 1 antes da semântica.
    return errorFlag ? nullptr : progNode;
}

std::shared_ptr<ClassASTNode> Parser::classe() {
    int classLine = currentToken.line;
    advance();  // Consome 'class'

    if (currentToken.type != TokenType::TYPEID) {
        std::cerr << "Erro sintático na linha " << currentToken.line
                  << ": Esperado identificador de tipo após 'class'\n";
        errorFlag = true;
        synchronizeToClassOrEOF();
        return nullptr;
    }

    std::string className = currentToken.lexeme;
    std::string parentName = "Object";
    advance();

    if (isKeyword(currentToken, "inherits")) {
        advance();  // Consome 'inherits'

        if (currentToken.type == TokenType::TYPEID) {
            parentName = currentToken.lexeme;
            advance();  // Consome o nome da classe pai
        } else {
            std::cerr << "Erro sintático na linha " << currentToken.line
                      << ": Esperado identificador de tipo após 'inherits'\n";
            errorFlag = true;
            synchronizeToClassOrEOF();
            return nullptr;
        }
    }

    auto classNode = std::make_shared<ClassASTNode>(className, parentName);
    classNode->line = classLine;

    if (currentToken.type != TokenType::ABRECHAVES) {
        std::cerr << "Erro sintático na linha " << currentToken.line
                  << ": Esperado '{' após o nome da classe\n";
        errorFlag = true;
        synchronizeToClassOrEOF();
        return nullptr;
    }

    advance();  // Consome '{'
    while (currentToken.type != TokenType::FECHACHAVES &&
           currentToken.type != TokenType::END_OF_FILE) {
        auto feat = feature();  // Processa cada feature dentro da classe
        if (feat) {
            classNode->features.push_back(feat);
        }

        if (currentToken.type == TokenType::PONTOEVIRGULA) {
            advance();  // Consome o ';' e continua
        } else if (currentToken.type == TokenType::FECHACHAVES) {
            // Falta o ';' antes de fechar a classe. Reporta, mas deixa o laço
            // terminar para não perder a classe inteira.
            std::cerr << "Erro sintático na linha " << currentToken.line
                      << ": Esperado ';' após a feature\n";
            errorFlag = true;
        } else if (currentToken.type == TokenType::END_OF_FILE) {
            std::cerr << "Erro sintático na linha " << currentToken.line
                      << ": Esperado '}' para fechar a classe\n";
            errorFlag = true;
        } else {
            std::cerr << "Erro sintático na linha " << currentToken.line
                      << ": Esperado ';' após a feature\n";
            errorFlag = true;
            // Sincroniza no fim desta feature para tentar ler a próxima
            while (currentToken.type != TokenType::PONTOEVIRGULA &&
                   currentToken.type != TokenType::FECHACHAVES &&
                   currentToken.type != TokenType::END_OF_FILE &&
                   !isKeyword(currentToken, "class")) {
                advance();
            }
            if (currentToken.type == TokenType::PONTOEVIRGULA) {
                advance();
            }
        }
    }

    if (currentToken.type == TokenType::FECHACHAVES) {
        advance();
        return classNode;
    }

    std::cerr << "Erro sintático na linha " << currentToken.line
              << ": Esperado '}' para fechar a classe\n";
    errorFlag = true;
    synchronizeToClassOrEOF();
    return classNode;
}

std::shared_ptr<FeatureASTNode> Parser::feature() {
    int featureLine = currentToken.line;  // linha do nome da feature
    if (currentToken.type == TokenType::OBJECTID) {
        std::string featureName = currentToken.lexeme;
        advance();

        // CASO 1: MÉTODO -> ID( [ formal [[, formal]]* ] ) : TYPE { expr }
        if (currentToken.type == TokenType::ABREPARENTESES) {
            advance();
            std::vector<std::shared_ptr<FormalASTNode>> formalsList;

            while (currentToken.type != TokenType::FECHAPARENTESES &&
                   currentToken.type != TokenType::END_OF_FILE) {
                auto fNode = formal();  // Processa cada formal dentro do método

                if (fNode) {
                    formalsList.push_back(fNode);
                } else {
                    return nullptr;  // Falha no parsing do formal
                }

                if (currentToken.type == TokenType::VIRGULA) {
                    advance();  // Consome a ',' e continua
                } else if (currentToken.type != TokenType::FECHAPARENTESES) {
                    std::cerr << "Erro sintático na linha " << currentToken.line
                              << ": Esperado ',' ou ')' após o formal\n";
                    errorFlag = true;
                    return nullptr;
                }
            }
            if (currentToken.type == TokenType::FECHAPARENTESES) {
                advance();
                if (currentToken.type == TokenType::DOISPONTOS) {
                    advance();
                    if (isTypeToken()) {
                        std::string returnType = currentToken.lexeme;
                        advance();

                        if (currentToken.type == TokenType::ABRECHAVES) {
                            advance();
                            auto methodBody = expr();  // Processa a expressão do corpo
                            if (currentToken.type != TokenType::FECHACHAVES) {
                                std::cerr << "Erro sintático na linha " << currentToken.line
                                          << ": Esperado '}' para fechar o corpo do método\n";
                                errorFlag = true;

                                // Recuperação: avança até o '}' do método (ou EOF)
                                while (currentToken.type != TokenType::FECHACHAVES &&
                                       currentToken.type != TokenType::END_OF_FILE) {
                                    advance();
                                }
                                if (currentToken.type != TokenType::FECHACHAVES) {
                                    return nullptr;
                                }
                            }

                            advance();  // Consome o '}'
                            auto methodNode = std::make_shared<FeatureASTNode>(
                                featureName, formalsList, returnType, methodBody);
                            methodNode->line = featureLine;
                            return methodNode;
                        } else {
                            std::cerr << "Erro sintático na linha " << currentToken.line
                                      << ": Esperado '{' para iniciar o corpo do método\n";
                            errorFlag = true;
                            return nullptr;
                        }
                    } else {
                        std::cerr << "Erro sintático na linha " << currentToken.line
                                  << ": Esperado identificador de tipo após ':'\n";
                        errorFlag = true;
                        return nullptr;
                    }
                } else {
                    std::cerr << "Erro sintático na linha " << currentToken.line
                              << ": Esperado ':' após a lista de formais\n";
                    errorFlag = true;
                    return nullptr;
                }
            } else {
                std::cerr << "Erro sintático na linha " << currentToken.line
                          << ": Esperado ')' para fechar a lista de formais\n";
                errorFlag = true;
                return nullptr;
            }
        }
        // CASO 2: ATRIBUTO -> ID : TYPE [ <- expr ]
        else if (currentToken.type == TokenType::DOISPONTOS) {
            advance();  // Consome ':'
            if (isTypeToken()) {
                std::string attrType = currentToken.lexeme;
                advance();  // Consome o tipo (ex: String)

                std::shared_ptr<ExprNode> initExpression = nullptr;

                // Inicialização opcional com <-
                if (currentToken.type == TokenType::ATRIBUIR) {
                    advance();  // Consome '<-'
                    initExpression = expr();           // Processa a expressão inicial
                }
                // Se não houver '<-', a função termina deixando o token no ';'
                auto attrNode =
                    std::make_shared<FeatureASTNode>(featureName, attrType, initExpression);
                attrNode->line = featureLine;
                return attrNode;
            } else {
                std::cerr << "Erro sintático na linha " << currentToken.line
                          << ": Esperado identificador de tipo após ':'\n";
                errorFlag = true;
                return nullptr;
            }
        } else {
            std::cerr << "Erro sintático na linha " << currentToken.line
                      << ": Esperado '(' ou ':' após o identificador de feature\n";
            errorFlag = true;
            return nullptr;
        }
    } else {
        std::cerr << "Erro sintático na linha " << currentToken.line
                  << ": Esperado identificador de feature\n";
        errorFlag = true;
        return nullptr;
    }
}

std::shared_ptr<FormalASTNode> Parser::formal() {
    int formalLine = currentToken.line;  // linha do nome do parâmetro
    if (currentToken.type == TokenType::SELF) {
        std::cerr << "Erro sintático na linha " << currentToken.line
                  << ": 'self' não pode ser usado como nome de parâmetro\n";
        errorFlag = true;
        return nullptr;
    } else if (currentToken.type == TokenType::OBJECTID) {
        std::string paramName = currentToken.lexeme;
        advance();
        if (currentToken.type == TokenType::DOISPONTOS) {
            advance();
            if (!isTypeToken()) {
                std::cerr << "Erro sintático na linha " << currentToken.line
                          << ": Esperado identificador de tipo após ':'\n";
                errorFlag = true;
                return nullptr;
            } else {
                std::string paramType = currentToken.lexeme;
                advance();  // Consome o TYPEID e continua

                auto formalNode = std::make_shared<FormalASTNode>(paramName, paramType);
                formalNode->line = formalLine;
                return formalNode;
            }
        } else {
            std::cerr << "Erro sintático na linha " << currentToken.line
                      << ": Esperado ':' após o nome do parâmetro\n";
            errorFlag = true;
            return nullptr;
        }
    } else {
        std::cerr << "Erro sintático na linha " << currentToken.line
                  << ": Esperado identificador de parâmetro\n";
        errorFlag = true;
        return nullptr;
    }
}

std::shared_ptr<ExprNode> Parser::expr() {
    int line = currentToken.line;  // linha do início da expressão

    // A atribuição (ID <- expr) possui a menor precedência em COOL e exige lookahead 2
    if (currentToken.type == TokenType::OBJECTID &&
        peekNext().type == TokenType::ATRIBUIR) {
        std::string varName = currentToken.lexeme;
        advance();  // Consome OBJECTID
        advance();  // Consome '<-'
        auto assignedValue = expr();       // Processa a expressão atribuída
        auto assignNode = std::make_shared<AssignASTNode>(varName, assignedValue);
        assignNode->line = line;
        return assignNode;
    }

    // Caso não seja atribuição, inicia a subida pela hierarquia de precedência
    return expr_not();
}

std::shared_ptr<ExprNode> Parser::expr_not() {
    int line = currentToken.line;
    if (isKeyword(currentToken, "not")) {
        std::string op = currentToken.lexeme;
        advance();  // Consome "not"
        auto operand = expr_not();         // Suporta 'not' aninhado
        auto unaryNode = std::make_shared<UnaryOpASTNode>(op, operand);
        unaryNode->line = line;
        return unaryNode;
    } else {
        return expr_cmp();
    }
}

std::shared_ptr<ExprNode> Parser::expr_cmp() {
    int line = currentToken.line;
    auto left = expr_add();
    // Comparação não é associativa em COOL (apenas uma por nível de expressão)
    if (currentToken.type == TokenType::MENORQ || currentToken.type == TokenType::SETAE ||
        currentToken.type == TokenType::IGUAL) {
        std::string op = currentToken.lexeme;
        advance();  // Consome op de comparação
        auto right = expr_add();
        auto binaryNode = std::make_shared<BinaryOpASTNode>(op, left, right);
        binaryNode->line = line;
        return binaryNode;
    }
    return left;
}

std::shared_ptr<ExprNode> Parser::expr_add() {
    int line = currentToken.line;
    auto left = expr_mult();
    // Laço while garante associação à esquerda (ex: a + b - c)
    while (currentToken.type == TokenType::MAIS || currentToken.type == TokenType::MENOS) {
        std::string op = currentToken.lexeme;
        advance();  // Consome '+' ou '-'
        auto right = expr_mult();
        left = std::make_shared<BinaryOpASTNode>(op, left, right);
        left->line = line;
    }
    return left;
}

std::shared_ptr<ExprNode> Parser::expr_mult() {
    int line = currentToken.line;
    auto left = expr_unary();
    // Laço while garante associação à esquerda (ex: a * b / c)
    while (currentToken.type == TokenType::ASTERISCO || currentToken.type == TokenType::BARRA) {
        std::string op = currentToken.lexeme;
        advance();  // Consome '*' ou '/'
        auto right = expr_unary();
        left = std::make_shared<BinaryOpASTNode>(op, left, right);
        left->line = line;
    }
    return left;
}

std::shared_ptr<ExprNode> Parser::expr_unary() {
    int line = currentToken.line;
    if (currentToken.type == TokenType::TIL) {
        std::string op = currentToken.lexeme;
        advance();  // Consome '~'
        auto operand = expr_unary();
        auto unaryNode = std::make_shared<UnaryOpASTNode>(op, operand);
        unaryNode->line = line;
        return unaryNode;
    } else if (isKeyword(currentToken, "isvoid")) {
        std::string op = currentToken.lexeme;
        advance();  // Consome "isvoid"
        auto operand = expr_unary();
        auto unaryNode = std::make_shared<UnaryOpASTNode>(op, operand);
        unaryNode->line = line;
        return unaryNode;
    } else {
        // 'new' mora em expr_primary: assim o dispatch de maior precedência
        // também se aplica a ele, como em (new Foo).bar().
        return expr_dispatch();
    }
}

std::shared_ptr<ExprNode> Parser::expr_dispatch() {
    int line = currentToken.line;  // linha do início do receptor
    auto leftExpr = expr_primary();

    // Permite chamadas de métodos encadeadas: obj.metodo1().metodo2()
    while (currentToken.type == TokenType::ARROBA || currentToken.type == TokenType::PONTO) {
        std::string staticType = "";
        if (currentToken.type == TokenType::ARROBA) {
            advance();  // Consome '@'
            if (currentToken.type == TokenType::TYPEID) {
                staticType = currentToken.lexeme;
                advance();  // Consome o tipo do dispatch estático
                if (currentToken.type != TokenType::PONTO) {
                    std::cerr << "Erro sintático na linha " << currentToken.line
                              << ": Esperado '.' após '@TIPO'\n";
                    errorFlag = true;
                    return nullptr;
                }
            } else {
                std::cerr << "Erro sintático na linha " << currentToken.line
                          << ": Esperado identificador de tipo após '@'\n";
                errorFlag = true;
                return nullptr;
            }
        }

        if (currentToken.type == TokenType::PONTO) {
            advance();  // Consome '.'
            if (currentToken.type == TokenType::OBJECTID) {
                std::string methodName = currentToken.lexeme;
                advance();  // Consome nome do método
                if (currentToken.type == TokenType::ABREPARENTESES) {
                    advance();  // Consome '('
                    std::vector<std::shared_ptr<ExprNode>> args;
                    if (currentToken.type != TokenType::FECHAPARENTESES) {
                        args.push_back(expr());
                        while (currentToken.type == TokenType::VIRGULA) {
                            advance();  // Consome ','
                            args.push_back(expr());
                        }
                    }

                    if (currentToken.type == TokenType::FECHAPARENTESES) {
                        advance();  // Consome ')'
                        leftExpr = std::make_shared<DispatchASTNode>(leftExpr, staticType,
                                                                     methodName, args);
                        leftExpr->line = line;
                    } else {
                        std::cerr << "Erro sintático na linha " << currentToken.line
                                  << ": Esperado ')' após lista de argumentos\n";
                        errorFlag = true;
                        return nullptr;
                    }
                } else {
                    std::cerr << "Erro sintático na linha " << currentToken.line
                              << ": Esperado '(' após nome do método\n";
                    errorFlag = true;
                    return nullptr;
                }
            } else {
                std::cerr << "Erro sintático na linha " << currentToken.line
                          << ": Esperado identificador de método após '.'\n";
                errorFlag = true;
                return nullptr;
            }
        }
    }
    return leftExpr;
}

std::shared_ptr<ExprNode> Parser::expr_primary() {
    int line = currentToken.line;  // Guarda a linha do token atual para definir nos nós AST

    // 'new' é um primário (e não um operador unário): o dispatch '.', que tem
    // precedência máxima, pode ser aplicado ao objeto criado.
    if (isKeyword(currentToken, "new")) {
        advance();  // Consome "new"
        if (!isTypeToken()) {
            std::cerr << "Erro sintático na linha " << currentToken.line
                      << ": Esperado identificador de tipo após 'new'\n";
            errorFlag = true;
            return nullptr;
        }
        std::string typeName = currentToken.lexeme;
        advance();  // Consome o TYPEID/SELF_TYPE
        auto newNode = std::make_shared<NewASTNode>(typeName);
        newNode->line = line;
        return newNode;
    }

    if (isKeyword(currentToken, "if")) {
        advance();  // Consome 'if'
        auto cond = expr();
        if (isKeyword(currentToken, "then")) {
            advance();  // Consome 'then'
            auto thenExpr = expr();
            if (isKeyword(currentToken, "else")) {
                advance();  // Consome 'else'
                auto elseExpr = expr();
                if (isKeyword(currentToken, "fi")) {
                    advance();  // Consome 'fi'
                    auto ifNode = std::make_shared<IfASTNode>(cond, thenExpr, elseExpr);
                    ifNode->line = line;
                    return ifNode;
                } else {
                    std::cerr << "Erro sintático na linha " << currentToken.line << ": Esperado 'fi'\n";
                    errorFlag = true;
                    return nullptr;
                }
            } else {
                std::cerr << "Erro sintático na linha " << currentToken.line << ": Esperado 'else'\n";
                errorFlag = true;
                return nullptr;
            }
        } else {
            std::cerr << "Erro sintático na linha " << currentToken.line << ": Esperado 'then'\n";
            errorFlag = true;
            return nullptr;
        }
    } else if (isKeyword(currentToken, "while")) {
        advance();  // Consome 'while'
        auto cond = expr();
        if (isKeyword(currentToken, "loop")) {
            advance();  // Consome 'loop'
            auto body = expr();
            if (isKeyword(currentToken, "pool")) {
                advance();  // Consome 'pool'
                auto whileNode = std::make_shared<WhileASTNode>(cond, body);
                whileNode->line = line;
                return whileNode;
            } else {
                std::cerr << "Erro sintático na linha " << currentToken.line << ": Esperado 'pool'\n";
                errorFlag = true;
                return nullptr;
            }
        } else {
            std::cerr << "Erro sintático na linha " << currentToken.line << ": Esperado 'loop'\n";
            errorFlag = true;
            return nullptr;
        }
    } else if (isKeyword(currentToken, "let")) {
        advance();  // Consome 'let'
        std::vector<LetBinding> bindings;

        auto parse_binding = [this, &bindings]() {
            if (currentToken.type == TokenType::OBJECTID) {
                std::string varName = currentToken.lexeme;
                advance();
                if (currentToken.type == TokenType::DOISPONTOS) {
                    advance();
                    if (isTypeToken()) {
                        std::string varType = currentToken.lexeme;
                        advance();
                        std::shared_ptr<ExprNode> initExpr = nullptr;
                        if (currentToken.type == TokenType::ATRIBUIR) {
                            advance();
                            initExpr = expr();
                        }
                        bindings.push_back({varName, varType, initExpr});
                    } else {
                        std::cerr << "Erro sintático na linha " << currentToken.line
                                  << ": Esperado TYPEID em 'let'\n";
                        errorFlag = true;
                        return;
                    }
                } else {
                    std::cerr << "Erro sintático na linha " << currentToken.line
                              << ": Esperado ':' em 'let'\n";
                    errorFlag = true;
                    return;
                }
            } else {
                std::cerr << "Erro sintático na linha " << currentToken.line
                          << ": Esperado identificador de variável em 'let'\n";
                errorFlag = true;
            }
        };

        parse_binding();
        while (currentToken.type == TokenType::VIRGULA) {
            advance();  // Consome ','
            parse_binding();
        }

        if (isKeyword(currentToken, "in")) {
            advance();  // Consome 'in'
            auto body = expr();
            auto letNode = std::make_shared<LetASTNode>(bindings, body);
            letNode->line = line;
            return letNode;
        } else {
            std::cerr << "Erro sintático na linha " << currentToken.line << ": Esperado 'in' em 'let'\n";
            errorFlag = true;
            return nullptr;
        }
    } else if (currentToken.type == TokenType::ABRECHAVES) {
        advance();  // Consome '{'
        std::vector<std::shared_ptr<ExprNode>> expressions;
        while (currentToken.type != TokenType::FECHACHAVES &&
               currentToken.type != TokenType::END_OF_FILE) {
            auto expression = expr();
            if (expression) {
                expressions.push_back(expression);
            }

            if (currentToken.type == TokenType::PONTOEVIRGULA) {
                advance();  // Consome ';'
            } else {
                std::cerr << "Erro sintático na linha " << currentToken.line
                          << ": Esperado ';' no bloco\n";
                errorFlag = true;

                // Recuperação: pula o resto da expressão inválida até o próximo
                // ';' ou '}' em vez de abandonar o bloco inteiro.
                while (currentToken.type != TokenType::PONTOEVIRGULA &&
                       currentToken.type != TokenType::FECHACHAVES &&
                       currentToken.type != TokenType::END_OF_FILE) {
                    advance();
                }
                if (currentToken.type == TokenType::PONTOEVIRGULA) {
                    advance();
                }
            }
        }
        if (currentToken.type == TokenType::FECHACHAVES) {
            if (expressions.empty()) {
                // §11: o bloco exige pelo menos uma expressão: { expr; }
                if (!errorFlag) {
                    std::cerr << "Erro sintático na linha " << line
                              << ": Bloco vazio: esperado pelo menos uma expressão\n";
                    errorFlag = true;
                }
                return nullptr;
            }
            advance();  // Consome '}'
            auto blockNode = std::make_shared<BlockASTNode>(expressions);
            blockNode->line = line;
            return blockNode;
        }
        std::cerr << "Erro sintático na linha " << currentToken.line
                  << ": Esperado '}' para fechar o bloco\n";
        errorFlag = true;
        return nullptr;
    } else if (isKeyword(currentToken, "case")) {
        advance();  // Consome 'case'
        auto caseExpr = expr();

        if (isKeyword(currentToken, "of")) {
            advance();  // Consome 'of'
            std::vector<CaseBranch> branches;

            while (currentToken.type == TokenType::OBJECTID) {
                std::string varName = currentToken.lexeme;
                advance();  // Consome ID

                if (currentToken.type == TokenType::DOISPONTOS) {
                    advance();  // Consome ':'
                    if (isTypeToken()) {
                        std::string varType = currentToken.lexeme;
                        advance();  // Consome tipo
                        if (currentToken.type == TokenType::SETAD) {
                            advance();  // Consome '=>'
                            auto branchExpr = expr();         // Processa ramo do case
                            if (currentToken.type == TokenType::PONTOEVIRGULA) {
                                advance();  // Consome ';'
                                branches.push_back({varName, varType, branchExpr});
                            } else {
                                std::cerr << "Erro sintático na linha " << currentToken.line
                                          << ": Esperado ';' no caso\n";
                                errorFlag = true;
                                return nullptr;
                            }
                        } else {
                            std::cerr << "Erro sintático na linha " << currentToken.line
                                      << ": Esperado '=>' no caso\n";
                            errorFlag = true;
                            return nullptr;
                        }
                    } else {
                        std::cerr << "Erro sintático na linha " << currentToken.line
                                  << ": Esperado TYPEID no caso\n";
                        errorFlag = true;
                        return nullptr;
                    }
                } else {
                    std::cerr << "Erro sintático na linha " << currentToken.line
                              << ": Esperado ':' no caso\n";
                    errorFlag = true;
                    return nullptr;
                }
            }
            // Validação da gramática do COOL: ao menos um ramo é obrigatório
            if (branches.empty()) {
                std::cerr << "Erro sintático na linha " << currentToken.line
                          << ": A instrução 'case' exige pelo menos um ramo\n";
                errorFlag = true;
                return nullptr;
            }
            if (isKeyword(currentToken, "esac")) {
                advance();  // Consome 'esac'
                auto caseNode = std::make_shared<CaseASTNode>(caseExpr, branches);
                caseNode->line = line;
                return caseNode;
            } else {
                std::cerr << "Erro sintático na linha " << currentToken.line << ": Esperado 'esac'\n";
                errorFlag = true;
                return nullptr;
            }
        } else {
            std::cerr << "Erro sintático na linha " << currentToken.line << ": Esperado 'of' no 'case'\n";
            errorFlag = true;
            return nullptr;
        }
    } else if (currentToken.type == TokenType::OBJECTID) {
        std::string name = currentToken.lexeme;
        advance();  // Consome o ID do objeto

        // Trata chamada de método local: id(...)
        if (currentToken.type == TokenType::ABREPARENTESES) {
            advance();  // Consome '('
            std::vector<std::shared_ptr<ExprNode>> args;
            if (currentToken.type != TokenType::FECHAPARENTESES) {
                args.push_back(expr());
                while (currentToken.type == TokenType::VIRGULA) {
                    advance();  // Consome ','
                    args.push_back(expr());
                }
            }
            if (currentToken.type == TokenType::FECHAPARENTESES) {
                advance();  // Consome ')'
                auto dispatchNode = std::make_shared<LocalCallASTNode>(name, args);
                dispatchNode->line = line;
                return dispatchNode;
            } else {
                std::cerr << "Erro sintático na linha " << currentToken.line << ": Esperado ')'\n";
                errorFlag = true;
                return nullptr;
            }
        }
        // Se não for '(', trata-se de uma variável simples. O token já foi consumido.
        auto objectNode = std::make_shared<ObjectASTNode>(name);
        objectNode->line = line;
        return objectNode;
    } else if (currentToken.type == TokenType::INT) {
        int val = 0;
        try {
            val = std::stoi(currentToken.lexeme);
        } catch (const std::out_of_range&) {
            std::cerr << "Erro sintático na linha " << line
                      << ": Constante inteira fora do limite suportado (overflow/underflow): '"
                      << currentToken.lexeme << "'\n";
            errorFlag = true;
            return nullptr;
        } catch (const std::invalid_argument&) {
            std::cerr << "Erro sintático na linha " << line << ": Literal inteiro inválido: '"
                      << currentToken.lexeme << "'\n";
            errorFlag = true;
            return nullptr;
        }
        advance();
        auto intNode = std::make_shared<IntASTNode>(val);
        intNode->line = line;
        return intNode;
    } else if (currentToken.type == TokenType::STRING) {
        std::string str = currentToken.lexeme;
        advance();
        auto stringNode = std::make_shared<StringASTNode>(str);
        stringNode->line = line;
        return stringNode;
    } else if (currentToken.type == TokenType::BOOLEAN ||
               currentToken.type == TokenType::SELF) {
        std::string lex = currentToken.lexeme;
        advance();

        // true/false são case-insensitive a partir da segunda letra (§10.4):
        // tRuE e fAlSe também são constantes booleanas válidas.
        std::string lexLower = lex;
        for (char& ch : lexLower) {
            ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        }

        if (lexLower == "true") {
            auto boolNode = std::make_shared<BoolASTNode>(true);
            boolNode->line = line;
            return boolNode;
        }
        if (lexLower == "false") {
            auto boolNode = std::make_shared<BoolASTNode>(false);
            boolNode->line = line;
            return boolNode;
        }
        auto selfNode = std::make_shared<ObjectASTNode>(lex);
        selfNode->line = line;
        return selfNode;  // 'self'
    } else if (currentToken.type == TokenType::ABREPARENTESES) {
        advance();  // Consome '('
        auto innerExpr = expr();
        if (innerExpr) innerExpr->line = line;
        if (currentToken.type == TokenType::FECHAPARENTESES) {
            advance();  // Consome ')'
            return innerExpr;
        } else {
            std::cerr << "Erro sintático na linha " << currentToken.line << ": Esperado ')'\n";
            errorFlag = true;
            return nullptr;
        }
    } else {
        std::cerr << "Erro sintático na linha " << currentToken.line << ": Expressão inválida\n";
        errorFlag = true;
        return nullptr;
    }
}

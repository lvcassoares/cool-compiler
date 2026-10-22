#include "SemanticAnalyzer.hpp"

#include <algorithm>
#include <iostream>
#include <vector>

namespace {

// Tipo-sentinela usado para suprimir erros em cascata depois que a causa raiz
// já foi reportada. Um nó com este tipo não tenta buscar métodos/atributos.
constexpr const char* ERROR_TYPE = "<error>";

bool isErrorType(const std::string& type) { return type == ERROR_TYPE; }

// Itera mapas de classes em ordem alfabética. A ordem de unordered_map varia
// entre builds/execuções; para comparar saída com gabarito, precisamos de
// mensagens determinísticas.
template <typename Map>
std::vector<std::string> sortedKeys(const Map& map) {
    std::vector<std::string> keys;
    keys.reserve(map.size());
    for (const auto& entry : map) {
        keys.push_back(entry.first);
    }
    std::sort(keys.begin(), keys.end());
    return keys;
}

}  // namespace

// ============================================================================
// SemanticAnalyzer.cpp
// ----------------------------------------------------------------------------
// Porte fiel do analisador semântico da fase 3 (lexsinsemantico.cpp).
// A única mudança estrutural: a entrada é a AST, não o texto-fonte.
// ============================================================================

// ------------------------------- SymbolTable -------------------------------

void SymbolTable::enterScope() { scopes.push_back({}); }

void SymbolTable::exitScope() {
    if (!scopes.empty()) {
        scopes.pop_back();
    }
}

bool SymbolTable::addSymbol(const std::string& name, const std::string& type) {
    if (scopes.empty()) return false;
    if (scopes.back().find(name) != scopes.back().end()) {
        return false;  // Redefinição no mesmo escopo
    }
    scopes.back()[name] = type;
    return true;
}

std::string SymbolTable::lookup(const std::string& name) const {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) {
            return found->second;
        }
    }
    return "";  // Não encontrado
}

// ---------------------------- SemanticAnalyzer -----------------------------

void SemanticAnalyzer::logError(int line, const std::string& msg) {
    // Formato único: "Erro semântico na linha N: mensagem", sem ponto final.
    std::string message = msg;
    if (!message.empty() && message.back() == '.') {
        message.pop_back();
    }
    std::cerr << "Erro semântico na linha " << line << ": " << message << "\n";
    errorFlag = true;
    ++errorCount;
}

// Passagem 1: instala as classes básicas do COOL
void SemanticAnalyzer::installBasicClasses() {
    // 1. Class Object (Sem pai)
    auto objectClass = std::make_shared<ClassASTNode>("Object", "");
    // Métodos nativos de Object: abort() : Object, type_name() : String, copy() : SELF_TYPE
    objectClass->features.push_back(std::make_shared<FeatureASTNode>(
        "abort", std::vector<std::shared_ptr<FormalASTNode>>{}, "Object", nullptr));
    objectClass->features.push_back(std::make_shared<FeatureASTNode>(
        "type_name", std::vector<std::shared_ptr<FormalASTNode>>{}, "String", nullptr));
    objectClass->features.push_back(std::make_shared<FeatureASTNode>(
        "copy", std::vector<std::shared_ptr<FormalASTNode>>{}, "SELF_TYPE", nullptr));
    classTable["Object"] = objectClass;

    // 2. Class IO (inherits Object)
    auto ioClass = std::make_shared<ClassASTNode>("IO", "Object");
    // out_string(x : String) : SELF_TYPE
    ioClass->features.push_back(std::make_shared<FeatureASTNode>(
        "out_string",
        std::vector<std::shared_ptr<FormalASTNode>>{std::make_shared<FormalASTNode>("x", "String")},
        "SELF_TYPE", nullptr));
    // out_int(x : Int) : SELF_TYPE
    ioClass->features.push_back(std::make_shared<FeatureASTNode>(
        "out_int",
        std::vector<std::shared_ptr<FormalASTNode>>{std::make_shared<FormalASTNode>("x", "Int")},
        "SELF_TYPE", nullptr));
    // in_string() : String
    ioClass->features.push_back(std::make_shared<FeatureASTNode>(
        "in_string", std::vector<std::shared_ptr<FormalASTNode>>{}, "String", nullptr));
    // in_int() : Int
    ioClass->features.push_back(std::make_shared<FeatureASTNode>(
        "in_int", std::vector<std::shared_ptr<FormalASTNode>>{}, "Int", nullptr));
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
    stringClass->features.push_back(std::make_shared<FeatureASTNode>(
        "length", std::vector<std::shared_ptr<FormalASTNode>>{}, "Int", nullptr));
    // concat(s : String) : String
    stringClass->features.push_back(std::make_shared<FeatureASTNode>(
        "concat",
        std::vector<std::shared_ptr<FormalASTNode>>{std::make_shared<FormalASTNode>("s", "String")},
        "String", nullptr));
    // substr(i : Int, l : Int) : String
    stringClass->features.push_back(std::make_shared<FeatureASTNode>(
        "substr",
        std::vector<std::shared_ptr<FormalASTNode>>{std::make_shared<FormalASTNode>("i", "Int"),
                                                    std::make_shared<FormalASTNode>("l", "Int")},
        "String", nullptr));
    classTable["String"] = stringClass;
}

// Passagem 1: Validação do Grafo de Herança
void SemanticAnalyzer::buildInheritanceGraph() {
    // 1. Instala as classes básicas do COOL
    installBasicClasses();

    // 1. Registra as classes do programa
    for (const auto& cls : astRoot->classes) {
        if (classTable.count(cls->name) > 0) {
            logError(cls->line,
                     "Classe " + cls->name + " já foi definida(ou é uma classe básica reservada).");
        } else {
            classTable[cls->name] = cls;
        }
    }

    // 2. Verifica se os pais existem e proíbe herança de tipos primários
    for (const std::string& name : sortedKeys(classTable)) {
        const auto& cls = classTable.at(name);
        if (name == "Object") {
            continue;
        } else if (cls->parent == "Int" || cls->parent == "String" || cls->parent == "Bool") {
            logError(cls->line, "Classe " + name + " não pode herdar de " + cls->parent + ".");
        } else if (cls->parent != "Object" && classTable.count(cls->parent) == 0) {
            logError(cls->line, "Classe " + name + " herda de classe não declarada " + cls->parent +
                                    ".");
        }
    }

    // Se já houver erros de herança inexistente ou redefinição, evita procurar ciclos falsos
    if (errorFlag) return;

    // 3. Detecção de Ciclos de Herança
    for (const std::string& name : sortedKeys(classTable)) {
        if (name == "Object") continue;
        const auto& cls = classTable.at(name);

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
            auto currentIt = classTable.find(current);
            if (currentIt != classTable.end()) {
                current = currentIt->second->parent;
            } else {
                // Pai não pertence às classes do usuário (ex: Object ou erro tratado)
                break;
            }
        }
    }
}

// Passagem 2: Coleta de Atributos e Métodos

// Busca se um atributo já foi definido em alguma classe ancestral
bool SemanticAnalyzer::isAttributeInAncestors(const std::string& className,
                                              const std::string& attrName) {
    // Nunca use classTable[className]: operator[] INSERE uma entrada nullptr
    // quando a classe não existe, e o ->parent seguinte derruba o compilador.
    auto classIt = classTable.find(className);
    if (classIt == classTable.end()) return false;

    std::string current = classIt->second->parent;
    while (!current.empty()) {
        auto currentIt = classTable.find(current);
        if (currentIt == classTable.end()) break;

        auto attrsIt = classAttributes.find(current);
        if (attrsIt != classAttributes.end() && attrsIt->second.count(attrName) > 0) {
            return true;
        }
        current = currentIt->second->parent;
    }
    return false;
}

// Busca se um método existe em alguma classe ancestral para validar a assinatura do override
SemanticAnalyzer::MethodSymbol* SemanticAnalyzer::findMethodInAncestors(
    const std::string& className, const std::string& methodName) {
    auto classIt = classTable.find(className);
    if (classIt == classTable.end()) return nullptr;

    std::string current = classIt->second->parent;
    while (!current.empty()) {
        auto currentIt = classTable.find(current);
        if (currentIt == classTable.end()) break;

        auto methodsIt = classMethods.find(current);
        if (methodsIt != classMethods.end()) {
            auto methodIt = methodsIt->second.find(methodName);
            if (methodIt != methodsIt->second.end()) {
                return &methodIt->second;
            }
        }
        current = currentIt->second->parent;
    }
    return nullptr;
}

void SemanticAnalyzer::collectSymbolsForClass(const std::shared_ptr<ClassASTNode>& cls) {
    auto& attrs = classAttributes[cls->name];
    auto& methods = classMethods[cls->name];

    for (const auto& feat : cls->features) {
        // -----------------------------------------------------------------
        // 1. PROCESSAMENTO DE ATRIBUTOS
        // -----------------------------------------------------------------
        if (!feat->isMethod) {
            // Regra: Atributo não pode se chamar 'self'
            if (feat->name == "self") {
                logError(cls->line, "'self' não pode ser usado como nome de atributo na classe '" +
                                        cls->name + "'.");
                continue;
            }

            // Regra: Duplicata na mesma classe
            if (attrs.count(feat->name) > 0) {
                logError(cls->line, "Atributo '" + feat->name + "' redefinido na classe '" +
                                        cls->name + "'.");
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
                logError(cls->line, "Método '" + feat->name + "' redefinido na classe '" +
                                        cls->name + "'.");
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
                    logError(cls->line, "Parâmetro formal '" + formal->name +
                                            "' duplicado no método '" + feat->name +
                                            "' da classe '" + cls->name + "'.");
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
                    logError(cls->line, "Sobrescrita do método '" + feat->name + "' na classe '" +
                                            cls->name + "' altera o tipo de retorno de '" +
                                            inheritedMethod->returnType + "' para '" +
                                            newMethod.returnType + "'.");
                }

                // Checa quantidade de parâmetros
                if (inheritedMethod->formals.size() != newMethod.formals.size()) {
                    logError(cls->line, "Sobrescrita do método '" + feat->name + "' na classe '" +
                                            cls->name + "' altera o número de parâmetros formais.");
                } else {
                    // Checa os tipos de cada parâmetro
                    for (size_t i = 0; i < newMethod.formals.size(); ++i) {
                        if (inheritedMethod->formals[i].second != newMethod.formals[i].second) {
                            logError(cls->line, "Sobrescrita do método '" + feat->name +
                                                    "' na classe '" + cls->name +
                                                    "' altera o tipo do parâmetro '" +
                                                    newMethod.formals[i].first + "'.");
                        }
                    }
                }
            }

            methods[feat->name] = newMethod;
        }
    }
}

void SemanticAnalyzer::collectAllSymbols() {
    // 1. Constrói a lista de adjacência do grafo invertido (Pai -> Filhos)
    std::unordered_map<std::string, std::vector<std::shared_ptr<ClassASTNode>>> childrenMap;

    for (const std::string& name : sortedKeys(classTable)) {
        const auto& cls = classTable.at(name);
        if (name != "Object") {
            childrenMap[cls->parent].push_back(cls);
        }
    }

    // Ordena as filhas para que a BFS seja determinística.
    for (auto& [parent, children] : childrenMap) {
        std::sort(children.begin(), children.end(),
                  [](const std::shared_ptr<ClassASTNode>& a,
                     const std::shared_ptr<ClassASTNode>& b) { return a->name < b->name; });
    }

    // 2. Fila para travessia em largura (BFS) a partir da raiz "Object"
    std::queue<std::string> processQueue;
    processQueue.push("Object");

    // 3. Processa em ordem topológica garantida (pais sempre antes das filhas)
    while (!processQueue.empty()) {
        std::string currentClassName = processQueue.front();
        processQueue.pop();

        auto clsIt = classTable.find(currentClassName);
        if (clsIt == classTable.end()) continue;
        auto clsNode = clsIt->second;

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

// Passagem 3: Checagem de tipos e escopos

// Função auxiliar para buscar atributo em toda a cadeia de herança
std::string SemanticAnalyzer::lookupAttributeInHierarchy(const std::string& className,
                                                         const std::string& attrName) {
    std::string current = className;
    while (!current.empty()) {
        auto classIt = classTable.find(current);
        if (classIt == classTable.end()) break;

        auto attrsIt = classAttributes.find(current);
        if (attrsIt != classAttributes.end()) {
            auto attrIt = attrsIt->second.find(attrName);
            if (attrIt != attrsIt->second.end()) {
                return attrIt->second.type;
            }
        }
        current = classIt->second->parent;
    }
    return "";
}

// Carrega todos os atributos herdados e próprios da classe no escopo atual
void SemanticAnalyzer::populateClassScope(const std::string& className) {
    std::vector<std::string> hierarchy;
    std::string current = className;

    // Coleta os ancestrais até Object
    while (!current.empty()) {
        auto classIt = classTable.find(current);
        if (classIt == classTable.end()) break;
        hierarchy.push_back(current);
        current = classIt->second->parent;
    }

    // Injeta os atributos do ancestral mais distante (Object) até a classe atual
    for (auto it = hierarchy.rbegin(); it != hierarchy.rend(); ++it) {
        auto attrsIt = classAttributes.find(*it);
        if (attrsIt == classAttributes.end()) continue;
        for (const auto& [attrName, attrSym] : attrsIt->second) {
            symbolTable.addSymbol(attrName, attrSym.type);
        }
    }

    // Adiciona a pseudo-variável 'self' apontando para o tipo da própria classe
    symbolTable.addSymbol("self", "SELF_TYPE");
}

// Resolve o tipo real se for SELF_TYPE
std::string SemanticAnalyzer::resolveType(const std::string& type,
                                          const std::string& currentClass) const {
    return (type == "SELF_TYPE") ? currentClass : type;
}

// Verifica se child <= parent (Conformidade de Tipos, §3.2 e §4.1)
bool SemanticAnalyzer::isSubtype(std::string child, std::string parent,
                                 const std::string& currentClass) {
    if (child == parent) return true;  // inclui SELF_TYPE <= SELF_TYPE

    // Depois de um erro já reportado, não vale a pena gerar erros em cascata.
    if (isErrorType(child) || isErrorType(parent)) return true;

    // Regra especial: T <= SELF_TYPE só se T for SELF_TYPE (já tratado acima).
    if (parent == "SELF_TYPE") return false;

    // SELF_TYPE_C <= P  se  C <= P
    if (child == "SELF_TYPE") child = currentClass;

    // Tipos desconhecidos já foram diagnosticados na declaração/uso; tratar
    // como compatíveis evita uma enxurrada de mensagens secundárias.
    if (classTable.find(child) == classTable.end()) return true;
    if (classTable.find(parent) == classTable.end()) return true;

    std::unordered_set<std::string> visited;
    std::string curr = child;
    while (!curr.empty()) {
        if (curr == parent) return true;
        if (!visited.insert(curr).second) break;  // proteção contra ciclos

        auto classIt = classTable.find(curr);
        if (classIt == classTable.end()) break;
        curr = classIt->second->parent;
    }
    return false;
}

// Calcula o Menor Ancestral Comum (Least Upper Bound / join, §7.5)
std::string SemanticAnalyzer::lub(std::string type1, std::string type2,
                                  const std::string& currentClass) {
    if (isErrorType(type1) || isErrorType(type2)) return ERROR_TYPE;

    // SELF_TYPE_D ⊔ SELF_TYPE_D = SELF_TYPE_D. Não resolva antes de comparar!
    if (type1 == "SELF_TYPE" && type2 == "SELF_TYPE") return "SELF_TYPE";

    // SELF_TYPE_D ⊔ A = D ⊔ A  (e simétrico)
    if (type1 == "SELF_TYPE") type1 = currentClass;
    if (type2 == "SELF_TYPE") type2 = currentClass;

    if (type1 == type2) return type1;

    if (classTable.find(type1) == classTable.end() ||
        classTable.find(type2) == classTable.end()) {
        return "Object";
    }

    // Coleta o caminho de type1 até a raiz Object
    std::vector<std::string> path1;
    std::unordered_set<std::string> visited;
    std::string curr = type1;
    while (!curr.empty() && visited.insert(curr).second) {
        path1.push_back(curr);
        auto classIt = classTable.find(curr);
        if (classIt == classTable.end()) break;
        curr = classIt->second->parent;
    }

    // Sobe por type2 até encontrar o primeiro ancestral em comum
    curr = type2;
    visited.clear();
    while (!curr.empty() && visited.insert(curr).second) {
        for (const auto& ancestor : path1) {
            if (curr == ancestor) return curr;
        }
        auto classIt = classTable.find(curr);
        if (classIt == classTable.end()) break;
        curr = classIt->second->parent;
    }
    return "Object";
}

// Função recursiva principal que percorre a AST de expressões e retorna seu tipo inferido
std::string SemanticAnalyzer::checkExprType(const std::shared_ptr<ExprNode>& expr,
                                            const std::string& currentClass) {
    if (!expr) return ERROR_TYPE;

    // 1. TERMINAIS E LITERAIS
    if (std::dynamic_pointer_cast<IntASTNode>(expr)) return "Int";
    if (std::dynamic_pointer_cast<StringASTNode>(expr)) return "String";
    if (std::dynamic_pointer_cast<BoolASTNode>(expr)) return "Bool";

    // Identificadores / Variáveis
    if (auto objNode = std::dynamic_pointer_cast<ObjectASTNode>(expr)) {
        if (objNode->name == "self") return "SELF_TYPE";

        std::string type = symbolTable.lookup(objNode->name);
        if (type.empty()) {
            logError(objNode->line,
                     "Identificador '" + objNode->name + "' não foi declarado no escopo.");
            return ERROR_TYPE;
        }
        return type;
    }

    // 2. ATRIBUIÇÃO (x <- expr)
    if (auto assignNode = std::dynamic_pointer_cast<AssignASTNode>(expr)) {
        if (assignNode->name == "self") {
            logError(assignNode->line, "Não é permitido atribuir valor a 'self'.");
            return ERROR_TYPE;
        }

        std::string varType = symbolTable.lookup(assignNode->name);
        if (varType.empty()) {
            logError(assignNode->line, "Variável '" + assignNode->name + "' não declarada.");
            return ERROR_TYPE;
        }

        std::string exprType = checkExprType(assignNode->value, currentClass);
        if (!isSubtype(exprType, varType, currentClass)) {
            logError(assignNode->line, "Tipo incompatível na atribuição de '" + assignNode->name +
                                           "': esperado '" + varType + "', mas obtido '" +
                                           exprType + "'.");
        }
        return exprType;
    }

    // 3. OPERAÇÕES ARITMÉTICAS E LÓGICAS (BinaryOpASTNode)
    if (auto binOp = std::dynamic_pointer_cast<BinaryOpASTNode>(expr)) {
        std::string leftType = checkExprType(binOp->left, currentClass);
        std::string rightType = checkExprType(binOp->right, currentClass);

        // Operadores Aritméticos (+, -, *, /)
        if (binOp->op == "+" || binOp->op == "-" || binOp->op == "*" || binOp->op == "/") {
            if (!isSubtype(leftType, "Int", currentClass) ||
                !isSubtype(rightType, "Int", currentClass)) {
                logError(binOp->line,
                         "Operador '" + binOp->op + "' exige operandos do tipo 'Int'.");
            }
            return "Int";
        }

        // Operadores de Comparação Relacional (<, <=)
        if (binOp->op == "<" || binOp->op == "<=") {
            if (!isSubtype(leftType, "Int", currentClass) ||
                !isSubtype(rightType, "Int", currentClass)) {
                logError(binOp->line,
                         "Operador '" + binOp->op + "' exige operandos do tipo 'Int'.");
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

        if (unOp->op == "~") {  // Negação de inteiro
            if (!isSubtype(operandType, "Int", currentClass)) {
                logError(unOp->line, "Operador '~' exige tipo 'Int'.");
            }
            return "Int";
        }
        if (unOp->op == "not") {  // Negação lógica
            if (!isSubtype(operandType, "Bool", currentClass)) {
                logError(unOp->line, "Operador 'not' exige tipo 'Bool'.");
            }
            return "Bool";
        }
        if (unOp->op == "isvoid") {  // Teste de ponteiro nulo
            return "Bool";
        }
    }

    // 5. INSTANCIAÇÃO (NewASTNode)
    if (auto newObj = std::dynamic_pointer_cast<NewASTNode>(expr)) {
        if (newObj->typeName != "SELF_TYPE" && classTable.count(newObj->typeName) == 0) {
            logError(newObj->line,
                     "Instanciação 'new " + newObj->typeName + "' de classe não declarada.");
            return ERROR_TYPE;
        }
        return newObj->typeName;
    }

    // 6. DISPATCH DINÂMICO E ESTÁTICO (DispatchASTNode)
    if (auto dispatch = std::dynamic_pointer_cast<DispatchASTNode>(expr)) {
        std::string targetType = checkExprType(dispatch->expr, currentClass);
        if (isErrorType(targetType)) return ERROR_TYPE;

        std::string realTargetType = resolveType(targetType, currentClass);

        // Se for Dispatch Estático (expr@TYPE.method(...))
        if (!dispatch->typeExpr.empty()) {
            if (classTable.find(dispatch->typeExpr) == classTable.end()) {
                logError(dispatch->line, "Classe '" + dispatch->typeExpr +
                                             "' não declarada no dispatch estático.");
                return ERROR_TYPE;
            }
            if (!isSubtype(targetType, dispatch->typeExpr, currentClass)) {
                logError(dispatch->line, "Expressão do dispatch estático não é do tipo '" +
                                             dispatch->typeExpr + "'.");
                return ERROR_TYPE;
            }
            realTargetType = dispatch->typeExpr;
        }

        // Tipo do receptor desconhecido: a causa já foi reportada na declaração.
        if (classTable.find(realTargetType) == classTable.end()) {
            return ERROR_TYPE;
        }

        // Procura pelo método na classe ou em seus ancestrais
        MethodSymbol* method = findMethodInAncestors(realTargetType, dispatch->name);
        if (!method) {
            auto methodsIt = classMethods.find(realTargetType);
            if (methodsIt != classMethods.end()) {
                auto methodIt = methodsIt->second.find(dispatch->name);
                if (methodIt != methodsIt->second.end()) method = &methodIt->second;
            }
        }

        if (!method) {
            logError(dispatch->line, "Método '" + dispatch->name + "' não encontrado na classe '" +
                                         realTargetType + "'.");
            return ERROR_TYPE;
        }

        // Valida argumentos do dispatch
        if (dispatch->args.size() != method->formals.size()) {
            logError(dispatch->line, "Número incorreto de argumentos para o método '" +
                                         dispatch->name + "'.");
        } else {
            for (size_t i = 0; i < dispatch->args.size(); ++i) {
                std::string argType = checkExprType(dispatch->args[i], currentClass);
                std::string paramType = method->formals[i].second;
                if (!isSubtype(argType, paramType, currentClass)) {
                    logError(dispatch->line, "Argumento " + std::to_string(i + 1) +
                                                 " do método '" + dispatch->name +
                                                 "': incompatibilidade de tipo.");
                }
            }
        }

        if (method->returnType == "SELF_TYPE") return targetType;
        if (classTable.find(method->returnType) == classTable.end()) return ERROR_TYPE;
        return method->returnType;
    }

    // Chamada de Método Local (self.method(...))
    if (auto localCall = std::dynamic_pointer_cast<LocalCallASTNode>(expr)) {
        MethodSymbol* method = findMethodInAncestors(currentClass, localCall->name);
        if (!method) {
            auto methodsIt = classMethods.find(currentClass);
            if (methodsIt != classMethods.end()) {
                auto methodIt = methodsIt->second.find(localCall->name);
                if (methodIt != methodsIt->second.end()) method = &methodIt->second;
            }
        }

        if (!method) {
            logError(localCall->line, "Método local '" + localCall->name +
                                          "' não declarado na classe '" + currentClass + "'.");
            return ERROR_TYPE;
        }

        if (localCall->args.size() != method->formals.size()) {
            logError(localCall->line, "Número incorreto de argumentos para o método local '" +
                                          localCall->name + "'.");
        } else {
            for (size_t i = 0; i < localCall->args.size(); ++i) {
                std::string argType = checkExprType(localCall->args[i], currentClass);
                std::string paramType = method->formals[i].second;
                if (!isSubtype(argType, paramType, currentClass)) {
                    logError(localCall->line, "Argumento " + std::to_string(i + 1) +
                                                  " da chamada local '" + localCall->name +
                                                  "': incompatibilidade de tipo.");
                }
            }
        }

        if (method->returnType == "SELF_TYPE") return "SELF_TYPE";
        if (classTable.find(method->returnType) == classTable.end()) return ERROR_TYPE;
        return method->returnType;
    }

    // 7. CONTROLE DE FLUXO: IF, WHILE, BLOCK
    if (auto ifNode = std::dynamic_pointer_cast<IfASTNode>(expr)) {
        std::string condType = checkExprType(ifNode->cond, currentClass);
        if (!isSubtype(condType, "Bool", currentClass)) {
            logError(ifNode->line, "Condição da instrução 'if' deve ser do tipo 'Bool'.");
        }

        std::string thenType = checkExprType(ifNode->thenExpr, currentClass);
        std::string elseType = checkExprType(ifNode->elseExpr, currentClass);

        // O retorno de um IF é o Menor Ancestral Comum das duas ramificações
        return lub(thenType, elseType, currentClass);
    }

    if (auto whileNode = std::dynamic_pointer_cast<WhileASTNode>(expr)) {
        std::string condType = checkExprType(whileNode->cond, currentClass);
        if (!isSubtype(condType, "Bool", currentClass)) {
            logError(whileNode->line, "Condição do laço 'while' deve ser do tipo 'Bool'.");
        }

        checkExprType(whileNode->body, currentClass);
        return "Object";  // Laços em COOL sempre retornam tipo Object
    }

    if (auto blockNode = std::dynamic_pointer_cast<BlockASTNode>(expr)) {
        std::string lastType = "Object";
        for (const auto& e : blockNode->expressions) {
            lastType = checkExprType(e, currentClass);
        }
        return lastType;  // Retorna o tipo da última expressão do bloco
    }

    // 8. DECLARAÇÃO DE VARIÁVEIS LOCAIS (LetASTNode)
    if (auto letNode = std::dynamic_pointer_cast<LetASTNode>(expr)) {
        int scopesEntered = 0;

        for (const auto& bind : letNode->bindings) {
            if (bind.name == "self") {
                logError(letNode->line, "'self' não pode ser declarado em um bloco 'let'.");
                continue;
            }

            // Valida se o tipo declarado para a variável existe no programa.
            // Se não existir, o símbolo recebe o tipo-sentinela <error> para
            // não derrubar o compilador nem gerar erros em cascata.
            std::string bindType = bind.type;
            if (bind.type != "SELF_TYPE" && classTable.count(bind.type) == 0) {
                logError(letNode->line, "Tipo não declarado '" + bind.type + "' no 'let' para a variável '" +
                                            bind.name + "'.");
                bindType = ERROR_TYPE;
            }

            // A expressão de inicialização (se houver) é avaliada NO ESCOPO ANTERIOR
            // (antes que esta nova variável seja adicionada à tabela)
            if (bind.init) {
                std::string initType = checkExprType(bind.init, currentClass);
                if (!isErrorType(bindType) && !isSubtype(initType, bindType, currentClass)) {
                    logError(letNode->line, "Inicialização incompatível no 'let' para a variável '" +
                                                bind.name + "': esperado '" + bindType +
                                                "', mas obtido '" + initType + "'.");
                }
            }

            // Cria um NOVO subescopo para este binding específico.
            // Isso permite que um binding posterior sombreie (shadow) este identificador.
            symbolTable.enterScope();
            symbolTable.addSymbol(bind.name, bindType);
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
            std::string branchType = branch.type;
            if (branch.type == "SELF_TYPE") {
                logError(caseNode->line, "O tipo 'SELF_TYPE' não pode ser usado em uma ramificação de 'case'.");
                branchType = ERROR_TYPE;
            }
            // Regra 3: Valida se o tipo declarado existe no programa
            else if (classTable.count(branch.type) == 0) {
                logError(caseNode->line, "Tipo não declarado '" + branch.type + "' na ramificação do 'case'.");
                branchType = ERROR_TYPE;
            }

            // Regra 4: Duplicidade de tipos nas ramificações
            if (branchTypes.count(branch.type) > 0) {
                logError(caseNode->line, "Tipo '" + branch.type + "' duplicado em ramificações do 'case'.");
            }
            branchTypes.insert(branch.type);

            // Cria o escopo isolado do ramo e adiciona a variável
            symbolTable.enterScope();
            symbolTable.addSymbol(branch.name, branchType);

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
void SemanticAnalyzer::checkTypes() {
    // Garante que a classe Main e o método main() existem.
    auto mainClassIt = classTable.find("Main");
    if (mainClassIt == classTable.end()) {
        logError(astRoot ? astRoot->line : 0, "Classe 'Main' não foi declarada no programa.");
        return;
    }

    int mainLine = mainClassIt->second->line;
    auto mainMethodsIt = classMethods.find("Main");
    if (mainMethodsIt == classMethods.end() ||
        mainMethodsIt->second.find("main") == mainMethodsIt->second.end()) {
        logError(mainLine, "Método 'main' não foi encontrado na classe 'Main'.");
        return;
    }

    // Regra do COOL (§9): main() deve ser declarado em Main (não herdado) e
    // não pode receber parâmetros formais.
    const MethodSymbol& mainMethod = mainMethodsIt->second.at("main");
    if (!mainMethod.formals.empty()) {
        logError(mainMethod.node ? mainMethod.node->line : mainLine,
                 "O método 'main' na classe 'Main' não deve receber parâmetros formais.");
    }

    // 2. Percorre todas as classes do usuário em ordem determinística.
    for (const std::string& className : sortedKeys(classTable)) {
        auto clsIt = classTable.find(className);
        if (clsIt == classTable.end()) continue;
        const auto& clsNode = clsIt->second;

        // Ignora as classes básicas estáticas na checagem interna
        if (className == "Object" || className == "IO" || className == "Int" ||
            className == "String" || className == "Bool") {
            continue;
        }

        if (traceEnabled) {
            std::cout << "\n[trace] Classe " << className << " inherits " << clsNode->parent
                      << std::endl;
        }

        // Para cada classe, cria o escopo base carregando seus atributos
        symbolTable.enterScope();
        populateClassScope(className);

        // 3. Valida cada feature (atributo ou método) da classe
        for (const auto& feat : clsNode->features) {
            int featureErrorCount = errorCount;

            // CASO A: CHECAGEM DE ATRIBUTO (x : Type [<- expr])
            if (!feat->isMethod) {
                // Valida se o tipo do atributo existe
                std::string attrType = feat->returnType;
                if (attrType != "SELF_TYPE" && classTable.count(attrType) == 0) {
                    logError(feat->line, "Atributo '" + feat->name + "' na classe '" + className +
                                             "' possui tipo não declarado '" + attrType + "'.");
                    attrType = ERROR_TYPE;
                }

                // Checa a expressão de inicialização do atributo (se houver: x : Int <- expr)
                std::string initType = "(sem inicializacao)";
                if (feat->initExpr) {
                    initType = checkExprType(feat->initExpr, className);

                    // O tipo da expressão de inicialização deve ser conforme ao tipo do atributo
                    if (!isErrorType(attrType) && !isSubtype(initType, attrType, className)) {
                        logError(feat->line, "Tipo de inicialização '" + initType +
                                                 "' incompatível com o tipo do atributo '" +
                                                 feat->name + "' (" + attrType +
                                                 ") na classe '" + className + "'.");
                    }
                }

                if (traceEnabled) {
                    std::cout << "  [atributo] " << feat->name << " : " << feat->returnType;
                    if (feat->initExpr) {
                        std::cout << " <- " << initType;
                    }
                    std::cout << (errorCount == featureErrorCount ? "  OK" : "  ERRO") << std::endl;
                }
            }
            // CASO B: CHECAGEM DE MÉTODOS (m(f1:T1, ...): T_ret { body })
            else {
                // Valida se o tipo de retorno do método existe
                std::string returnType = feat->returnType;
                if (returnType != "SELF_TYPE" && classTable.count(returnType) == 0) {
                    logError(feat->line, "Método '" + feat->name + "' na classe '" + className +
                                             "' possui tipo de retorno não declarado '" +
                                             returnType + "'.");
                    returnType = ERROR_TYPE;
                }

                // É um método: entra num novo escopo local para os parâmetros formais
                symbolTable.enterScope();

                for (const auto& formal : feat->formals) {
                    // Valida se o tipo do parâmetro formal existe
                    std::string formalType = formal->type;
                    if (formal->type == "SELF_TYPE") {
                        logError(feat->line, "Parâmetro '" + formal->name + "' no método '" +
                                                 feat->name +
                                                 "' não pode ser do tipo 'SELF_TYPE'.");
                        formalType = ERROR_TYPE;
                    } else if (classTable.count(formal->type) == 0) {
                        logError(feat->line, "Parâmetro '" + formal->name + "' no método '" +
                                                 feat->name + "' possui tipo não declarado '" +
                                                 formal->type + "'.");
                        formalType = ERROR_TYPE;
                    }

                    symbolTable.addSymbol(formal->name, formalType);
                }

                // Valida a expressão do corpo do método
                std::string bodyType = "(sem corpo)";
                if (feat->body) {
                    bodyType = checkExprType(feat->body, className);

                    // O tipo inferido do corpo deve ser conforme ao tipo de retorno declarado
                    if (!isErrorType(returnType) && !isSubtype(bodyType, returnType, className)) {
                        logError(feat->line, "Tipo do corpo do método '" + feat->name + "' ('" +
                                                 bodyType +
                                                 "') incompatível com o tipo de retorno declarado '" +
                                                 returnType + "' na classe '" + className + "'.");
                    }
                }

                if (traceEnabled) {
                    std::cout << "  [metodo] " << feat->name << "(";
                    for (size_t i = 0; i < feat->formals.size(); ++i) {
                        if (i > 0) std::cout << ", ";
                        std::cout << feat->formals[i]->name << " : " << feat->formals[i]->type;
                    }
                    std::cout << ") : " << feat->returnType << "  corpo: " << bodyType;
                    std::cout << (errorCount == featureErrorCount ? "  OK" : "  ERRO") << std::endl;
                }

                symbolTable.exitScope();  // Sai do escopo dos parâmetros formais
            }
        }

        symbolTable.exitScope();  // Sai do escopo dos atributos da classe
    }
}

bool SemanticAnalyzer::analyze() {
    if (!astRoot) {
        std::cerr << "Análise semântica abortada: AST ausente (a análise sintática falhou).\n";
        return false;
    }

    buildInheritanceGraph();
    if (errorFlag) return false;

    collectAllSymbols();
    if (errorFlag) return false;

    checkTypes();

    return !errorFlag;
}

void SemanticAnalyzer::printSummary() const {
    std::cout << "=== Resumo semântico (tabela de símbolos) ===\n";
    bool printedAny = false;

    for (const std::string& className : sortedKeys(classTable)) {
        if (className == "Object" || className == "IO" || className == "Int" ||
            className == "String" || className == "Bool") {
            continue;
        }

        auto clsIt = classTable.find(className);
        if (clsIt == classTable.end()) continue;
        printedAny = true;

        std::cout << "Classe " << className << " inherits " << clsIt->second->parent << "\n";

        auto attrsIt = classAttributes.find(className);
        if (attrsIt != classAttributes.end() && !attrsIt->second.empty()) {
            for (const std::string& attrName : sortedKeys(attrsIt->second)) {
                const auto& attr = attrsIt->second.at(attrName);
                std::cout << "  atributo " << attr.name << " : " << attr.type << "\n";
            }
        }

        auto methodsIt = classMethods.find(className);
        if (methodsIt != classMethods.end() && !methodsIt->second.empty()) {
            for (const std::string& methodName : sortedKeys(methodsIt->second)) {
                const auto& method = methodsIt->second.at(methodName);
                std::cout << "  metodo " << method.name << "(";
                for (size_t i = 0; i < method.formals.size(); ++i) {
                    if (i > 0) std::cout << ", ";
                    std::cout << method.formals[i].first << " : " << method.formals[i].second;
                }
                std::cout << ") : " << method.returnType << "\n";
            }
        }
    }

    if (!printedAny) {
        std::cout << "(nenhuma classe de usuário registrada)\n";
    }
}

#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

#include "AST.hpp"
#include "Lexer.hpp"
#include "Parser.hpp"
#include "SemanticAnalyzer.hpp"
#include "Token.hpp"

// ============================================================================
// main.cpp  (Driver do compilador)
// ----------------------------------------------------------------------------
// O pipeline agora é explícito e linear — cada fase consome o produto da
// anterior e nada é refeito:
//
//   texto --[Lexer]--> Token --[Parser]--> AST --[SemanticAnalyzer]--> ok/erros
//
// Antes, o main instanciava o parser E o analisador semântico passando o texto
// nas duas vezes; o semântico tinha um parser dentro dele e lexava/parseava o
// mesmo arquivo de novo. Aqui o Lexer é criado uma vez só e a AST é construída
// uma vez só.
//
// Uso:
//   coolc <arquivo.cl>              analisa (léxico + sintático + semântico)
//   coolc --tokens <arquivo.cl>     mostra só os tokens (fase 1)
//   coolc --ast <arquivo.cl>        mostra a árvore sintática abstrata
//   coolc --trace <arquivo.cl>      mostra a análise semântica passo a passo
//   coolc --help
//
// Cada flag existe para poder demonstrar uma fase isolada na apresentação.
// ============================================================================

namespace {

void usage(const char* prog) {
    std::cout << "Uso: " << prog << " [--tokens|--ast|--trace|--help] <arquivo.cl>\n\n"
              << "  (sem flags)  analisa o programa completo e reporta erros\n"
              << "  --tokens     imprime apenas a lista de tokens (análise léxica)\n"
              << "  --ast        imprime a árvore sintática abstrata\n"
              << "  --trace      mostra a tabela de símbolos e o tipo inferido de\n"
              << "               cada atributo/método durante a análise semântica\n"
              << "  --help       esta mensagem\n";
}

bool readFile(const std::string& path, std::string& out) {
    std::ifstream arquivo(path);
    if (!arquivo.is_open()) {
        std::cerr << "Erro ao abrir o arquivo: " << path << "\n";
        return false;
    }
    std::stringstream buffer;
    buffer << arquivo.rdbuf();
    out = buffer.str();
    return true;
}

// Fase 1 isolada: mostre o que o lexer produz, sem parser nenhum.
// Devolve false se algum token ERROR foi encontrado (código de saída 1).
bool printTokens(const std::string& source) {
    Lexer lexer(source);
    bool hadError = false;
    std::cout << "--- Tokens Extraídos ---\n";
    while (true) {
        Token token = lexer.nextToken();
        std::cout << "Linha " << token.line << " | Tipo: " << tokenTypeName(token.type)
                  << " | Lexema: \"" << token.lexeme << "\"\n";
        if (token.type == TokenType::ERROR) hadError = true;
        if (token.type == TokenType::END_OF_FILE) break;
    }
    return !hadError;
}

}  // namespace

int main(int argc, char* argv[]) {
    bool tokensOnly = false;
    bool printAst = false;
    bool trace = false;
    std::string caminhoArquivo;

    // 1. Interpretação simples dos argumentos
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            usage(argv[0]);
            return 0;
        }
        if (arg == "--tokens") {
            tokensOnly = true;
        } else if (arg == "--ast") {
            printAst = true;
        } else if (arg == "--trace") {
            trace = true;
        } else if (!arg.empty() && arg[0] == '-') {
            std::cerr << "Opção desconhecida: " << arg << "\n";
            usage(argv[0]);
            return 1;
        } else {
            caminhoArquivo = arg;
        }
    }

    if (caminhoArquivo.empty()) {
        std::cerr << "Erro: Nenhum arquivo de entrada fornecido.\n";
        usage(argv[0]);
        return 1;
    }

    // 2. Lê o arquivo inteiro para um buffer
    std::string conteudo;
    if (!readFile(caminhoArquivo, conteudo)) {
        return 1;
    }

    // 3. Fase 1 — análise léxica
    if (tokensOnly) {
        return printTokens(conteudo) ? 0 : 1;
    }

    // 4. Fase 2 — análise sintática: Lexer -> Parser -> AST
    Lexer lexer(conteudo);
    Parser parser(lexer);
    std::shared_ptr<ProgramASTNode> ast = parser.parse();

    if (!parser.valid() || !ast) {
        std::cerr << "Análise sintática concluída com erros. Verifique as mensagens acima.\n";
        return 1;
    }
    std::cout << "Análise sintática concluída com sucesso. Nenhum erro encontrado.\n";

    if (printAst) {
        ast->print();
    }

    // Garante que as mensagens de sucesso (stdout) apareçam antes dos erros
    // semânticos (stderr) quando os dois fluxos são redirecionados juntos.
    std::cout.flush();

    // 5. Fase 3 — análise semântica: AST -> verificação de tipos
    SemanticAnalyzer semantic(ast, trace);
    bool semResult = semantic.analyze();

    if (trace) {
        semantic.printSummary();
    }

    if (semResult) {
        std::cout << "Análise semântica concluída com sucesso. Nenhum erro encontrado.\n";
    } else {
        std::cout << "Análise semântica concluída com erros. Verifique as mensagens acima.\n";
    }
    return semResult ? 0 : 1;
}

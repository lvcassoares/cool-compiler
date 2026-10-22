# coolc — compilador COOL (análise léxica, sintática e semântica)

Projeto da disciplina: compilador para a linguagem **COOL** (Classroom Object-Oriented
Language — manual de referência em `docs`/curso), escrito em C++17.

O programa lê um arquivo `.cl`, faz as três análises e reporta os erros encontrados.

## Como compilar

Com `make` (Linux / macOS / MSYS2):

```sh
make            # gera ./coolc (ou coolc.exe no Windows)
make run        # compila e roda examples/hello.cl
make test       # roda a suíte completa em tests/
make clean
```

No Windows com o MinGW instalado (o GNU Make se chama `mingw32-make`):

```sh
mingw32-make            # gera coolc.exe
mingw32-make run
mingw32-make tests      # roda a suíte completa em tests/
```

Ou com CMake:

```sh
cmake -B build && cmake --build build
```

Ou direto, sem build system:

```sh
g++ -std=c++17 -Iinclude src/*.cpp -o coolc
```

Estado atual: compila sem nenhum warning com `-Wall -Wextra` (g++ 14.2
neste ambiente). Os 5 exemplos de `examples/` e os 76 testes de `tests/`
passam: programas válidos saem com código `0`, programas com erro saem com
código `1`. Os 18 exemplos oficiais do curso (Stanford CS143), incluindo
`hairyscary.cl`, `lam.cl` e `life.cl`, também foram analisados com sucesso.

> Atenção: este projeto usa as classes `Lexer`, `Parser` e `SemanticAnalyzer`.
> `antigo/` não faz parte do build (`src/*.cpp` não o inclui).

## Como executar

```sh
./coolc examples/hello.cl              # análise completa
./coolc --tokens examples/hello.cl     # só a fase léxica (lista de tokens)
./coolc --ast examples/hello.cl        # árvore sintática abstrata
./coolc --trace examples/hello.cl      # análise semântica passo a passo
./coolc --help
```

Código de saída: `0` = programa válido, `1` = erro (léxico, sintático ou semântico).
Isso permite usar em script de teste.

Exemplos prontos em `examples/`:

| arquivo | serve para mostrar |
|---|---|
| `hello.cl` | programa mínimo válido |
| `completo.cl` | a maior parte da gramática: herança, override, `while`, `let`, `case`, dispatch estático `@`, `isvoid`, `not`, `~` |
| `erros_lexicos.cl` | erro pego na fase 1 |
| `erros_sintaticos.cl` | erro pego na fase 2 |
| `erros_semanticos.cl` | erro pego na fase 3 |

## Testes automatizados

A suíte fica em `tests/` e não depende de framework externo: é um único script
Python que compila o projeto, roda os casos e compara código de saída/mensagens.
O guia detalhado está em [`COMO-RODAR-TESTES.md`](COMO-RODAR-TESTES.md).

```sh
python tests/run_tests.py              # compila e roda tudo
python tests/run_tests.py --verbose    # mostra a saída de cada teste
python tests/run_tests.py --filter sem_  # só os testes semânticos
python tests/run_tests.py --compiler ./coolc.exe  # usa um binário já compilado
```

Estrutura:

```
tests/
  valid/*.cl      devem terminar com código 0 (e sem mensagem de erro)
  invalid/*.cl    devem terminar com código 1
  run_tests.py    compila, executa e confere as expectativas
```

As expectativas ficam em comentários COOL no próprio arquivo (o compilador os
ignora):

```cool
-- @expect exit 1
-- @expect contains Erro semântico na linha 3
-- @expect not_contains Segmentation fault
```

Além dos arquivos, o runner gera casos de borda em memória: byte NUL dentro e
fora de string, string de 1024/1025 caracteres, milhares de comentários `--`,
comentários de bloco com 1000 níveis de aninhamento, múltiplos erros léxicos e
saída determinística entre execuções.

### Como saber que ele não está só imprimindo “sucesso”

Três formas de inspecionar o trabalho real, sem confiar na mensagem final:

```sh
./coolc --tokens examples/completo.cl   # vê os tokens que o lexer produziu
./coolc --ast examples/completo.cl      # vê a árvore que o parser construiu
./coolc --trace examples/completo.cl    # vê tipos inferidos e a tabela de símbolos
```

O `--trace` imprime, para cada classe, cada atributo/método com o tipo inferido
e `OK`/`ERRO`, e no fim a tabela de símbolos coletada. Exemplo:

```text
[trace] Classe Animal inherits Object
  [atributo] nome : String <- String  OK
  [metodo] init(n : String, i : Int) : Animal  corpo: SELF_TYPE  OK
...
=== Resumo semântico (tabela de símbolos) ===
Classe Animal inherits Object
  atributo nome : String
  metodo init(n : String, i : Int) : Animal
```

Se o compilador estivesse só “roubando”, ele não conseguiria imprimir a árvore
correta nem os tipos inferidos de cada expressão. Além disso, a suíte tem casos
válidos e inválidos: se a mensagem de sucesso fosse fixa, os 52 testes de erro
falhariam.

## Arquitetura

Um pipeline, cada fase alimentando a próxima. Nada é refeito: o texto é lido uma
vez, os tokens são produzidos uma vez, a AST é construída uma vez.

```
        texto-fonte (.cl)
              │
              ▼
     ┌─────────────────┐
     │  Lexer          │   include/Lexer.hpp     src/Lexer.cpp
     │  análise léxica │   texto -> Token
     └─────────────────┘
              │  Token  (include/Token.hpp)
              ▼
     ┌─────────────────┐
     │  Parser         │   include/Parser.hpp    src/Parser.cpp
     │  descida recurs.│   Token -> AST
     └─────────────────┘
              │  ProgramASTNode  (include/AST.hpp)
              ▼
     ┌─────────────────┐
     │ SemanticAnalyzer│   include/SemanticAnalyzer.hpp  src/SemanticAnalyzer.cpp
     │  checagem tipos │   AST -> erros/ok
     └─────────────────┘
              │
              ▼
        main.cpp: lê arquivo, liga as fases, imprime resultado
```

Cada módulo depende **apenas** do que está acima dele na figura:

- `Lexer` não sabe que existe AST, tipos ou classes.
- `Parser` não sabe que existe análise semântica; recebe um `Lexer` pronto e
  devolve a `AST`.
- `SemanticAnalyzer` não sabe que existe texto-fonte nem tokens; recebe a `AST`.
- `main.cpp` é o único que conhece as três fases e a ordem entre elas.

O contrato entre as fases são dois tipos de dados, não código:

- **`Token`** (`Token.hpp`) — a interface Lexer → Parser.
- **`ProgramASTNode`** (`AST.hpp`) — a interface Parser → Semântico (e, no futuro,
  → geração de código).

### Por que não `sintatico` dentro de `semantico`

A versão anterior (`antigo/`) tinha três programas independentes e cada um
continha **uma cópia inteira** das fases anteriores: o arquivo da fase 3 tinha um
lexer, um parser e um analisador semântico no mesmo arquivo, e o `main` passava o
**texto-fonte** tanto para o parser quanto para o semântico — o semântico lexava e
parseava o arquivo de novo por dentro. Efeitos: código duplicado três vezes,
programa analisado duas vezes, e as cópias divergiram entre si (ver
`RELATORIO-TECNICO.md`, seção "Divergência entre as três cópias").

Agora existe uma única implementação de cada fase, e a ligação é por dados.

## Estrutura de diretórios

```
include/     cabeçalhos (a "interface" entre as fases)
  Token.hpp            TokenType, Token
  Lexer.hpp            Lexer
  AST.hpp              todos os nós da AST
  Parser.hpp           Parser
  SemanticAnalyzer.hpp SymbolTable + SemanticAnalyzer
src/         implementação
  Lexer.cpp
  Parser.cpp
  SemanticAnalyzer.cpp
  main.cpp
examples/    programas COOL para teste e demonstração
tests/       suíte automatizada (valid/, invalid/ e run_tests.py)
antigo/      as três versões antigas (fase 1, 2, 3), guardadas só para comparação
```

## Relação com o curso

- `antigo/lexico.cpp`      → fase 1 (léxico): lexer + `main` imprimindo tokens
- `antigo/lexsintatico.cpp` → fase 2 (léxico + sintático): lexer copiado + parser
- `antigo/lexsinsemantico.cpp` → fase 3 (léxico + sintático + semântico): tudo copiado
- este repositório        → as três fases em módulos separados, ligados por dados

O comportamento das análises foi preservado; a única mudança de comportamento
consciente está documentada em `RELATORIO-TECNICO.md`.

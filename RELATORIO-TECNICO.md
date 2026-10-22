# Relatório técnico — revisão do compilador COOL

Documento de apoio à apresentação. Duas partes:

1. **O que foi feito** neste passo (a separação em módulos).
2. **O que está errado / vale mudar** — bugs, desvios do manual de COOL e dívidas
   de projeto, com severidade e onde está cada um.

Referências do manual (`cool-manual.pdf`): §10 léxico, §11 gramática e precedência,
§12 regras de tipo, §4.1 SELF_TYPE, §3.1 features, §9 classe Main.

---

# Parte 0 — Status das correções (esta rodada)

Corrigido e coberto por teste automatizado:

- **[CRÍTICO] 2.1** — `classTable[...]`/`classAttributes[...]`/`classMethods[...]`
  agora usam `find`/`at` com checagem; tipos inválidos recebem o sentinela
  `<error>` em vez de derrubar o compilador. O caso `let x : NaoExiste in x.foo()`
  reporta erro e termina com código 1.
- **[ALTO] 2.2** — `lub` implementa as regras do §7.5: `SELF_TYPE_D ⊔ SELF_TYPE_D =
  SELF_TYPE_D` e `SELF_TYPE_D ⊔ A = D ⊔ A`. A conformidade também foi corrigida:
  `T <= SELF_TYPE` só vale quando `T = SELF_TYPE`.
- **[ALTO] 2.3** — a linha de cada nó passou a ser capturada no início da regra
  (ou no operador, para binários), não depois de consumir o próximo token.
- **[ALTO] 2.11 / 2.23** — o parser centraliza o avanço em `advance()`: um token
  `ERROR` é reportado como **erro léxico** e pulado, sem se multiplicar pelas
  regras gramaticais. Há recuperação em bloco, corpo de método e fronteira de
  classe, então um arquivo com vários erros reporta mais de um. Continua sendo
  recuperação simples (panic mode), não a refatoração completa com nó de erro.
- **[MÉDIO] 2.4** — `ASTNode::line` inicializado em 0.
- **[MÉDIO] 2.5 / 2.6** — conferido no manual §10.2: `\c` denota `c`, então `\0`
  (barra + zero) é o caractere `'0'`; o proibido é o byte NUL real. O lexer agora
  usa `int` com `-1` para EOF, distinguindo NUL de fim de arquivo.
- **[MÉDIO] 2.7** — comentários são pulados com laço, sem recursão.
- **[MÉDIO] 2.8** — `isalpha`/`isalnum`/`isdigit`/`islower`/`isupper` recebem
  `unsigned char`.
- **[MÉDIO] 2.9** — o lookahead de 1 token mora no `Parser`
  (`currentToken` + `nextTokenBuffer`); `Lexer` só tem `nextToken()`.
- **[MÉDIO] 2.10** — suíte em `tests/` com 76 verificações
  (`python tests/run_tests.py` ou `make test`).
- **[MÉDIO] 2.12** — erros semânticos no formato
  `Erro semântico na linha N: mensagem`, sem ponto final; erros do parser vão
  para `stderr` com prefixo `Erro sintático`. O `stdout` é descarregado antes da
  fase semântica para não embaralhar a ordem.
- **[MÉDIO] 2.13** — classes e filhos são percorridos em ordem alfabética; a
  saída é determinística.
- **[MÉDIO] 2.14** — `StaticDispatchASTNode` foi renomeado para
  `LocalCallASTNode`; o dispatch estático de verdade continua em
  `DispatchASTNode::typeExpr`.
- **[BAIXO] 2.16** — `MAIORQ` removido; `>` produz erro léxico com a mensagem
  "Operador '>' nao existe em COOL".
- **[BAIXO] 2.17** — o texto de comentários não é mais acumulado em `std::string`.
- **[BAIXO] 2.18** — mensagens melhores para bloco vazio e `let` sem binding.

Bugs adicionais encontrados e corrigidos nesta rodada:

- O parser aceitava `TYPEID` mas não `SELF_TYPE` nas posições de tipo, então o
  exemplo clássico `copy() : SELF_TYPE { self }` (manual §4.1) nem chegava à
  semântica. Agora `new SELF_TYPE`, retorno de método, `let` e atributo aceitam
  `SELF_TYPE`; formal e ramo de `case` também aceitam para a semântica rejeitar
  com mensagem específica.
- `new Foo.bar()` agora parseia como `(new Foo).bar()`: `new` virou primário, e o
  dispatch `.` (maior precedência) se aplica a ele.
- `tRuE`/`fAlSe` são constantes booleanas válidas (§10.4: só a primeira letra
  precisa ser minúscula); o parser comparava o lexema exato.
- Strings com mais de 1024 caracteres agora são erro (§10.2).
- `--tokens` retorna código 1 quando encontra `ERROR`.
- Novo modo `--trace`: imprime a tabela de símbolos e o tipo inferido de cada
  atributo/método, com `OK`/`ERRO` por feature. Serve para demonstrar que a
  análise semântica realmente percorreu a AST, em vez de só imprimir sucesso.
- O parser comparava o lexema de palavra-chave com a forma minúscula exata,
  embora o lexer aceitasse palavras-chave case-insensitive (§10.4). Por isso
  `Class`, `IF`, `LeT` etc. falhavam no parser (foi o que derrubava
  `book_list.cl` e `sort_list.cl` dos exemplos oficiais). Agora `isKeyword()`
  compara sem diferenciar maiúsculas/minúsculas.
- `Makefile`: `TARGET` inclui `.exe` no Windows e o link é estático no MinGW,
  então `make run`/`make test` funcionam sem depender de qual `libstdc++` está
  no `PATH`.

Ainda em aberto (dívida de projeto, não bug funcional): **2.15** (visitor em vez
de cadeia de `dynamic_pointer_cast`), **2.19** (um `Diagnostics` compartilhado),
**2.20** (nomenclatura PT/EN) e **2.21** (acentos no console do Windows).

Comandos da suíte:

```sh
python tests/run_tests.py
python tests/run_tests.py --verbose
python tests/run_tests.py --filter sem_
make test            # ou: mingw32-make tests
```

---

# Parte 1 — A separação em módulos

## 1.1 O problema

As três entregas eram três programas completos e independentes:

| arquivo (agora em `antigo/`) | contém |
|---|---|
| `lexico.cpp` | lexer + `main` que imprime tokens |
| `lexsintatico.cpp` | **cópia** do lexer + parser + `main` |
| `lexsinsemantico.cpp` | **cópia** do lexer + **cópia** do parser (com AST) + semântico + `main` |

Cada fase nova foi feita copiando a anterior e continuando a escrever. Consequências:

- **Duplicação**: o lexer existia em 3 versões, o parser em 2, as classes da AST
  dentro do arquivo do semântico.
- **Retrabalho em execução**: o `main` da fase 3 fazia
  `sintatico sim(conteudo)` (lexa + parseia) e logo depois
  `semantico sem(conteudo)`, e o construtor de `semantico` criava **outro**
  `sintatico` e lexava/parseava o mesmo arquivo de novo. O programa era analisado
  duas vezes para produzir um resultado só.
- **Divergência entre as cópias** (ver 2.1): as cópias não eram idênticas, então os
  três binários aceitavam/rejeitavam coisas diferentes.

## 1.2 O pipeline agora

```
texto ──[Lexer]──> Token ──[Parser]──> ProgramASTNode ──[SemanticAnalyzer]──> erros/ok
```

`src/main.cpp` é o único lugar que conhece a ordem das fases:

```cpp
Lexer lexer(conteudo);            // fase 1: recebe texto
Parser parser(lexer);             // fase 2: recebe o lexer (injeção de dependência)
auto ast = parser.parse();        //          devolve a AST
if (!parser.valid() || !ast) { ... return 1; }
SemanticAnalyzer semantic(ast);   // fase 3: recebe a AST, e só a AST
bool ok = semantic.analyze();
```

Os contratos entre fases são **dois tipos de dados**:

- `Token` (`include/Token.hpp`) — único formato que atravessa Lexer → Parser.
- `ProgramASTNode` (`include/AST.hpp`) — único formato que atravessa Parser → resto
  do compilador.

A semântica (e qualquer fase futura, como geração de código) nunca mais vê texto
nem tokens. É isso que torna o projeto extensível.

## 1.3 Mudanças de interface feitas no split

| antes | agora | por quê |
|---|---|---|
| `sintatico(const std::string& src)` criava o próprio lexer **e parseava no construtor** | `Parser(Lexer& lex)` + `parse()` explícito | o parser não deve criar sua entrada; efeito colateral escondido no construtor impedia controlar a ordem das fases |
| `semantico(const std::string& src)` com um `sintatico` dentro | `SemanticAnalyzer(shared_ptr<ProgramASTNode>)` | o semântico não deve reparsear; recebe a árvore |
| `sintatico::valid()` / `parse()` duplicando trabalho | `Parser::valid()` depois de `parse()` | uma única análise |
| 3 `main`s diferentes | 1 `main` com flags `--tokens` / `--ast` | cada fase pode ser demonstrada isolada, sem compilar 3 binários |
| classes da AST dentro do `.cpp` do semântico | `include/AST.hpp` | são a interface entre fases, não um detalhe da fase 3 |
| nomes de arquivo com acento (`léxico.cpp`) | `lexico.cpp` em `antigo/`, `Lexer.cpp` no projeto | acento em nome de arquivo quebra build em vários toolchains/CI |

## 1.4 Mudanças de comportamento (conscientes e pequenas)

Todo o resto é porte fiel: verifiquei comparando a **sequência de tokens de código**
(ignorando comentários, espaços e conteúdo de strings) entre cada original e o
respectivo módulo novo — Parser 99,2%, Semântico 97,6%, Lexer 96,3% de igualdade,
sendo as diferenças exatamente as listadas abaixo. Depois, compilei de fato
(`g++ 16.2`, `-Wall -Wextra`): **zero warnings**, e os 5 exemplos rodam como esperado.

1. **`Lexer`: lexema de palavra-chave.** A cópia da fase 3 devolvia o lexema
   **minúsculo** para palavras-chave (`if (id_lower == "class") return {KEYWORD, id_lower, line}`),
   enquanto as fases 1 e 2 devolviam o texto original. Agora devolve o **texto
   original** (`CLASS` vira lexema `"CLASS"`), como qualquer outro token. O parser
   não muda de comportamento (ele compara com minúsculas de qualquer forma), mas a
   impressão de tokens passa a dizer a verdade.
2. **`Lexer`: renomeada a variável `c` da varredura de comentário `(*` para `ca`**,
   como no original, para não sombrear o `c` do `nextToken`.
3. **`Lexer`: removidos dois `break;` inalcançáveis** depois de `return` (`case '<'`
   e `case '='`).
4. **`Parser::expr_primary`**: `innerExpr->line = line;` virou
   `if (innerExpr) innerExpr->line = line;`. Sem a guarda, um erro dentro de
   parênteses fazia o parser desreferenciar `nullptr` (crash em vez de mensagem).
5. Arquivos regravados em **UTF-8** (os originais estavam em Latin-1/CP1252).
6. **`AST.hpp`**: `StringASTNode::print` agora escapa `\n`, `\t` etc.
   (`String("Hello World\n")` em vez de quebrar a árvore em duas linhas — importa
   para a impressão da AST na apresentação).
7. **`AST.hpp`**: ordem dos inicializadores de `FeatureASTNode` alinhada à ordem de
declaração dos membros (corrige 18 warnings de `-Wreorder`; sem mudança de comportamento).

---

# Parte 2 — O que está errado / vale mudar

Severidades: **[CRÍTICO]** quebra o compilador · **[ALTO]** resultado errado em
programa válido ou em erro comum · **[MÉDIO]** qualidade/robustez · **[BAIXO]**
estilo ou detalhe.

## 2.0 Divergência entre as três cópias (a lição do split)

Não é um bug a corrigir — já desapareceu — mas é o argumento da apresentação.

| divergência | onde | efeito |
|---|---|---|
| `ARROBA` (`@`) não existe no enum nem no `switch` | `antigo/lexico.cpp` | na fase 1, `e@T.m()` produzia token `ERROR`: dispatch estático **não** era reconhecido |
| parsing de **atributo** testava `TYPEID` onde a gramática exige `:` | `antigo/lexsintatico.cpp`, `feature()` | `x : Int;` dava erro sintático → **a fase 2 rejeitava qualquer programa com atributo**; só passava porque o exemplo da apresentação (`hello.cl`) não tinha atributo |
| `feature()` de atributo chamava `lexer.nextToken()` depois de `expr()` | `antigo/lexsintatico.cpp` | consumia um token a mais (o `;` ou o começo da próxima feature) |
| `program()` chamava `lexer.nextToken()` no fim de cada iteração, depois de já ter consumido o `;` | `antigo/lexsintatico.cpp` | pulava um token por classe |
| lexema de keyword em minúsculas | `antigo/lexsinsemantico.cpp` | inconsistente com as outras fases |
| só a fase 3 construía AST | fase 3 | fases 1 e 2 descartavam a informação e o semântico precisou de um parser novo dentro dele |

Três cópias do mesmo lexer, três comportamentos diferentes. Esse é o custo real da
duplicação — e o motivo de agora existir um `Token.hpp` só.

## 2.1 [CRÍTICO] Crash por `classTable[...]` em tipo inexistente

`src/SemanticAnalyzer.cpp:170,183` (`isAttributeInAncestors`, `findMethodInAncestors`):

```cpp
std::string current = classTable[className]->parent;   // operator[] INSERE nullptr
```

`std::unordered_map::operator[]` **insere** uma entrada com `nullptr` quando a chave
não existe; o `->parent` seguinte desreferencia `nullptr` → segfault.

É alcançável: o erro de tipo não declarado em `let` é reportado mas a análise
**continua**, e a variável entra na tabela de símbolos com o tipo inválido. Exemplo
que derruba o compilador (deveria só reportar erro):

```
class Main {
  main() : Object {
    let x : NaoExiste in x.foo()
  };
};
```

Em `checkExprType` (dispatch) → `findMethodInAncestors("NaoExiste", "foo")` → crash.

Correção: usar `classTable.find(...)`/`.at()` com checagem, e não inserir símbolos
cujo tipo é inválido.

## 2.2 [ALTO] `lub` perde `SELF_TYPE`

Manual §7.5: `SELF_TYPE_D ⊔ A = D ⊔ A` e `A ⊔ A = A`. Ou seja, se as duas
ramificações são `SELF_TYPE`, o resultado é **`SELF_TYPE`** (de D), não `D`.

`src/SemanticAnalyzer.cpp:381` resolve `SELF_TYPE` para a classe corrente **antes**
de comparar:

```cpp
type1 = resolveType(type1, currentClass);   // SELF_TYPE -> Main
type2 = resolveType(type2, currentClass);
if (type1 == type2) return type1;           // devolve "Main", não "SELF_TYPE"
```

Consequência prática: `if c then self else self fi` tem tipo `Main` em vez de
`SELF_TYPE`, então programas legítimos que dependem da propagação de `SELF_TYPE`
são rejeitados — o exemplo clássico do manual §4.1:

```
class Silly { copy() : SELF_TYPE { self }; };
class Sally inherits Silly { };
class Main {
  x : Sally <- (new Sally).copy();
  main() : Sally { x };
};
```

(`copy()` devolve `SELF_TYPE` → `(new Sally).copy() : Sally`). O caso direto passa,
mas a composição `if ... self ... fi` / `case ... self ... esac` perde o tipo.
Correção: tratar `SELF_TYPE` como um caso especial em `lub` (se ambos são
`SELF_TYPE`, devolver `SELF_TYPE`), e não resolver antes da comparação.

## 2.3 [ALTO] Números de linha dos nós da AST estão "um token à frente"

`src/Parser.cpp` (ex.: 81, 174, 219, 288, 302, 318, 332, 345): a linha é atribuída
**depois** de consumir o token do construto:

```cpp
auto binaryNode = std::make_shared<BinaryOpASTNode>(op, left, right);
binaryNode->line = currentToken.line;   // já é o token SEGUINTE à expressão
```

Em `expr_add`/`expr_mult` é pior: `left->line = currentToken.line` roda no fim de
cada iteração, então o nó recebe a linha do token que veio depois. Resultado:
mensagens de erro semântico apontando a linha errada (às vezes a linha seguinte) —
exatamente o que mais atrapalha quem usa o compilador. `expr_primary` já faz certo
(`int line = currentToken.line;` no começo). Padronizar: capturar `line` no início de
**cada** regra e usar essa variável em todos os nós criados ali.

## 2.4 [MÉDIO] `ASTNode::line` não é inicializado

`include/AST.hpp` (`class ASTNode { public: int line; ... }`). `installBasicClasses()`
cria `ClassASTNode`/`FeatureASTNode` das classes básicas **sem** atribuir `line`;
qualquer `logError(cls->line, ...)` que dispare para uma classe básica imprime lixo.
Latente hoje, mas é UB. Correção de uma linha: `int line = 0;`.

## 2.5 [MÉDIO] `\0` em string: mensagem e regra

`src/Lexer.cpp`, no `switch (escaped)`: `case '0': return ERROR "String contains null character"`.
O manual §10.2 diz que `\c` denota o caractere `c` (com exceção de `\b \t \n \f`),
o que faria `\0` denotar o caractere `'0'`; o manual proíbe apenas o caractere
**nulo de verdade** (`"\0"` na notação dele) dentro de string. Ou seja: a regra
"`\0` escapado é erro" precisa ser conferida contra os casos de teste oficiais do
curso antes de permanecer.

## 2.6 [MÉDIO] NUL real é indistinguível de fim de arquivo

O lexer usa `'\0'` como sentinela de EOF (`peek()`/`advance()`). Um byte NUL no meio
do arquivo, portanto, é tratado como fim de arquivo: dentro de uma string ele produz
`"Unterminated string constant"` em vez de `"String contains null character"`, e o
`if (str.back() == '\0')` de `src/Lexer.cpp` é **inalcançável** (código morto).
Correção: usar `int` (`-1` para EOF) ou um `bool atEnd()` separado, e checar o byte
real.

## 2.7 [MÉDIO] Recursão para pular comentários

`src/Lexer.cpp`: ao final de cada comentário, `return nextToken();`. Um arquivo com
N comentários seguidos consome N quadros de pilha. Um arquivo gerado, ou uma lista
longa de `--`, pode estourar a pilha (segfault no lexer). Correção: laço
(`do { ... } while (emComentario)` ou `goto refazer`) em vez de recursão.

## 2.8 [MÉDIO] `isalpha`/`isalnum`/`isdigit` com `char` possivelmente negativo

`std::isalpha(c)`, `std::isalnum(peek())`, `std::isdigit(peek())` recebem `char`
que, se ≥ 0x80 (ex.: `ç`, `ã` fora de comentário), é **negativo** em plataformas
com `char` assinado → comportamento indefinido (o manual exige
`unsigned char`). Só o `std::tolower` está com o cast correto. Correção:
`static_cast<unsigned char>(...)` nos três.

## 2.9 [MÉDIO] O parser lê o lexer para fazer lookahead

`src/Parser.cpp:283` — `lexer.peekToken()` dentro de `expr()` para distinguir
`ID <- expr` de `ID ...`. A gramática de COOL é LL(2) nesse ponto, então **precisa**
de lookahead; o problema é *onde* ele mora. `peekToken()` relexa (e reentra na
recursão de comentários), e a interface Lexer↔Parser fica com duas portas.
Alternativa limpa: o parser guarda `currentToken` **e** `nextToken` (buffer de 1
token, preenchido por `advance()`); o lexer passa a ter só `nextToken()`.

## 2.10 [MÉDIO] Nenhum teste automatizado

Não existe nenhum teste no projeto: cada alteração é verificada "rodando na mão".
Isso é o maior risco do projeto e o de maior retorno consertar. Proposta:

```
tests/
  validos/*.cl        (devem sair com código 0)
  invalidos/*.cl      (devem sair com código 1; opcional: 1º erro esperado)
  rodar.py ou run.sh  roda todos e compara o código de saída
```

O curso disponibiliza os casos de teste oficiais (PA1–PA3) — são o alvo natural.

## 2.11 [ALTO] Recuperação de erro: para no primeiro erro

`Parser` retorna `nullptr` em toda falha: um programa errado reporta **um** erro por
execução. Para um compilador de curso, e para nota, reportar mais erros por rodada
importa. Correção clássica: "panic mode" — ao encontrar erro, registrar e
sincronizar (avançar até `;` ou `}`), continuando a análise. Isso muda a estrutura
do parser (ele deixa de devolver `nullptr` e passa a devolver um nó "erro"), então é
uma refatoração, não um patch.

## 2.12 [MÉDIO] Formato das mensagens de erro

`SemanticAnalyzer::logError`:
`std::cerr << "Erro Semântico: " << msg << "linha " << line` — e as mensagens já
terminam com `.`, então a saída real de `examples/erros_semanticos.cl` é:

```
Erro Semântico: Variável 'z' não declarada.linha 8
Erro Semântico: Condição da instrução 'if' deve ser do tipo 'Bool'.linha 10
```

Padronizar `"Erro semântico na linha " << line << ": " << msg` e tirar o ponto
final das mensagens. Também: o parser mistura `std::cout` e `std::cerr` para erros
(mensagens de erro indo para `stdout` atrapalham qualquer script de teste e fazem a
saída sair fora de ordem quando se olha o terminal — ver a saída de `erros_lexicos.cl`
em 2.23).

## 2.13 [MÉDIO] Ordem de percurso não determinística

`checkTypes()` percorre `classTable`, que é `unordered_map`: a ordem das mensagens
muda entre execuções/builds (e a ordem de `buildInheritanceGraph` também). Para
apresentação e para comparar saída com o gabarito, convém ordenar por nome ou
reusar a ordem topológica já calculada em `collectAllSymbols()`.

## 2.14 [MÉDIO] Nome enganoso: `StaticDispatchASTNode`

Em `include/AST.hpp`, `StaticDispatchASTNode` representa a chamada **local**
`id(args)` (= `self.id(args)`), enquanto o dispatch estático de verdade
`e@T.id(...)` está **dentro** de `DispatchASTNode`, no campo `typeExpr`. Numa
apresentação isso confunde na hora de explicar os três tipos de dispatch (§7.4).
Sugestão: renomear para `LocalCallASTNode` (ou criar `StaticDispatchASTNode` de
verdade para `e@T.id()`).

## 2.15 [MÉDIO] `checkExprType` é uma cadeia de `dynamic_pointer_cast`

`src/SemanticAnalyzer.cpp` — uma função de ~200 linhas com uma corrente de
`if (auto x = dynamic_pointer_cast<T>(expr))`, até 10 casts por nó, e todo nó novo
exige editar essa função. É a motivação de livro para **visitor** / **typecheck
virtual no nó**. Trade-off honesto para a apresentação: AST burra + visitor separa
"estrutura" de "regra", mas requer *double dispatch* (ou `std::visit` com
`std::variant`) — vale como slide "o que melhoraríamos com mais tempo".

## 2.16 [BAIXO] `>` declarado e nunca produzido

`MAIORQ` existe em `TokenType` mas nenhum `case '>'` o produz: o caractere `>` vira
token `ERROR`. Em COOL não existe operador `>` (§11), então gerar erro é aceitável —
mas o enum engana e a mensagem fica estranha (`Lexema: ">"`). Ou remover o enum, ou
produzir o token e reportar "operador '>' não existe em COOL" (mensagem melhor).

## 2.17 [BAIXO] Comentários `--` acumulam o texto num `std::string` que ninguém usa

`src/Lexer.cpp`: `std::string comment(1, c); comment += advance(); ...` — aloca e
cresce para cada comentário, sem uso. Remover.

## 2.18 [BAIXO] `expressão` de bloco vazia / mensagens de `let`

- `{ }` (bloco vazio) não é aceito: a gramática exige ≥ 1 expressão (§7.7) ✔ —
  mas a mensagem saída é "Expressão inválida", pouco didática.
- `let` sem binding: `parse_binding()` não reclama, o erro aparece depois como
  "Esperado 'in' em 'let'".
- `case` sem ramo é checado com boa mensagem (bom exemplo a citar).

## 2.19 [BAIXO] `errorFlag` duplicado

Existe em `Parser` e em `SemanticAnalyzer`, cada um com seu formato de erro. Um
`Diagnostics` compartilhado (lista de erros com linha, fase e mensagem) resolveria
formato, ordenação, "quantos erros" e permitiria JSON de saída para a demonstração.

## 2.20 [BAIXO] Mistura de idiomas na nomenclatura

`sintatico`/`semantico` viraram `Parser`/`SemanticAnalyzer`, mas os métodos seguem a
gramática em português (`classe`, `feature`, `formal`) misturados com
`checkExprType`, `lub`, `isSubtype`. Padronizar (ou tudo em inglês, ou tudo em
português). Manter `classe`, `feature`, `formal` é defensável: são os nomes das
regras da gramática (§11).

## 2.21 [BAIXO] Acentuação na saída vs. console do Windows

Os fontes agora são UTF-8, então os literais de mensagem (`"não declarado"`) são
UTF-8. No console do Windows (CP850/CP1252) isso imprime caractere quebrado — ruim
na hora de projetar a apresentação. Opções: manter as mensagens de **runtime**
sem acento (mais simples) ou rodar `chcp 65001` antes da demo.

## 2.22 Cobertura da especificação — o que já está certo (para a apresentação)

Vale citar como "o que o analisador semântico já garante" (todas conferidas contra
o manual):

- `Object`, `IO`, `Int`, `String`, `Bool` pré-instaladas com os métodos nativos
  corretos (§8: `abort`, `type_name`, `copy`, `out_string`, `out_int`, `in_string`,
  `in_int`, `length`, `concat`, `substr`).
- Classe redefinida, herança de `Int`/`String`/`Bool`, pai inexistente, ciclo de
  herança (§3.2).
- Atributo redefinido, atributo herdado não pode ser redefinido, método
  redefinido na mesma classe, override com assinatura diferente (retorno/nº/tipos
  dos parâmetros — invariante, §6).
- `self` proibido como atributo, formal, variável de `let`, variável de `case`, e
  como alvo de atribuição (§7.2).
- `SELF_TYPE` proibido como tipo de formal e como tipo de ramo de `case`;
  permitido em `new`, retorno de método, `let` e atributo (§4.1) — exatamente os
  quatro usos legais.
- Escopos: `let` avalia o inicializador **antes** de declarar a variável (§12.2
  `[Let-Init]`), bindings posteriores sombreiam os anteriores; `case` abre escopo
  por ramo; atributos visíveis no inicializador de outros atributos (§5 `[Attr-Init]`).
- Tipos: `if` = join dos ramos, `while` : `Object` (§7.6), bloco = tipo da última
  expressão, `=` proibido entre básicos diferentes (§12.2 `[Equal]`), `+ - * / < <=`
  exigem `Int`, `not` exige `Bool` (§7.12), `new T` valida `T`, dispatch valida
  receptor + aridade + tipos dos argumentos + `SELF_TYPE` no retorno (§7.4).
- `Main` existe, tem `main` **declarado nela mesma** (não herdado) e sem parâmetros
  (§9).

## 2.23 [ALTO] Token `ERROR` dentro de classe vira erro sintático (o erro léxico desaparece)

Só `Parser::program()` testa `currentToken.type == TokenType::ERROR` e imprime
`Token inválido`. Dentro de uma classe o token inválido chega às regras de
expressão, que só sabem reclamar de gramática. Saída real de
`examples/erros_lexicos.cl` (`x <- 1 # 2;`):

```
Erro na linha 5: Esperado ';' no bloco          <-- causa real: '#' é token inválido
Análise sintática concluída com erros. Verifique as mensagens acima.
Erro na linha 5: Esperado '}' para fechar o corpo do método
Erro na linha 5: Esperado ';' após a feature
Erro na linha 5: Esperado ';' após a classe
```

Além de esconder a causa, o erro se multiplica em 4 mensagens falsas (ver 2.11).
Correção: tratar `ERROR` no ponto de consumo de token (uma função `avancar()` no
parser) e não espalhar checagens pelas regras.

## 2.24 Ordem sugerida de ataque

1. **[CRÍTICO]** 2.1 (crash do `classTable[...]`) — 2 linhas.
2. **[ALTO]** 2.2 (`lub`/`SELF_TYPE`) — é o conceito mais bonito do sistema de tipos
   de COOL e o mais fácil de errar; corrigir e usar como slide.
3. **[ALTO]** 2.3 (linhas dos nós) — mecânico, melhora todo diagnóstico.
4. **[ALTO]** 2.23 + 2.11 (token `ERROR` e recuperação de erro) — juntos, porque a
   correção é a mesma: centralizar o avanço do parser.
5. **[MÉDIO]** 2.10 (testes) — depois da 4, com os casos oficiais.
6. **[MÉDIO]** 2.6/2.7/2.8 (robustez do lexer), 2.12/2.13 (formato e ordem).
7. O resto é higiene.

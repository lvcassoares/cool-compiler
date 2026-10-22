# Como rodar os testes do coolc

Este documento explica:

- o que a suíte de testes verifica;
- como rodar os testes;
- como interpretar `PASS`, `FAIL` e o resumo final;
- como adicionar novos casos;
- como inspecionar o compilador manualmente com `--tokens`, `--ast` e `--trace`.

O projeto fica em:

```text
C:/Users/Pichau/Documents/fairy poses/learn/learn/mycoolcompiler-main/mycoolcompiler-main
```

Entre nessa pasta antes de rodar qualquer comando:

```sh
cd "C:/Users/Pichau/Documents/fairy poses/learn/learn/mycoolcompiler-main/mycoolcompiler-main"
```

---

## 1. O que é testado

O `coolc` é um compilador de frente (front-end). Ele faz três análises:

```text
texto-fonte -> Lexer -> tokens -> Parser -> AST -> SemanticAnalyzer -> ok/erros
```

Os testes verificam as três fases:

### Léxico
- caracteres inválidos, como `#`, `$` e `>`;
- string sem fechar;
- quebra de linha não escapada dentro de string;
- `*)` sem `(*`;
- comentário `(*` sem fim;
- byte NUL real dentro e fora de string;
- string com mais de 1024 caracteres;
- muitos comentários `--` e comentários de bloco aninhados.

### Sintático
- expressão faltando depois de `<-`;
- bloco vazio `{ }`;
- `let` sem binding;
- falta de `;` entre features/classes;
- `case` sem ramo;
- `inherits` sem tipo;
- falta de `}`;
- recuperação de erro: mais de um erro no mesmo arquivo.

### Semântico
- variável, atributo, método, parâmetro ou classe não declarada;
- tipo incompatível em atribuição, inicialização, argumento e retorno;
- `SELF_TYPE` usado de forma inválida;
- `lub`/join com `SELF_TYPE`;
- override com retorno, aridade ou tipo de parâmetro diferente;
- atributo herdado sendo redefinido;
- herança de `Int`, `String` ou `Bool`;
- ciclo de herança;
- classe `Main` ausente, `main` herdado ou `main` com parâmetros;
- comparação `=` entre tipos básicos diferentes;
- operadores aritméticos/lógicos com tipos errados;
- dispatch estático `@Tipo` inválido;
- tipos duplicados em `case`.

Além dos arquivos, o runner gera casos especiais em memória:

- byte NUL dentro de string;
- byte NUL no código;
- string de 1024 caracteres (válida);
- string de 1025 caracteres (inválida);
- 5000 comentários de linha;
- 1000 níveis de comentários de bloco;
- múltiplos erros léxicos no mesmo arquivo;
- execução determinística: o mesmo arquivo deve produzir a mesma saída duas vezes.

Também são testados:

- `--help`;
- execução sem arquivo;
- opção desconhecida;
- arquivo inexistente;
- `--tokens` em programa válido e em programa com erro léxico;
- `--trace` em programa válido.

O total atual é **76 verificações**.

---

## 2. Estrutura da pasta `tests/`

```text
tests/
  valid/*.cl       programas que devem terminar com código 0
  invalid/*.cl     programas que devem terminar com código 1
  run_tests.py     runner que compila e executa tudo
```

Cada arquivo `.cl` pode declarar expectativas em comentários COOL. O compilador
ignora esses comentários, mas o runner os lê:

```cool
-- @expect exit 1
-- @expect contains Erro semântico na linha 3
-- @expect not_contains Segmentation fault
```

Significado:

- `exit N`: código de saída esperado (`0` para válido, `1` para erro).
- `contains TEXTO`: a saída precisa conter esse texto.
- `not_contains TEXTO`: a saída não pode conter esse texto.

Para testes válidos, o runner já exige por padrão:

- código de saída `0`;
- presença de `Análise semântica concluída com sucesso`;
- execução de `--ast` com sucesso.

Para testes inválidos, o runner já exige por padrão:

- código de saída `1`;
- pelo menos uma mensagem de erro.

As expectativas podem ficar no topo do arquivo ou no fim. No caso de string não
terminada, elas ficam no topo, porque tudo depois da aspa faria parte da string.

---

## 3. Como rodar

### Opção 1: pelo Makefile

Windows com MinGW:

```sh
mingw32-make tests
```

Linux/macOS/MSYS2:

```sh
make tests
```

`make test` é um alias para `make tests`.

O runner compila o compilador em `tests/build/coolc_test.exe` (ou
`tests/build/coolc_test` fora do Windows) e roda a suíte inteira.

### Opção 2: direto com Python

```sh
python tests/run_tests.py
```

No Linux/macOS pode ser necessário usar `python3`:

```sh
python3 tests/run_tests.py
```

O runner vai:

1. compilar `src/*.cpp` com `g++`;
2. criar um executável de teste em `tests/build/`;
3. rodar todos os arquivos de `tests/valid` e `tests/invalid`;
4. rodar os testes de CLI e os casos gerados em memória;
5. imprimir `PASS`/`FAIL` e um resumo.

No Windows/MinGW o executável de teste é linkado com
`-static-libstdc++ -static-libgcc`. Isso evita o problema de carregar uma
`libstdc++` incompatível de outro diretório no `PATH`.

---

## 4. Opções do runner

### Mostrar a saída de cada teste

```sh
python tests/run_tests.py --verbose
```

ou:

```sh
python tests/run_tests.py -v
```

### Rodar só uma parte da suíte

```sh
python tests/run_tests.py --filter sem_
python tests/run_tests.py --filter lex_
python tests/run_tests.py --filter syn_
python tests/run_tests.py --filter cli_
```

O filtro compara com o caminho do teste. Por exemplo, `sem_` roda os testes
semânticos.

### Usar um compilador já compilado

```sh
python tests/run_tests.py --compiler ./coolc.exe
```

Isso não recompila nada: o runner usa o executável indicado.

### Escolher outro compilador C++

```sh
python tests/run_tests.py --cxx clang++
```

O padrão é `g++`, ou a variável de ambiente `CXX`.

---

## 5. Como interpretar a saída

Exemplo de teste passando:

```text
PASS valid/hello.cl
PASS invalid/sem_undefined_var.cl
```

Exemplo de teste falhando:

```text
FAIL invalid/sem_undefined_var.cl: saída não contém: 'Identificador 'y' não foi declarado'
--- saída ---
Erro semântico na linha 3: Identificador 'x' não foi declarado
```

No fim:

```text
Resumo: 76 passaram, 0 falharam
```

O runner retorna:

- `0` se todos passaram;
- `1` se pelo menos um falhou.

Se um teste travar ou demorar demais, aparece `FAIL ...: timeout`.

---

## 6. Como adicionar um novo teste

### Programa válido

Crie um arquivo em `tests/valid/`:

```cool
class Main inherits IO {
    main() : Object {
        out_string("ok\n")
    };
};
```

Rode só ele:

```sh
python tests/run_tests.py --filter nome_do_arquivo
```

### Programa inválido

Crie um arquivo em `tests/invalid/`:

```cool
class Main {
    x : Int <- "texto";
    main() : Object { 0 };
};
-- @expect exit 1
-- @expect contains Tipo de inicialização 'String' incompatível
```

Rode:

```sh
python tests/run_tests.py --filter nome_do_arquivo
```

Dica: use um trecho único da mensagem esperada em `contains`, para o teste não
passar por acidente com outra mensagem parecida.

---

## 7. Inspeção manual: provando que ele não está só imprimindo “sucesso”

O compilador não executa o programa COOL. Ele só analisa. Para ver o trabalho
interno, use os modos de inspeção.

### Tokens

```sh
./coolc.exe --tokens examples/hello.cl
```

Mostra o tipo, o lexema e a linha de cada token.

### AST

```sh
./coolc.exe --ast examples/completo.cl
```

Mostra a árvore sintática construída pelo parser.

### Trace semântico

```sh
./coolc.exe --trace examples/completo.cl
```

Mostra, para cada classe, cada atributo/método com o tipo inferido e o status
`OK`/`ERRO`, além da tabela de símbolos final.

Exemplo:

```text
[trace] Classe Animal inherits Object
  [atributo] nome : String <- String  OK
  [metodo] init(n : String, i : Int) : Animal  corpo: SELF_TYPE  OK

[trace] Classe Main inherits IO
  [atributo] animal_favorito : Animal <- Gato  OK
  [metodo] main() : Object  corpo: SELF_TYPE  OK

=== Resumo semântico (tabela de símbolos) ===
Classe Animal inherits Object
  atributo nome : String
  metodo init(n : String, i : Int) : Animal
```

Se o compilador estivesse só imprimindo uma mensagem fixa, ele não conseguiria
mostrar a árvore nem os tipos inferidos de cada expressão.

### Comparação com um programa errado

```sh
./coolc.exe examples/erros_semanticos.cl; echo "exit=$?"
```

Deve sair código `1` e mensagens com linha. Com trace:

```sh
./coolc.exe --trace examples/erros_semanticos.cl
```

As features problemáticas aparecem como `ERRO`.

---

## 8. Comandos rápidos

Compilar:

```sh
mingw32-make
```

Rodar o exemplo mínimo:

```sh
mingw32-make run
```

Rodar a suíte completa:

```sh
mingw32-make tests
```

Rodar com detalhes:

```sh
python tests/run_tests.py --verbose
```

Rodar só semântica:

```sh
python tests/run_tests.py --filter sem_
```

Rodar contra o binário já existente:

```sh
python tests/run_tests.py --compiler ./coolc.exe
```

Inspecionar manualmente:

```sh
./coolc.exe --tokens examples/completo.cl
./coolc.exe --ast examples/completo.cl
./coolc.exe --trace examples/completo.cl
```

---

## 9. Observação sobre execução do programa COOL

O `coolc` atual valida o programa, mas não gera código executável nem interpreta
as expressões. Portanto, rodar:

```sh
./coolc.exe examples/hello.cl
```

não imprime `Hello World`: ele apenas confirma que o programa passou nas análises
léxica, sintática e semântica.

Para ver `Hello World` de verdade seria necessário adicionar uma etapa de geração
de código ou um interpretador para a AST. Isso não faz parte da suíte atual.

#!/usr/bin/env python3
"""Runner de testes do compilador COOL.

Uso:
    python tests/run_tests.py
    python tests/run_tests.py --filter sem_
    python tests/run_tests.py --compiler ./coolc.exe
    python tests/run_tests.py --verbose

Por padrão o runner compila os fontes de src/ com g++ e roda a suíte em
tests/valid/ e tests/invalid/. Os arquivos de teste podem declarar
expectativas em comentários COOL:

    -- @expect exit 1
    -- @expect contains texto que deve aparecer na saída
    -- @expect not_contains texto que não pode aparecer

Os comentários de expectativa são ignorados pelo compilador.
"""

from __future__ import annotations

import argparse
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
TESTS = Path(__file__).resolve().parent
BUILD = TESTS / "build"
DIRECTIVE_RE = re.compile(r"^\s*--\s*@expect\s+(\S+)\s*(.*)$")


class TestFailure(Exception):
    pass


def parse_directives(path: Path) -> dict:
    directives = {"contains": [], "not_contains": []}
    text = path.read_text(encoding="utf-8", errors="replace")
    for line in text.splitlines():
        match = DIRECTIVE_RE.match(line)
        if not match:
            continue
        key = match.group(1).strip()
        value = match.group(2).strip()
        if key == "exit":
            try:
                directives["exit"] = int(value)
            except ValueError:
                raise TestFailure(f"diretiva exit inválida em {path}: {value!r}")
        elif key in ("contains", "not_contains"):
            directives[key].append(value)
        else:
            raise TestFailure(f"diretiva desconhecida em {path}: {key!r}")
    return directives


def decode(data: bytes) -> str:
    return data.decode("utf-8", errors="replace")


def run_process(command, cwd: Path, env: dict, timeout: float = 25.0):
    return subprocess.run(
        [str(part) for part in command],
        cwd=str(cwd),
        env=env,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        timeout=timeout,
    )


def build_compiler(cxx: str) -> Path:
    BUILD.mkdir(parents=True, exist_ok=True)
    exe_name = "coolc_test.exe" if os.name == "nt" else "coolc_test"
    exe = BUILD / exe_name

    cxx_path = shutil.which(cxx)
    if not cxx_path:
        raise TestFailure(f"compilador C++ não encontrado: {cxx}")

    sources = sorted((ROOT / "src").glob("*.cpp"))
    if not sources:
        raise TestFailure("nenhum fonte em src/")

    command = [
        cxx_path,
        "-std=c++17",
        "-Wall",
        "-Wextra",
        "-O2",
        "-I",
        str(ROOT / "include"),
        *[str(source) for source in sources],
        "-o",
        str(exe),
    ]
    # No Windows/MinGW, linkar a runtime estaticamente evita que o executável
    # carregue uma libstdc++ incompatível de outro diretório no PATH.
    if os.name == "nt":
        command += ["-static-libstdc++", "-static-libgcc"]

    print(f"Compilando: {' '.join(command)}")
    result = subprocess.run(
        command,
        cwd=str(ROOT),
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    stdout = decode(result.stdout)
    stderr = decode(result.stderr)
    if result.returncode != 0:
        if stdout:
            print(stdout)
        if stderr:
            print(stderr, file=sys.stderr)
        raise TestFailure("falha ao compilar o compilador")

    warnings = [line for line in stderr.splitlines() if "warning:" in line]
    if warnings:
        print("Avisos do compilador C++:")
        for line in warnings:
            print("  " + line)

    return exe


def resolve_compiler(value: str) -> Path:
    path = Path(value)
    if path.exists():
        return path.resolve()
    if os.name == "nt":
        for candidate in (Path(value + ".exe"), Path(value).with_suffix(".exe")):
            if candidate.exists():
                return candidate.resolve()
    found = shutil.which(value)
    if found:
        return Path(found).resolve()
    raise TestFailure(f"executável do compilador não encontrado: {value}")


def make_env(compiler: Path) -> dict:
    env = os.environ.copy()
    # Ajuda no Windows quando o compilador foi linkado dinamicamente.
    env["PATH"] = str(compiler.parent) + os.pathsep + env.get("PATH", "")
    return env


def check_output(
    name: str,
    output: str,
    returncode: int,
    expected_exit: int,
    contains: list,
    not_contains: list,
) -> None:
    problems = []
    if returncode != expected_exit:
        problems.append(f"código de saída {returncode}, esperado {expected_exit}")
    if returncode < 0:
        problems.append("o processo terminou por sinal (possível segfault)")
    for text in contains:
        if text not in output:
            problems.append(f"saída não contém: {text!r}")
    for text in not_contains:
        if text in output:
            problems.append(f"saída contém o texto proibido: {text!r}")
    if problems:
        raise TestFailure("; ".join(problems))


def run_cli_test(compiler: Path, args: list, expected_exit: int, contains: list,
                 env: dict, label: str) -> None:
    proc = run_process([compiler, *args], ROOT, env)
    output = decode(proc.stdout)
    try:
        check_output(label, output, proc.returncode, expected_exit, contains, [])
    except TestFailure as exc:
        raise TestFailure(f"{exc}\n--- saída ---\n{output}")


def run_file_test(compiler: Path, path: Path, env: dict, verbose: bool) -> bool:
    name = path.relative_to(TESTS).as_posix()
    directives = parse_directives(path)
    if "exit" not in directives:
        directives["exit"] = 0 if path.parent.name == "valid" else 1

    contains = list(directives["contains"])
    not_contains = list(directives["not_contains"])

    if path.parent.name == "valid":
        contains.append("Análise semântica concluída com sucesso")
    else:
        if not contains:
            contains.append("Erro ")

    try:
        proc = run_process([compiler, path], ROOT, env)
        output = decode(proc.stdout)
        check_output(name, output, proc.returncode, directives["exit"], contains, not_contains)
    except subprocess.TimeoutExpired:
        print(f"FAIL {name}: timeout")
        return False
    except TestFailure as exc:
        print(f"FAIL {name}: {exc}")
        if "--- saída ---" in str(exc):
            pass
        else:
            print("--- saída ---")
            print(output if 'output' in locals() else "(sem saída)")
        return False

    if verbose:
        print(f"PASS {name}")
        print(output)

    # Testa também a impressão da AST nos programas válidos.
    if path.parent.name == "valid":
        try:
            proc = run_process([compiler, "--ast", path], ROOT, env)
            output = decode(proc.stdout)
            check_output(f"{name} (--ast)", output, proc.returncode, 0,
                         ["Program", "Class("], [])
            if "--tokens" in directives.get("flags", []):
                pass
        except TestFailure as exc:
            print(f"FAIL {name} (--ast): {exc}")
            print("--- saída ---")
            print(output if 'output' in locals() else "(sem saída)")
            return False

    if not verbose:
        print(f"PASS {name}")
    return True


def run_generated_test(compiler: Path, env: dict, name: str, content: bytes,
                       expected_exit: int, contains: list, not_contains: list = ()) -> bool:
    BUILD.mkdir(parents=True, exist_ok=True)
    path = BUILD / name
    path.write_bytes(content)
    try:
        proc = run_process([compiler, path], ROOT, env, timeout=40.0)
        output = decode(proc.stdout)
        check_output(name, output, proc.returncode, expected_exit, contains, list(not_contains))
        print(f"PASS {name}")
        return True
    except subprocess.TimeoutExpired:
        print(f"FAIL {name}: timeout")
        return False
    except TestFailure as exc:
        print(f"FAIL {name}: {exc}")
        print("--- saída ---")
        print(output if 'output' in locals() else "(sem saída)")
        return False


def run_programmatic_tests(compiler: Path, env: dict, verbose: bool) -> tuple:
    passed = 0
    failed = 0

    cases = []

    cases.append((
        "gen_nul_in_string.cl",
        b'class Main { main() : Object { "a\x00b" } };\n',
        1,
        ["String contains null character"],
    ))
    cases.append((
        "gen_nul_in_code.cl",
        b'class Main { main() : Object { 0\x00 } };\n',
        1,
        ["Caractere nulo"],
    ))
    cases.append((
        "gen_string_1024.cl",
        b'class Main { main() : Object { "' + b"a" * 1024 + b'" } ; };\n',
        0,
        ["Análise semântica concluída com sucesso"],
    ))
    cases.append((
        "gen_string_1025.cl",
        b'class Main { main() : Object { "' + b"a" * 1025 + b'" } ; };\n',
        1,
        ["String constant too long"],
    ))
    cases.append((
        "gen_many_line_comments.cl",
        b"-- comentario\n" * 5000 + b"class Main { main() : Object { 0 } ; };\n",
        0,
        ["Análise semântica concluída com sucesso"],
    ))
    cases.append((
        "gen_deep_nested_comments.cl",
        b"(*" * 1000 + b"*)" * 1000 + b"class Main { main() : Object { 0 } ; };\n",
        0,
        ["Análise semântica concluída com sucesso"],
    ))

    for name, content, expected_exit, contains in cases:
        if run_generated_test(compiler, env, name, content, expected_exit, contains):
            passed += 1
        else:
            failed += 1

    # Vários erros léxicos no mesmo programa devem ser todos reportados.
    name = "gen_multiple_lex_errors.cl"
    content = b"class Main { main() : Object { { 1 # 2; 3 $ 4; } } ; };\n"
    path = BUILD / name
    path.write_bytes(content)
    try:
        proc = run_process([compiler, path], ROOT, env)
        output = decode(proc.stdout)
        count = output.count("Erro léxico")
        if proc.returncode != 1 or count < 2:
            raise TestFailure(
                f"código {proc.returncode}, {count} erros léxicos (esperado >= 2)"
            )
        print(f"PASS {name}")
        passed += 1
        if verbose:
            print(output)
    except TestFailure as exc:
        print(f"FAIL {name}: {exc}")
        print("--- saída ---")
        print(output if 'output' in locals() else "(sem saída)")
        failed += 1

    # A saída semântica deve ser determinística entre execuções.
    det_path = TESTS / "invalid" / "sem_deterministic.cl"
    try:
        first = run_process([compiler, det_path], ROOT, env)
        second = run_process([compiler, det_path], ROOT, env)
        if decode(first.stdout) != decode(second.stdout):
            raise TestFailure("duas execuções produziram saídas diferentes")
        print("PASS gen_deterministic_output")
        passed += 1
    except TestFailure as exc:
        print(f"FAIL gen_deterministic_output: {exc}")
        failed += 1
    except subprocess.TimeoutExpired:
        print("FAIL gen_deterministic_output: timeout")
        failed += 1

    return passed, failed


def main() -> int:
    parser = argparse.ArgumentParser(description="Roda a suíte de testes do coolc")
    parser.add_argument("--compiler", help="executável já compilado (não recompila)")
    parser.add_argument("--cxx", default=os.environ.get("CXX", "g++"),
                        help="compilador C++ para build (padrão: g++)")
    parser.add_argument("--filter", default="", help="roda só testes cujo nome contém")
    parser.add_argument("--verbose", "-v", action="store_true", help="mostra a saída de cada teste")
    args = parser.parse_args()

    try:
        if args.compiler:
            compiler = resolve_compiler(args.compiler)
            print(f"Usando compilador: {compiler}")
        else:
            compiler = build_compiler(args.cxx)
            print(f"Compilador de teste: {compiler}")
    except TestFailure as exc:
        print(f"ERRO: {exc}", file=sys.stderr)
        return 1

    env = make_env(compiler)

    files = sorted((TESTS / "valid").glob("*.cl")) + sorted((TESTS / "invalid").glob("*.cl"))
    if args.filter:
        files = [path for path in files if args.filter in path.relative_to(TESTS).as_posix()]

    passed = 0
    failed = 0

    for path in files:
        if run_file_test(compiler, path, env, args.verbose):
            passed += 1
        else:
            failed += 1

    # Testes de CLI (independentes dos arquivos).
    cli_cases = [
        (["--help"], 0, ["Uso:"], "cli_help"),
        ([], 1, ["Nenhum arquivo"], "cli_no_args"),
        (["--opcao-que-nao-existe"], 1, ["Opção desconhecida"], "cli_unknown_option"),
        (["nao_existe_de_verdade.cl"], 1, ["Erro ao abrir"], "cli_missing_file"),
    ]
    for cli_args, expected, contains, label in cli_cases:
        try:
            run_cli_test(compiler, cli_args, expected, contains, env, label)
            print(f"PASS {label}")
            passed += 1
        except TestFailure as exc:
            print(f"FAIL {label}: {exc}")
            failed += 1

    # --tokens: um caso válido e um caso com erro léxico.
    try:
        run_cli_test(compiler, ["--tokens", TESTS / "valid" / "hello.cl"], 0,
                     ["Tokens Extraídos", "END_OF_FILE"], env, "cli_tokens_valid")
        print("PASS cli_tokens_valid")
        passed += 1
    except TestFailure as exc:
        print(f"FAIL cli_tokens_valid: {exc}")
        failed += 1

    try:
        run_cli_test(compiler, ["--tokens", TESTS / "invalid" / "lex_gt.cl"], 1,
                     ["ERROR", "Operador '>'"], env, "cli_tokens_error")
        print("PASS cli_tokens_error")
        passed += 1
    except TestFailure as exc:
        print(f"FAIL cli_tokens_error: {exc}")
        failed += 1

    # --trace: mostra a tabela de símbolos e os tipos inferidos.
    try:
        run_cli_test(compiler, ["--trace", TESTS / "valid" / "hello.cl"], 0,
                     ["[trace]", "[metodo] main", "Resumo semântico"], env, "cli_trace")
        print("PASS cli_trace")
        passed += 1
    except TestFailure as exc:
        print(f"FAIL cli_trace: {exc}")
        failed += 1

    # Testes gerados em memória (NUL, strings longas, muitos comentários...).
    gen_passed, gen_failed = run_programmatic_tests(compiler, env, args.verbose)
    passed += gen_passed
    failed += gen_failed

    print()
    print(f"Resumo: {passed} passaram, {failed} falharam")
    return 0 if failed == 0 else 1


if __name__ == "__main__":
    sys.exit(main())

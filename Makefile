CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O2
CPPFLAGS := -Iinclude
PYTHON   ?= python

# No Windows/MinGW o g++ gera coolc.exe; o target precisa refletir isso para
# que 'make run' e 'make test' funcionem.
ifeq ($(OS),Windows_NT)
EXEEXT  := .exe
# Evita carregar uma libstdc++ de outro diretório no PATH.
LDFLAGS += -static-libstdc++ -static-libgcc
else
EXEEXT  :=
endif
TARGET := coolc$(EXEEXT)

SRC := src/Lexer.cpp src/Parser.cpp src/SemanticAnalyzer.cpp src/main.cpp
OBJ := $(SRC:.cpp=.o)

.PHONY: all clean run test tests

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $(OBJ)

src/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

# Dependências de cabeçalho (sem gerador automático, simples e suficiente)
$(OBJ): include/Token.hpp include/Lexer.hpp include/AST.hpp include/Parser.hpp \
        include/SemanticAnalyzer.hpp

run: $(TARGET)
	./$(TARGET) examples/hello.cl

# Suíte completa: compila o compilador em tests/build/ e roda todos os casos.
tests:
	$(PYTHON) tests/run_tests.py

test: tests

clean:
	rm -f $(OBJ) $(TARGET) coolc coolc.exe

-- @expect exit 1
-- @expect contains Erro léxico
-- @expect contains Operador '>' nao existe em COOL
class Main { main() : Object { 1 > 0 }; };

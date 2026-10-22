-- @expect exit 1
-- @expect contains Esperado identificador de variável em 'let'
class Main { main() : Object { let in 0 }; };

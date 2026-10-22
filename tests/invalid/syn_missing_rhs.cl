-- @expect exit 1
-- @expect contains Expressão inválida
class Main { main() : Object { { x <- ; } }; };

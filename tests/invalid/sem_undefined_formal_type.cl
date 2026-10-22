class Main { f(x : NaoExiste) : Object { x }; main() : Object { 0 }; };
-- @expect exit 1
-- @expect contains Parâmetro 'x' no método 'f' possui tipo não declarado 'NaoExiste'

class Main { f() : NaoExiste { 0 }; main() : Object { 0 }; };
-- @expect exit 1
-- @expect contains possui tipo de retorno não declarado 'NaoExiste'

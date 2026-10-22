class Main { x : NaoExiste; main() : Object { 0 }; };
-- @expect exit 1
-- @expect contains Atributo 'x' na classe 'Main' possui tipo não declarado 'NaoExiste'

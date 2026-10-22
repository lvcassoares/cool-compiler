class Main { main() : Object { new NaoExiste }; };
-- @expect exit 1
-- @expect contains Instanciação 'new NaoExiste' de classe não declarada

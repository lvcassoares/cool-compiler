class Main { main() : Object { (new Main).foo() }; };
-- @expect exit 1
-- @expect contains Método 'foo' não encontrado na classe 'Main'

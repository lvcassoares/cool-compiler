class Parent { main() : Object { 0 }; };
class Main inherits Parent { };
-- @expect exit 1
-- @expect contains Método 'main' não foi encontrado

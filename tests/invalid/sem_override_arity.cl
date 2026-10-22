class A { f(x : Int) : Int { x }; };
class B inherits A { f() : Int { 0 }; };
class Main { main() : Object { 0 }; };
-- @expect exit 1
-- @expect contains altera o número de parâmetros

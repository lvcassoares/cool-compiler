class A { f(x : Int) : Int { x }; };
class Main { main() : Object { (new A).f() }; };
-- @expect exit 1
-- @expect contains Número incorreto de argumentos

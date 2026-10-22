class A { f() : Int { 1 }; };
class B inherits A { f() : String { "x" }; };
class Main { main() : Object { 0 }; };
-- @expect exit 1
-- @expect contains altera o tipo de retorno

class A { f(x : Int) : Int { x }; };
class B inherits A { f(x : String) : Int { 0 }; };
class Main { main() : Object { 0 }; };
-- @expect exit 1
-- @expect contains altera o tipo do parâmetro

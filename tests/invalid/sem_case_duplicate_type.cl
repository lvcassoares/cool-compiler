class A { };
class Main { main() : Object { case new A of x : A => 1; y : A => 2; esac }; };
-- @expect exit 1
-- @expect contains Tipo 'A' duplicado

class A { f() : Int { 1 }; };
class B { };
class Main { main() : Object { (new B)@A.f() }; };
-- @expect exit 1
-- @expect contains Expressão do dispatch estático não é do tipo 'A'

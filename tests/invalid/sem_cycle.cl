class A inherits B { };
class B inherits A { };
class Main { main() : Object { 0 }; };
-- @expect exit 1
-- @expect contains Ciclo de herança

class A { x : Int; };
class B inherits A { x : Int; };
class Main { main() : Object { 0 }; };
-- @expect exit 1
-- @expect contains redefine um atributo herdado

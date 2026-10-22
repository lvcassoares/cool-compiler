class A { f() : Int { 1 }; };
class Main { main() : Object { (new A)@NaoExiste.f() }; };
-- @expect exit 1
-- @expect contains Classe 'NaoExiste' não declarada no dispatch estático

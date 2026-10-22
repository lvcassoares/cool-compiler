class A inherits NaoExiste { };
class Main { main() : Object { 0 }; };
-- @expect exit 1
-- @expect contains herda de classe não declarada NaoExiste

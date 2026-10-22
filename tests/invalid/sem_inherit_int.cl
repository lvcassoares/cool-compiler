class A inherits Int { };
class Main { main() : Object { 0 }; };
-- @expect exit 1
-- @expect contains não pode herdar de Int

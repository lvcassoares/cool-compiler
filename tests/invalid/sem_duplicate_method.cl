class Main { f() : Int { 1 }; f() : Int { 2 }; main() : Object { 0 }; };
-- @expect exit 1
-- @expect contains Método 'f' redefinido

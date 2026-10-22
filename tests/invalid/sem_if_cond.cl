class Main { main() : Object { if 1 then 2 else 3 fi }; };
-- @expect exit 1
-- @expect contains Condição da instrução 'if' deve ser do tipo 'Bool'

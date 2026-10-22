class Main { main() : Object { while "x" loop 0 pool }; };
-- @expect exit 1
-- @expect contains Condição do laço 'while' deve ser do tipo 'Bool'

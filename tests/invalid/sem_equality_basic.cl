class Main { main() : Object { if 1 = "x" then 1 else 2 fi }; };
-- @expect exit 1
-- @expect contains Comparação de igualdade '=' entre tipos básicos distintos

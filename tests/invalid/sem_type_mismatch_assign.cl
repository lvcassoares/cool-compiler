class Main { x : Int; main() : Object { x <- "s" }; };
-- @expect exit 1
-- @expect contains Tipo incompatível na atribuição de 'x'

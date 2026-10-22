class Main {
    z : Int <- "a";
    y : Int <- "b";
    x : Int <- "c";
    main() : Object { 0 };
};
-- @expect exit 1
-- @expect contains Erro semântico na linha 2
-- @expect contains Erro semântico na linha 3
-- @expect contains Erro semântico na linha 4

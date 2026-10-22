class Main {
    x : Int <- "wrong";
    main() : Object {
        { y; }
    };
};
-- @expect exit 1
-- @expect contains Erro semântico na linha 2
-- @expect contains Erro semântico na linha 4

-- @expect exit 1
-- @expect contains Erro léxico
-- @expect contains Caractere inválido '#'
class Main {
    main() : Object {
        {
            x <- 1 # 2;
        }
    };
};

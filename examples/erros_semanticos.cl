-- Erros SEMÂNTICOS: a sintaxe esta correta, os tipos nao.
class Main {
    x : Int <- "texto";      -- inicializacao com tipo errado
    y : NaoExiste;           -- tipo nao declarado

    main() : Object {
        {
            z <- 1;              -- variavel nao declarada
            x <- "de novo";      -- atribuicao com tipo errado
            if 42 then 1 else 2 fi;   -- condicao nao e Bool
            while 1 loop 0 pool;      -- condicao nao e Bool
        }
    };
};

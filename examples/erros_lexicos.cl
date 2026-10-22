-- Erros LÉXICOS: o programa nunca chega ao parser.
class Main {
    main() : Object {
        {
            x <- 1 # 2;    -- '#' nao faz parte da linguagem
            y <- 1;
        }
    };
};

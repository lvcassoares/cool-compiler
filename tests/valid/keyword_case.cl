-- Palavras-chave são case-insensitive (§10.4); só true/false têm restrição
-- na primeira letra, e self/SELF_TYPE são identificadores especiais.
Class Main Inherits IO {
    main() : Object {
        LeT x : Int <- 1 In
            IF x = 1 ThEn
                { out_string("ok\n"); NeW Main; }
            ELSE out_string("bad\n") Fi
    };
};

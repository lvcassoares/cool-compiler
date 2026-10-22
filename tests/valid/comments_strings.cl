-- comentario de linha
(* bloco (* aninhado *) fim *)
class Main inherits IO {
    s : String <- "quote:\" slash:\\ tab:\t zero:\0";

    main() : Object {
        {
            out_string(s);
            out_string("\n");
        }
    };
};

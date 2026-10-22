-- Exemplo que exercita a maior parte da gramática de COOL.
(* Comentários de bloco também existem e podem aninhar:
   (* assim *)
   e é isso.
*)

class Animal {
    nome : String <- "bicho";
    idade : Int <- 0;

    init(n : String, i : Int) : Animal {
        {
            nome <- n;
            idade <- i;
            self;
        }
    };

    fala() : String { "..." };
    getNome() : String { nome };
    getIdade() : Int { idade };
};

class Cachorro inherits Animal {
    -- override com a MESMA assinatura: legal em COOL
    fala() : String { "Woof!" };
};

class Gato inherits Animal {
    fala() : String { "Miau" };
    getIdade() : Int { idade + 1 };
};

class Main inherits IO {
    animal_favorito : Animal <- new Gato;
    contador : Int <- 0;

    main() : Object {
        {
            animal_favorito <- (new Cachorro).init("Rex", 3);
            out_string(animal_favorito.getNome());
            out_string(" diz ");
            out_string(animal_favorito.fala());
            out_string("\n");

            -- while + if + atribuição
            contador <- 0;
            while contador < 3 loop
                {
                    out_int(contador);
                    out_string(" ");
                    contador <- contador + 1;
                }
            pool;
            out_string("\n");

            -- let + case
            let bicho : Animal <- new Gato in
                case bicho of
                    c : Cachorro => out_string("e um cachorro\n");
                    g : Gato     => out_string("e um gato\n");
                    a : Animal   => out_string("e outro bicho\n");
                esac;

            -- dispatch estatico: forca a versao de Animal (ignora o override de Gato)
            out_int(animal_favorito@Animal.getIdade());
            out_string("\n");

            -- isvoid, not, ~ e comparação
            if isvoid animal_favorito then out_string("void\n") else out_string("nao void\n") fi;
            if not (3 <= 2) then out_int(~7) else out_int(0) fi;
            out_string("\n");
        }
    };
};

-- SELF_TYPE nos quatro usos legais: new, retorno de metodo, let e atributo.
class Silly {
    copy() : SELF_TYPE { self };
    maybe(b : Bool) : SELF_TYPE { if b then self else self fi };
    make() : SELF_TYPE { new SELF_TYPE };
};

class Sally inherits Silly { };

class Main {
    x : Sally <- (new Sally).copy();
    y : SELF_TYPE <- self;

    main() : SELF_TYPE {
        let z : SELF_TYPE <- self in
            case z of
                m : Main => if true then self else self fi;
            esac
    };
};

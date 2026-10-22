class A { x : Int <- 1; };
class B inherits A { y : Int <- 2; };

class Main inherits IO {
    a : Int <- 10;
    b : Int <- a + 1;

    main() : Object {
        {
            let a : Int <- a + 1, a : Int <- a + 1 in out_int(a);
            out_string("\n");
            let o : A <- new B in
                case o of
                    b : B => out_int(2);
                    a2 : A => out_int(1);
                esac;
            out_string("\n");
        }
    };
};

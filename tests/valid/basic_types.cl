class Main inherits IO {
    x : Int <- 2 + 3 * 4;
    s : String <- "a\tb\n\"c\"\\d\0";
    b : Bool <- true;

    main() : Object {
        {
            out_int(x);
            out_string("\n");
            if not (x = 14) then out_string("bad\n") else out_string("ok\n") fi;
            while x < 20 loop x <- x + 1 pool;
            x <- ~x;
            if isvoid s then out_string("void\n") else out_string(s) fi;
            out_string("\n");
            b;
        }
    };
};

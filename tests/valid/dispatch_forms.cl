class Foo {
    value() : Int { 1 };
    add(x : Int) : Int { value() + x };
};

class Main inherits IO {
    helper(x : Int) : Int { x + 1 };

    main() : Object {
        {
            out_int((new Foo).value());
            out_int(new Foo.add(2));
            out_int((new Foo).add(3));
            out_int(helper(4));
            out_int((new Foo).value() + helper(1));
            out_string("\n");
        }
    };
};

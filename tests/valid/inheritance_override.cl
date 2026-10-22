class Animal {
    name : String;
    init(n : String) : Animal { { name <- n; self; } };
    speak() : String { "..." };
};

class Dog inherits Animal {
    speak() : String { "Woof" };
};

class Main inherits IO {
    a : Animal <- new Dog;

    main() : Object {
        {
            a <- (new Dog).init("Rex");
            out_string(a.speak());
            out_string("\n");
            out_string(a@Animal.speak());
            out_string("\n");
        }
    };
};

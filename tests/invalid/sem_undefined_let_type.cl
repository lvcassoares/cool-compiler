class Main {
    main() : Object {
        let x : NaoExiste in x.foo()
    };
};
-- @expect exit 1
-- @expect contains Tipo não declarado 'NaoExiste' no 'let'
-- @expect not_contains Segmentation fault

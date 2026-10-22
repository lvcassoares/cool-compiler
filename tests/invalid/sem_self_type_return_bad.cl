class Main {
    f() : SELF_TYPE { new Main };
    main() : Object { self };
};
-- @expect exit 1
-- @expect contains incompatível com o tipo de retorno declarado 'SELF_TYPE'

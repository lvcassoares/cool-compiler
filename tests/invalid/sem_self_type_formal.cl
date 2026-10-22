class Main {
    f(x : SELF_TYPE) : Object { x };
    main() : Object { self };
};
-- @expect exit 1
-- @expect contains não pode ser do tipo 'SELF_TYPE'

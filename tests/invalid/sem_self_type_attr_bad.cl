class Main {
    x : SELF_TYPE <- new Main;
    main() : Object { self };
};
-- @expect exit 1
-- @expect contains incompatível com o tipo do atributo 'x' (SELF_TYPE)

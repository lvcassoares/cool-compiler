class Main {
    main() : Object {
        case self of
            x : SELF_TYPE => x;
        esac
    };
};
-- @expect exit 1
-- @expect contains não pode ser usado em uma ramificação de 'case'

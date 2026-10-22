class Main { main() : Object { case new Main of x : NaoExiste => 1; esac }; };
-- @expect exit 1
-- @expect contains Tipo não declarado 'NaoExiste' na ramificação do 'case'

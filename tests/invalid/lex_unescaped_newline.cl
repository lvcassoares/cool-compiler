-- @expect exit 1
-- @expect contains Unescaped newline in string constant
class Main { main() : Object { "abc
def" } };

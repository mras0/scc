void main()
{
    int x = 42;
    if (0) {
        char* s = "test!";
        ++s;
        *s = 42;
        memset(s, 0, 4);
    }
    assert_eq(x, 42);
}

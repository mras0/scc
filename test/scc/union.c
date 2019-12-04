union U {
    char c;
    int i;
    struct S {
        int a;
        int b;
    } s;
} gu;

union Empty {
};

void main()
{
    assert_eq(sizeof(union Empty), 0);
    assert_eq(sizeof(union U), 4);
    assert_eq((char*)&gu.c, (char*)&gu);
    assert_eq((char*)&gu.i, (char*)&gu);
    assert_eq((char*)&gu.s, (char*)&gu);
    assert_eq((char*)&gu.s.a, (char*)&gu);
    assert_eq((char*)&gu.s.b, (char*)&gu + 2);
    gu.s.b = -2;
    gu.i = 42;
    assert_eq(gu.c, 42);
    assert_eq(gu.s.a, 42);
    assert_eq(gu.s.b, -2);

    union U u;
    u = gu;
    assert_eq(u.s.a, 42);
    assert_eq(u.s.b, -2);

    u.s.b = -3;

    union U* up = &u;
    assert_eq(up->s.b, -3);
}

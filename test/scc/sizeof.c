void main()
{
    assert_eq(sizeof(char), 1);
    assert_eq(sizeof(int), 2);
    assert_eq(sizeof(char*), 2);
    assert_eq(sizeof(int*), 2);
    assert_eq(sizeof(int***), 2);
    struct S {
        int a;
        int b;
    };
    assert_eq(sizeof(struct S), 4);
    assert_eq(sizeof(struct S*), 2);
    union U {
        struct S s;
        int x;
    };
    assert_eq(sizeof(union U), 4);

    char c;
    int i;
    struct S s;
    union U u;
    assert_eq(sizeof(c), 1);
    assert_eq(sizeof(i), 2);
    assert_eq(sizeof(s), 4);
    assert_eq(sizeof(u), 4);
    assert_eq(sizeof(1+1), 2);

    struct S2 {
        struct {
            int a;
            int b;
            int c;
        } x;
        int y;
    } s2;
    assert_eq(sizeof(s2), 8);
    assert_eq(sizeof(s2.x), 6);
    assert_eq(sizeof(s2.y), 2);

    assert_eq(sizeof 1+1, 3);

    assert_eq(sizeof("test"), 5);
    assert_eq(1, 1);

    int x=1;
    int a[4];
    x += sizeof(a[x]);
    assert_eq(x, 3);
}

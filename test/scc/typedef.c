int t00_f(int x) { return x + 1; }

void t00() {
    unsigned char typedef uc;
    assert_eq(sizeof(uc), 1);
    uc a = -1;
    assert_eq(a, 0xff);
    typedef int i;
    assert_eq(sizeof(i), 2);
    i b = -42;
    assert_eq(b, -42);
    unsigned i c = -1;
    assert_eq(sizeof(c), 2);
    assert_eq(c, 0xffff);
    assert_eq(c>0, 1);

    typedef uc uc2;
    uc2 d = -2;
    assert_eq(d, 0xfe);
    assert_eq(sizeof(d), 1);
    assert_eq(sizeof(uc2), 1);

    typedef struct {
        int x, y;
    } S;
    S s;
    s.x = 1;
    s.y = 2;
    assert_eq(s.x, 1);
    assert_eq(s.y, 2);
    assert_eq(sizeof(S), 4);

    typedef S* SP;
    assert_eq(sizeof(SP), 2);
    SP sp = &s;
    assert_eq(sp->x, 1);
    assert_eq(sp->y, 2);

    typedef int (*fptr)(int);
    fptr f = &t00_f;
    assert_eq(f(2), 3);
}

void main() {
    t00();
}

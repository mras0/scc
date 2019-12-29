int f(int i) { return i-1; }

int g;

int glob = 1+1-2 ? 3*4 : 5*6;

int x(int i) { g = i; return i; }

void main()
{
    assert_eq(glob, 30);
    assert_eq(2 ? x(42) : x(43), 42);
    assert_eq(g, 42);
    char c = 42;
    int i = 33;
    assert_eq(f(1) * 4 ? f(i) : c, 42);
    assert_eq(f(2) * 4 ? f(i) : c, 32);
    i = 0 ? 1 : 2 ? 3 : 4;
    assert_eq(i, 3);

    int a = 0;
    int b = 0;
    *(1-1?&a:&b) = 42;
    assert_eq(a, 0);
    assert_eq(b, 42);
    {
        int i = 2;
        char c = 3;
        int x = 0;
        assert_eq(x ? c : i, 2);
    }
    {
        int i = 2;
        char c = 3;
        int x = 42;
        assert_eq(x ? c : i, 3);
    }
    int xx = 42;
    assert_eq(xx ? xx : -1, 42);
    assert_eq(!xx ? xx : -1, -1);

    const char* a = "AB";
    const char* b = a ? a : "XY";
    assert_eq(b[0], 'A');
    assert_eq(b[1], 'B');

    const char* c = 0;
    const char* d = c ? c : "XY";
    assert_eq(d[0], 'X');
    assert_eq(d[1], 'Y');

    int m1 = -1;
    int zr = 0;
    int p1 = 1;
    assert_eq(m1 <  0, 1);
    assert_eq(zr <  0, 0);
    assert_eq(p1 <  0, 0);
    assert_eq(m1 <= 0, 1);
    assert_eq(zr <= 0, 1);
    assert_eq(p1 <= 0, 0);
    assert_eq(m1 == 0, 0);
    assert_eq(zr == 0, 1);
    assert_eq(p1 == 0, 0);
    assert_eq(m1 != 0, 1);
    assert_eq(zr != 0, 0);
    assert_eq(p1 != 0, 1);
    assert_eq(m1 >= 0, 0);
    assert_eq(zr >= 0, 1);
    assert_eq(p1 >= 0, 1);
    assert_eq(m1 >  0, 0);
    assert_eq(zr >  0, 0);
    assert_eq(p1 >  0, 1);

    unsigned u1 = 0x8000;
    assert_eq(u1 <  0, 0);
    assert_eq(u1 <= 0, 0);
    assert_eq(u1 == 0, 0);
    assert_eq(u1 != 0, 1);
    assert_eq(u1 >= 0, 1);
    assert_eq(u1 >  0, 1);
}

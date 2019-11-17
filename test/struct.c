struct S {
    int a;
    int b;
};

struct S* s;

int foo(int x) { return x; }

void bug1() {
    int x = 2;
    s = (struct S*)&_EBSS;
    struct S* ss = &s[foo(x)];
    ss->a = 42;
    ss->b = 60;
    assert_eq(ss, s+2);
    assert_eq(ss-s, 2);
    assert_eq(s[2].a, 42);
    assert_eq(s[2].b, 60);
    struct S* s2 = s;
    assert_eq(++s2, s+1);
    s2++;
    assert_eq(s2, ss);
    assert_eq(--s2, s+1);
    s2--;
    assert_eq(s2, s);
}

void assign() {
    struct S s1;
    s1.a = 1;
    s1.b = 2;
    struct S s2;
    s2 = s1;
    assert_eq(s2.a, 1);
    assert_eq(s2.b, 2);
}

struct S gs;

void global() {
    assert_eq(gs.a, 0);
    assert_eq(gs.b, 0);
    gs.b = 2;
    assert_eq(gs.a, 0);
    assert_eq(gs.b, 2);
    struct S ts;
    ts = gs;
    assert_eq(ts.a, 0);
    assert_eq(ts.b, 2);
    ts.a = 3;
    ts.b = 4;
    gs = ts;
    assert_eq(gs.a, 3);
    assert_eq(gs.b, 4);
}

void main() {
    bug1();
    assign();
    global();
}

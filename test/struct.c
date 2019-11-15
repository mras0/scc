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
    assert_eq(s[2].a, 42);
    assert_eq(s[2].b, 60);
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

void _start() {
    bug1();
    assign();
}

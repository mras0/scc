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

void _start() {
    bug1();
}

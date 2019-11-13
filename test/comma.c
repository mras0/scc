int a() { return 1,2,3; }

int f1() {return 1;}
int f2() {return 2;}
int f3() {return 42;}

int b() { return f1(),f2(),f3(); }

int c() {
    int a = 0;
    int b;
    b = ++a, ++a;
    return b;
}

void _start(void) {
    assert_eq(a(), 3);
    assert_eq(b(), 42);
    assert_eq(c(), 1);
}

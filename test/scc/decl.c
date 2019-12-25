int ga = 1, gb = 2, *gp;

void test1() {
    int q = 42;
    assert_eq(q, 42);
    int a, b = 33, *pi = &a;
    *pi = 42;
    assert_eq(a, 42);
    assert_eq(b, 33);
    assert_eq(ga, 1);
    assert_eq(gb, 2);
    gp = &gb;
    *gp = 42;
    assert_eq(gb, 42);

    struct S {
        char a,b,*pc;
        int *pi, a, **pi;
    } s, s2;
    assert_eq(sizeof(s2), 10);

    assert_eq(no_fwd(42), 43);
}
int no_fwd(int x) { return x+1; }

int t2_s0() {
    static int s;
    return s++;
}

int t2_s1() {
    static int s, s2;
    s = s*2+1;
    s2 = s2*3+7;
    return s+s2;
}

int t2_s2() {
    static int x;
    static char c;
    x++;
    c = (char)x;
    return c;
}

static int t2_s3(int i, int x) {
    static char a[2];
    if (x) ++a[i];
    return a[i];
}

int t2_s4(int* x, int* y, int set) {
    static struct S {
        int x, y;
    } s;
    if (set) {
        s.x=*x;
        s.y=*y;
    } else {
        *x=s.x+1;
        *y=s.y+1;
    }
}

static int a;
static struct T2_1 {
    int x, y;
} t2_1;

void test2() {
    assert_eq(t2_s0(), 0);
    assert_eq(t2_s0(), 1);
    assert_eq(t2_s0(), 2);

    assert_eq(t2_s1(), 8);  // 1 7
    assert_eq(t2_s1(), 31); // 3 28
    assert_eq(t2_s1(), 98); // 7 91

    assert_eq(t2_s2(), 1);
    assert_eq(t2_s2(), 2);
    assert_eq(t2_s2(), 3);

    assert_eq(t2_s3(0, 0), 0);
    assert_eq(t2_s3(1, 0), 0);
    assert_eq(t2_s3(0, 1), 1);
    assert_eq(t2_s3(1, 0), 0);
    assert_eq(t2_s3(1, 1), 1);
    assert_eq(t2_s3(0, 0), 1);

    int x,y;
    t2_s4(&x, &y, 0);
    assert_eq(x, 1);
    assert_eq(y, 1);
    x=2;y=3;
    t2_s4(&x, &y, 1);
    t2_s4(&x, &y, 0);
    assert_eq(x, 3);
    assert_eq(y, 4);

    assert_eq(a, 0);
    a = 42;
    assert_eq(a, 42);

    assert_eq(sizeof(t2_1), 4);
    assert_eq(t2_1.x, 0);
    assert_eq(t2_1.y, 0);
    t2_1.x = 1;
    t2_1.y = 2;
    assert_eq(t2_1.x, 1);
    assert_eq(t2_1.y, 2);
}

void undeclstruct_test() {
    struct Undeclared* x = 0;
    assert_eq(x, 0);
}

void ignoreddecls_test() {
    signed char y;
    y = -17;
    assert_eq(y, -17);
    signed short z = 0x8123;
    assert_eq(z, -32477);
    char x;
    char *px = &x;
    volatile const char * volatile const * volatile const extreme = &px;
    x = 37;
    assert_eq(**extreme, 37);
}

void main() {
    test1();
    test2();
    undeclstruct_test();
    ignoreddecls_test();
}

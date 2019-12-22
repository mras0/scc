int ga = 1, gb = 2, *gp;

void main(void)
{
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

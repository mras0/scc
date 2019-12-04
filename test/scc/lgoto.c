void test1() {
    int x = 0;
    for (;;) {
        if (x > 1) goto elab;
        ++x;
    }
elab:
    assert_eq(x, 2);
    goto blah;
    x = 0;
    goto blah;
    if (0)
        blah: x = 42;
    else
        x = 2;
    assert_eq(x, 42);
}

void test2() {
    int x = 0;

    if (x==0) {
        x=1;
        goto Skip;
    } else {
        goto Foo;
    }

    if (!x) {
        if (~x + 1 == -1) {
        Foo:
            x=2;
            goto Skip;
        } else {
        Skip:
            x=123;
        }
    }
    assert_eq(x, 123);
}

void main() {
    test1();
    test2();
}

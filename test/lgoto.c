void main(void)
{
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

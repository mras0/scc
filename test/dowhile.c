void main(void)
{
    int x = 0;
    do assert_eq(x <= 5, 1); while (x++ != 4);
    assert_eq(x, 5);

    int y = 0;
    x = 0;
    do {
        if (x == 5) break;
        if (x&1) continue;
        y |= 1<<x;
    } while (++x != 10);
    assert_eq(y, 1<<0|1<<2|1<<4);
}

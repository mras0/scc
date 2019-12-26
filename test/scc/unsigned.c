void test_uchar() {
    unsigned char a = -1;
    assert_eq(a, 0xff);
    unsigned char b = 42;
    assert_eq(b, 42);
    a++;
    assert_eq(a, 0);
    --a;
    assert_eq(a, 0xff);

    assert_eq((unsigned char)-2, 0xfe);
    int x = 0x1234;
    assert_eq((unsigned char)x, 0x34);

    b = 0xABCD;
    assert_eq(b, 0xCD);

    b += 0x1234;
    assert_eq(b, 0x01);
}

void main() {
    test_uchar();

    // TODO:
    // - BinOP: Compare/MUL(?)/DIV/MOD/shift
    // - AssignOp
    // - Pointer compare
}

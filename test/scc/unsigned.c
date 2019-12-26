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

void test_uint() {
    unsigned int a = -1;
    assert_eq(a, 0xffff);
    ++a;
    assert_eq(a, 0);
    a--;
    assert_eq(a, 0xffff);
    a = a + 10;
    assert_eq(a, 9);
    a = a / 3;
    assert_eq(a, 3);
    a = a * 2;
    assert_eq(a, 6);
    a = a / 2;
    assert_eq(a, 3);

    unsigned int l = 0x1234;
    unsigned int r = 0xFEDC;

    assert_eq(l+r, 0x1110);
    assert_eq(l-r, 0x1358);
    assert_eq(l*r, 0x3CB0);
    assert_eq(l/r, 0);
    assert_eq(l%r, 0x1234);
    assert_eq(r>>1, 0x7F6E);
    assert_eq(r<<1, 0xFDB8);
    assert_eq(l<r, 1);
    assert_eq(l<=r, 1);
    assert_eq(l>r, 0);
    assert_eq(l>=r, 0);
    assert_eq(l==r, 0);
    assert_eq(l!=r, 1);
    assert_eq(r<l, 0);
    assert_eq(r<=l, 0);
    assert_eq(r>l, 1);
    assert_eq(r>=l, 1);
    assert_eq(r==l, 0);
    assert_eq(r!=l, 1);
}

void test_ptrcmp() {
    assert_eq((int*)0x7FFF <  (int*)0x8000, 1);
    assert_eq((int*)0x7FFF <= (int*)0x8000, 1);
    assert_eq((int*)0x7FFF >  (int*)0x8000, 0);
    assert_eq((int*)0x7FFF >= (int*)0x8000, 0);
}

void main() {
    test_uchar();
    test_uint();
    test_ptrcmp();
}

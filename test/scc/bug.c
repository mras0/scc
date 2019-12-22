
char b00_m[2];
int bug00_r(int i) { return b00_m[i]&0xff; }
void bug00() {
    b00_m[0] = 42;
    b00_m[1] = 9;
    assert_eq(bug00_r(0)|bug00_r(1)<<8, 0x92a);
}

void bug01() {
    int x = 0xAA;
    int y = !!(x & 4) != !!(x & 2);
    assert_eq(y, 1);
    y = !!(x & 4) == !!(x & 2);
    assert_eq(y, 0);
}

void bug02() {
    const int flags = 0x40;
    const int cc = 0x0e;
    enum {
        FC = 1<<0,
        FZ = 1<<6,
        FS = 1<<7,
        FO = 1<<11,
    };

    const int e = !(cc & 1);
    const int j = (flags & FZ) || (!(flags & FS) != !(flags & FO));
    assert_eq(j, 1);
    assert_eq(j==e, 1);
}

void bug03() {
    int x = 1;
    if (x) {
    }
    assert_eq(x, 1);
}

void main() {
    bug00();
    bug01();
    bug02();
    bug03();
}

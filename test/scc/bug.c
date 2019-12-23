
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

int b03_f(int a, int b, int c, int d, int e, int f) {
    return a+b*2+c*3+d*4+e*5+f*6;
}

void bug04() {
    assert_eq(b03_f(2,3,5,7,11,13), 184);
}

void main() {
    bug00();
    bug01();
    bug02();
    bug03();
    bug04();
}

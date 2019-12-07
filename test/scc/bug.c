
char b00_m[2];
int bug00_r(int i) { return b00_m[i]&0xff; }
void bug00() {
    b00_m[0] = 42;
    b00_m[1] = 9;
    assert_eq(bug00_r(0)|bug00_r(1)<<8, 0x92a);
}

void main() {
    bug00();
}

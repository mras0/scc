void memcpy(char* d, const char* s, int n)
{
    while (n--) *d++ = *s++;
}

void putchar(int ch)
{
    _emit 0x8B _emit 0x46|2<<3 _emit 0x04 // MOV DX, [BP+4]
    _emit 0xB8 _emit 0x00      _emit 0x02 // MOV AX, 0x0200
    _emit 0xCD _emit 0x21                 // INT 0x21
}

void putstr(const char* s)
{
    while (*s) putchar(*s++);
}

void putn(int n) {
    int i;
    for (i = 0; i < 4; ++i) {
        putchar("0123456789abcdef"[(n>>12)&0xf]);
        n <<= 4;
    }
}

void assert_eq(int a, int b) {
    if (a == b) return;
    putn(a); putstr(" != "); putn(b); putstr("\n");
    _emit 0xcc                       // INT3
    _emit 0xB8 _emit 0xFF _emit 0x4C // MOV AX, 0x4CFF
    _emit 0xcd _emit 0x21            // INT 0x21
}

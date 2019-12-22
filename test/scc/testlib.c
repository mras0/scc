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

int* GetBP(void)
{
    _emit 0x8B _emit 0x46 _emit 0x00 // MOV AX, [BP]
}

void assert_eq(int a, int b) {
    if (a == b) return;
    int* BP = GetBP();
    putstr("\nBP   Return address\n");
    int i = 0;
    while (*BP && ++i < 10) {
        putn(BP[0]); putstr(" "); putn(BP[1]); putstr("\n");
        BP = (int*)BP[0];
    }
    putn(a); putstr(" != "); putn(b); putstr("\n");
    _emit 0xcc                       // INT3
    _emit 0xB8 _emit 0xFF _emit 0x4C // MOV AX, 0x4CFF
    _emit 0xcd _emit 0x21            // INT 0x21
}

void main(void);
void _start(void)
{
    // Clear BSS
    char* bss = &_SBSS;
    while (bss != &_EBSS)
        *bss++ = 0;
    main();
}

#ifndef __SCC__
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef _MSC_VER
#include <io.h>
#define open _open
#define close _close
#define read _read
#define write _write
#define NORETURN __declspec(noreturn)
#else
#ifndef O_BINARY
#define O_BINARY 0
#endif
#include <unistd.h>
#define NORETURN __attribute__((noreturn))
#endif

#else
#define NORETURN
enum {
    O_RDONLY = 0,
    O_WRONLY = 1,
    O_CREAT  = 1,
    O_TRUNC  = 1,
    O_BINARY = 0,
};

void memcpy(void* dst, void* src, int size)
{
    _emit 0x8B _emit 0x7E _emit 0x04 // MOV DI,[BP+0x4]
    _emit 0x8B _emit 0x76 _emit 0x06 // MOV SI,[BP+0x6]
    _emit 0x8B _emit 0x4E _emit 0x08 // MOV CX,[BP+0x8]
    _emit 0xF3 _emit 0xA4            // REP MOVSB
}

void memset(void* ptr, int val, int size)
{
    _emit 0x8B _emit 0x7E _emit 0x04 // MOV DI,[BP+0x4]
    _emit 0x8B _emit 0x46 _emit 0x06 // MOV AX,[BP+0x6]
    _emit 0x8B _emit 0x4E _emit 0x08 // MOV CX,[BP+0x8]
    _emit 0xF3 _emit 0xAA            // REP STOSB
}

int DosCall(int* ax, int bx, int cx, int dx)
{
    // Use segment override to allow DS to not point at data segment

    _emit 0x8B _emit 0x5E _emit 0x04 // 8B5E04            MOV BX,[BP+0x4]
    _emit 0x36 _emit 0x8B _emit 0x07 // 8B07              MOV AX,[SS:BX]
    _emit 0x8B _emit 0x5E _emit 0x06 // 8B5E06            MOV BX,[BP+0x6]
    _emit 0x8B _emit 0x4E _emit 0x08 // 8B4E08            MOV CX,[BP+0x8]
    _emit 0x8B _emit 0x56 _emit 0x0A // 8B560A            MOV DX,[BP+0xA]
    _emit 0xCD _emit 0x21            // CD21              INT 0x21
    _emit 0x8B _emit 0x5E _emit 0x04 // 8B5E04            MOV BX,[BP+0x4]
    _emit 0x36 _emit 0x89 _emit 0x07 // 8907              MOV [SS:BX],AX
    _emit 0xB8 _emit 0x00 _emit 0x00 // B80000            MOV AX,0x0
    _emit 0x19 _emit 0xC0            // 19C0              SBB AX,AX
}

void exit(int retval)
{
    retval = (retval & 0xff) | 0x4c00;
    DosCall(&retval, 0, 0, 0);
}

void raw_putchar(int ch)
{
    int ax = 0x200;
    DosCall(&ax, 0, 0, ch);
}

void putchar(int ch)
{
    if (ch == '\n') raw_putchar('\r');
    raw_putchar(ch);
}

int open(const char* filename, int flags, ...)
{
    int ax;
    if (flags)
        ax = 0x3c00; // create or truncate file
    else
        ax = 0x3d00; // open existing file

    if (DosCall(&ax, 0, 0, filename))
        return -1;
    return ax;
}

void close(int fd)
{
    int ax = 0x3e00;
    DosCall(&ax, fd, 0, 0);
}

int read(int fd, char* buf, int count)
{
    int ax = 0x3f00;
    if (DosCall(&ax, fd, count, buf))
        return 0;
    return ax;
}

int write(int fd, const char* buf, int count)
{
    int ax = 0x4000;
    if (DosCall(&ax, fd, count, buf))
        return 0;
    return ax;
}

enum { _ARGV = 0x5C }; // Overwrite unopened FCBs

int ParseArgs()
{
    char** Args = _ARGV;
    char* CmdLine = (char*)0x80;
    int Len = *CmdLine++ & 0x7f;
    CmdLine[Len] = 0;
    Args[0] = "???";
    int NumArgs = 1;
    for (;;) {
        while (*CmdLine && *CmdLine <= ' ')
            ++CmdLine;
        if (!*CmdLine)
            break;
        Args[NumArgs++] = CmdLine;
        while (*CmdLine && *CmdLine > ' ')
            ++CmdLine;
        if (!*CmdLine)
            break;
        *CmdLine++ = 0;
    }
    Args[NumArgs] = 0;
    return NumArgs;
}

int main(int argc, char** argv);

void _start(void)
{
    // Clear BSS
    memset(&_SBSS, 0, &_EBSS-&_SBSS);
    exit(main(ParseArgs(), _ARGV));
}

char* CopyStr(char* dst, const char* src)
{
    while (*src) *dst++ = *src++;
    *dst = 0;
    return dst;
}

char* vsprintf(char* dest, const char* format, va_list vl)
{
    char TempBuf[9];
    char ch;
    while ((ch = *format++)) {
        if (ch != '%') {
            *dest++ = ch;
            continue;
        }
        char fill = ' ';
        int w = 0;
    Restart:
        ch = *format++;
        if (!w && ch == '0') {
            fill = '0';
            goto Restart;
        } else if (ch >= '0' && ch <= '9') {
            w = w*10 + ch - '0';
            goto Restart;
        }
        if (ch == 's') {
            dest = CopyStr(dest, va_arg(vl, char*));
        } else if(ch == 'c') {
            *dest++ = (char)va_arg(vl, int);
        } else if ((ch == '+' && *format == 'd') || ch == 'd') {
            char* buf;
            int n;
            int s;
            int always;
            if (ch == '+') {
                ++format;
                always = 1;
            } else {
                always = 0;
            }
            n = va_arg(vl, int);
            s = 0;
            if (n < 0) {
                s = 1;
                n = -n;
            }
            buf = TempBuf + sizeof(TempBuf);
            *--buf = 0;
            do {
                *--buf = '0' + n % 10;
                n/=10;
                --w;
            } while (n);
            if (s) {
                *--buf = '-';
                --w;
            } else if (always) {
                *--buf = '+';
                --w;
            }
            while (w-- > 0) *dest++ = fill;
            dest  = CopyStr(dest, buf);
        } else if (ch == 'x' || ch == 'X') {
            const char* HexD = "0123456789ABCDEF";
            int n = va_arg(vl, int);
            int i = 3;
            if (w) i = w-1;
            for (; i >= 0; --i) {
                *dest++ = HexD[(n>>i*4)&0xf];
            }
        } else {
            *dest++ = ch;
        }
    }
    *dest = 0;
    return dest;
}

void sprintf(char* str, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vsprintf(str, format, args);
    va_end(args);
}

void PutStr(const char* s)
{
    while (*s) putchar(*s++);
}

char msg[256]; // XXX
void printf(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vsprintf(msg, format, args);
    va_end(args);
    PutStr(msg);
}

void puts(const char* s)
{
    PutStr(s);
    putchar('\n');
}

void strcpy(char* dst, const char* src)
{
    CopyStr(dst, src);
}

int strcmp(const char* a, const char* b)
{
    _emit 0x8B _emit 0x76 _emit 0x04    // MOV SI, [BP+4]
    _emit 0x8B _emit 0x7E _emit 0x06    // MOV DI, [BP+6]
    _emit 0xAC                          // L: LODSB
    _emit 0x8A _emit 0x25               // MOV AH, [DI]
    _emit 0x47                          // INC DI
    _emit 0x28 _emit 0xE0               // SUB AL, AH
    _emit 0x75 _emit 0x04               // JNZ DONE
    _emit 0x20 _emit 0xE4               // AND AH, AH
    _emit 0x75 _emit 0xF4               // JNZ L
    _emit 0x98                          // DONE: CBW
}

int strlen(const char* s)
{
    _emit 0x8B _emit 0x4E _emit 0x04    // MOV  CX, [BP+4]
    _emit 0x89 _emit 0xCE               // MOV  SI, CX
    _emit 0xAC                          // L: LODSB
    _emit 0x20 _emit 0xC0               // AND  AL, AL
    _emit 0x75 _emit 0xFB               // JNZ  L
    _emit 0x96                          // XCHG AX, SI
    _emit 0x29 _emit 0xC8               // SUB  AX, CX
    _emit 0x48                          // DEC  AX
}

int* GetBP(void)
{
    _emit 0x8B _emit 0x46 _emit 0x00    // MOV AX, [BP]
}

int GetDS(void)
{
    _emit 0x8C _emit 0xD8               // MOV AX, DS
}

int FarPeek(int seg, int off)
{
    _emit 0x1E                          // PUSH DS
    _emit 0x8E _emit 0x5E _emit 0x04    // MOV DS, [BP+4]
    _emit 0x8B _emit 0x76 _emit 0x06    // MOV SI, [BP+6]
    _emit 0xAC                          // LODSB
    _emit 0x30 _emit 0xE4               // XOR AH, AH
    _emit 0x1F                          // POP DS
}

void FarPoke(int seg, int off, int val)
{
    _emit 0x06                          // PUSH ES
    _emit 0x8E _emit 0x46 _emit 0x04    // MOV ES, [BP+4]
    _emit 0x8B _emit 0x7E _emit 0x06    // MOV DI, [BP+6]
    _emit 0x8A _emit 0x46 _emit 0x08    // MOV AL, [BP+8]
    _emit 0xAA                          // STOSB
    _emit 0x07                          // POP ES
}

int clock()
{
    _emit 0x31 _emit 0xC0              // XOR  AX, AX
    _emit 0x8E _emit 0xD8              // MOV  DS, AX
    _emit 0xA1 _emit 0x6C _emit 0x04   // MOV  AX, [0x46C]
    _emit 0x0E                         // PUSH CS
    _emit 0x1F                         // POP  DS
}

void assert(int res)
{
    if (!res) {
        printf("Assertion failed\n");
        int* BP = GetBP();
        printf("\nBP   Return address\n");
        while (*BP) {
            printf("%X %X\n", BP[0], BP[1]);
            BP = (int*)BP[0];
        }
        exit(1);
    }
}
#endif

#ifndef __SCC__
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef _MSC_VER
#include <io.h>
#define open _open
#define close _close
#define read _read
#define write _write
#else
#ifndef O_BINARY
#define O_BINARY 0
#endif
#include <unistd.h>
#endif

#define CREATE_FLAGS O_WRONLY | O_CREAT | O_TRUNC | O_BINARY

#else
// Small "standard library"

enum { CREATE_FLAGS = 1 }; // Ugly hack

void memcpy(char* dst, const char* src, int size)
{
    while (size--)
        *dst++ = *src++;
}

void memset(char* dst, int val, int size)
{
    while (size--)
        *dst++ = val;
}

int DosCall(int* ax, int bx, int cx, int dx)
{
    _emit 0x8B _emit 0x5E _emit 0x04 // 8B5E04            MOV BX,[BP+0x4]
    _emit 0x8B _emit 0x07            // 8B07              MOV AX,[BX]
    _emit 0x8B _emit 0x5E _emit 0x06 // 8B5E06            MOV BX,[BP+0x6]
    _emit 0x8B _emit 0x4E _emit 0x08 // 8B4E08            MOV CX,[BP+0x8]
    _emit 0x8B _emit 0x56 _emit 0x0A // 8B560A            MOV DX,[BP+0xA]
    _emit 0xCD _emit 0x21            // CD21              INT 0x21
    _emit 0x8B _emit 0x5E _emit 0x04 // 8B5E04            MOV BX,[BP+0x4]
    _emit 0x89 _emit 0x07            // 8907              MOV [BX],AX
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
    int ax;
    ax = 0x200;
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
    int ax;
    ax = 0x3e00;
    DosCall(&ax, fd, 0, 0);
}

int read(int fd, char* buf, int count)
{
    int ax;
    ax = 0x3f00;
    if (DosCall(&ax, fd, count, buf))
        return 0;
    return ax;
}

int write(int fd, const char* buf, int count)
{
    int ax;
    ax = 0x4000;
    if (DosCall(&ax, fd, count, buf))
        return 0;
    return ax;
}

int main(int argc, char** argv);
void _start(void)
{
    // Clear BSS
    memset(&_SBSS, 0, &_EBSS-&_SBSS);

    char* CmdLine = (char*)0x80;
    int Len = *CmdLine++ & 0xff;
    CmdLine[Len] = 0;

    char *Args[10]; // Arbitrary limit
    int NumArgs;

    Args[0] = "sas";
    NumArgs = 1;

    while (*CmdLine) {
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
    exit(main(NumArgs, Args));
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
        ch = *format++;
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
            } while (n);
            if (s) *--buf = '-';
            else if (always) *--buf = '+';
            dest  = CopyStr(dest, buf);
        } else if (ch == 'X') {
            const char* HexD = "0123456789ABCDEF";
            int n = va_arg(vl, int);
            int i;
            for (i = 3; i >= 0; --i) {
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

void printf(const char* format, ...)
{
    char msg[80];
    va_list args;
    va_start(args, format);
    vsprintf(msg, format, args);
    va_end(args);
    char* s = msg;
    while (*s) putchar(*s++);
}

void strcpy(char* dst, const char* src)
{
    CopyStr(dst, src);
}

int strlen(const char* s)
{
    int l = 0;
    while (*s++)
        ++l;
    return l;
}

#endif

enum {
    ID_MAX = 600,
    ID_HASH_MAX = 1024, // Must be power of 2 and larger than ID_MAX
    IDBUFFER_MAX = 5000,
    OUTPUT_MAX = 20000,
    INBUF_MAX = 512,
    FIXUP_MAX = 600,
};

enum {
    TOK_EOF,
    TOK_NUMBER,
    TOK_STRING,

    TOK_PLUS      = '+',
    TOK_MINUS     = '-',
    TOK_COLON     = ':',
    TOK_COMMA     = ',',
    TOK_LBRACKET  = '[',
    TOK_RBRACKET  = ']',
    TOK_PIPE      = '|',
    TOK_TILDE     = '~',

    TOK_LSH       = 128,

    // Registers

    // We want the lower bits of the registers to match their index
    TOK_AL = 256, TOK_CL, TOK_DL, TOK_BL, TOK_AH, TOK_CH, TOK_DH, TOK_BH,
    TOK_AX, TOK_CX, TOK_DX, TOK_BX, TOK_SP, TOK_BP, TOK_SI, TOK_DI,
    TOK_ES, TOK_CS, TOK_SS, TOK_DS,

    // Modifiers
    TOK_BYTE,
    TOK_SHORT,
    TOK_WORD,

    // Directives
    TOK_CPU,
    TOK_DB,
    TOK_DW,
    TOK_EQU,
    TOK_ORG,
    TOK_RESB,
    TOK_RESW,

    // Instructions
    TOK_ADC,
    TOK_ADD,
    TOK_AND,
    TOK_CALL,
    TOK_CBW,
    TOK_CLC,
    TOK_CLI,
    TOK_CMP,
    TOK_CMPSB,
    TOK_CWD,
    TOK_DEC,
    TOK_DIV,
    TOK_IDIV,
    TOK_IMUL,
    TOK_INC,
    TOK_INT,
    TOK_JMP,
    TOK_LEA,
    TOK_LODSB,
    TOK_MOV,
    TOK_MOVSB,
    TOK_MUL,
    TOK_NEG,
    TOK_NOT,
    TOK_OR,
    TOK_POP,
    TOK_PUSH,
    TOK_REP,
    TOK_RET,
    TOK_SAR,
    TOK_SBB,
    TOK_SHL,
    TOK_SHR,
    TOK_STC,
    TOK_STI,
    TOK_STOSB,
    TOK_STOSW,
    TOK_SUB,
    TOK_TEST,
    TOK_XCHG,
    TOK_XOR,

    // Jcc
    TOK_JO,         // 0x0
    TOK_JNO,        // 0x1
    TOK_JC,         // 0x2
    TOK_JNC,        // 0x3
    TOK_JZ,         // 0x4
    TOK_JNZ,        // 0x5
    TOK_JNA,        // 0x6
    TOK_JA,         // 0x7
    TOK_JS,         // 0x8
    TOK_JNS,        // 0x9
    TOK_JPE,        // 0xa
    TOK_JPO,        // 0xb
    TOK_JL,         // 0xc
    TOK_JNL,        // 0xd
    TOK_JNG,        // 0xe
    TOK_JG,         // 0xf

    // Jcc synonyms
    TOK_JB,         // JC
    TOK_JNAE,       // JC
    TOK_JNB,        // JNC
    TOK_JAE,        // JNC
    TOK_JE,         // JZ
    TOK_JNE,        // JNZ
    TOK_JBE,        // JNA
    TOK_JNBE,       // JA
    TOK_JNGE,       // JL
    TOK_JGE,        // JNL
    TOK_JLE,        // JNG
    TOK_JNLE,       // JG

    TOK_USER_ID,
    TOK_ID        = TOK_AL,
    TOK_DIRECTIVE = TOK_CPU,
    TOK_INST      = TOK_ADC,
};

enum {
    FIXUP_REL8,
    FIXUP_REL16,
    FIXUP_DISP16,
};

struct Fixup {
    char Type;
    int  Offset;
    struct Fixup* Next;
};
struct Fixup Fixups[FIXUP_MAX];
struct Fixup* NextFixup;

enum {
    LABEL_NONE,
    LABEL_EQU,
    LABEL_NORMAL,
};

struct Label {
    char Type;
    int  Val;
    struct Fixup* Fixups;
};

int InFile;
char InBuf[INBUF_MAX];
int InBufCnt;
int InBufSize;

int Line = 1;
char CurrentChar;

char IdBuffer[IDBUFFER_MAX];
int IdOffset[ID_MAX];
int IdHash[ID_HASH_MAX];
int IdBufferIndex;
int IdCount;

struct Label Labels[ID_MAX-(TOK_USER_ID-TOK_ID)];

int TokenType;

char OutputBuf[OUTPUT_MAX];
int OutputBufPos;
int VirtAddress;
int InBSS;

char StringBuf[128]; // FIXME: remove this (use end of OutputBuf?)

union {
    int Num;
    struct {
        int Len;
        const char* Data;
    } Str;
} TokenVal;

enum {
    OPTYPE_LIT,
    OPTYPE_REG,
    OPTYPE_MEM,
};

struct Operand {
    char Type;
    char ModRM;
    int  Disp;
    struct Label* Fixup;
};

int SizeOverride;

void Fatal(const char* format, ...)
{
    printf("\nLine %d: Fatal error: ", Line);
    char msg[80];
    va_list args;
    va_start(args, format);
    vsprintf(msg, format, args);
    va_end(args);
    printf("%s\n", msg);
    exit(1);
}

#ifdef __SCC__
void assert(int res)
{
    if (!res) {
        Fatal("Assertion failed");
    }
}
#else
#define Fatal(...) do { printf("%s:%d: ", __FILE__, __LINE__); Fatal(__VA_ARGS__); } while (0)
#endif

void OutputByte(int b)
{
    assert(!InBSS);
    assert(OutputBufPos < (int)sizeof(OutputBuf));
    OutputBuf[OutputBufPos++] = b;
    ++VirtAddress;
}

void OutputWord(int w)
{
    OutputByte(w);
    OutputByte(w>>8);
}

int HashStr(const char* Str)
{
    int Hash = 17;
    while (*Str)
        Hash = Hash*89 + (*Str++ & 0xDF);
    return Hash;
}

int AddId(const char* Str)
{
    int l = strlen(Str) + 1;
    assert(IdBufferIndex + l <= IDBUFFER_MAX);
    memcpy(IdBuffer + IdBufferIndex, Str, l);
    IdOffset[IdCount] = IdBufferIndex;
    IdBufferIndex += l;
    int i = 0;
    int Hash = HashStr(Str);
    for (;;) {
        Hash &= ID_HASH_MAX-1;
        if (IdHash[Hash] == -1) {
            IdHash[Hash] = IdCount;
            break;
        }
        Hash += 1 + i++;
    }
    return IdCount++;
}

const char* IdText(int T)
{
    T -= TOK_ID;
    assert(T >=0 && T < IdCount);
    return IdBuffer + IdOffset[T];
}

int LookupId(const char* Str)
{
    int Hash = HashStr(Str);
    int i, j = 0;
    for (;;) {
        Hash &= ID_HASH_MAX-1;
        const int id = IdHash[Hash];
        if (id == -1) {
            break;
        }
        const char* ThisId = IdText(id+TOK_ID);
        for (i = 0;; ++i) {
            char c = Str[i];
            if (id < TOK_USER_ID-TOK_ID && (c >= 'a' && c <= 'z'))
                c &= 0xDF; // HACK: covert to upper case for non-user defined IDs
            if (c != ThisId[i])
                break;
            if (!c) {
                return id;
            }
        }
        Hash += 1 + j++;
    }
    return -1;
}

void ReadChar(void)
{
    if (InBufCnt == InBufSize) {
        InBufCnt = 0;
        InBufSize = read(InFile, InBuf, INBUF_MAX);
        if (!InBufSize) {
            CurrentChar = 0;
            return;
        }
    }
    CurrentChar = InBuf[InBufCnt++];
    if (CurrentChar == '\n') ++Line;
}

int IsDigit(void)
{
    return CurrentChar >= '0' && CurrentChar <= '9';
}

int IsAlpha(void)
{
    return (CurrentChar >= 'A' && CurrentChar <= 'Z') || (CurrentChar >= 'a' && CurrentChar <= 'z');
}

int IsIdChar(void)
{
    return IsAlpha() || CurrentChar == '_' || CurrentChar == '.' || CurrentChar == '$';
}

void TranslateIdToken(void)
{
    if (TokenType >= TOK_USER_ID) {
        const struct Label* L = &Labels[TokenType-TOK_USER_ID];
        if (L->Type == LABEL_EQU) {
            TokenType = TOK_NUMBER;
            TokenVal.Num = L->Val;
        }
        return;
    }
    switch (TokenType) {
    case TOK_JB:
    case TOK_JNAE:
        TokenType = TOK_JC;
        break;
    case TOK_JNB:
    case TOK_JAE:
        TokenType = TOK_JNC;
        break;
    case TOK_JE:
        TokenType = TOK_JZ;
        break;
    case TOK_JNE:
        TokenType = TOK_JNZ;
        break;
    case TOK_JBE:
        TokenType = TOK_JNA;
        break;
    case TOK_JNBE:
        TokenType = TOK_JA;
        break;
    case TOK_JNGE:
        TokenType = TOK_JL;
        break;
    case TOK_JGE:
        TokenType = TOK_JNL;
        break;
    case TOK_JLE:
        TokenType = TOK_JNG;
        break;
    case TOK_JNLE:
        TokenType = TOK_JG;
        break;
    }
}

void GetToken(void)
{
SkipSpaces:
    while (CurrentChar && CurrentChar <= ' ')
        ReadChar();
    if (CurrentChar == ';') {
        while (CurrentChar && CurrentChar != '\n')
            ReadChar();
        goto SkipSpaces;
    }

    TokenType = CurrentChar;

    if (!CurrentChar) {
        // EOF
        return;
    } else if (IsDigit()) {
        int n = CurrentChar-'0';
        ReadChar();
        if (!n) {
            if (CurrentChar == 'x' || CurrentChar == 'X') {
                ReadChar();
                for (;;) {
                    if (IsDigit())
                        n = n*16+CurrentChar-'0';
                    else if (CurrentChar >= 'A' && CurrentChar <= 'F')
                        n = n*16+CurrentChar-'A'+10;
                    else if (CurrentChar >= 'a' && CurrentChar <= 'f')
                        n = n*16+CurrentChar-'a'+10;
                    else
                        break;
                    ReadChar();
                }
            }
        }
        while (IsDigit()) {
            n = n*10 + CurrentChar-'0';
            ReadChar();
        }
        TokenType = TOK_NUMBER;
        TokenVal.Num = n;
        return;
    } else if (IsIdChar()) {
        char Token[17];
        char* Tp = Token;
        do {
            if (Tp < &Token[sizeof(Token)-1])
                *Tp++ = CurrentChar;
            ReadChar();
        } while (IsDigit() || IsIdChar());
        *Tp++ = 0;
        TokenType = LookupId(Token);
        if (TokenType < 0)
            TokenType = AddId(Token);
        TokenType += TOK_ID;
        TranslateIdToken();
        return;
    } else if (CurrentChar == '\'') {
        int Len = 0;
        ReadChar();
        while (CurrentChar != '\'') {
            assert(Len < (int)sizeof(StringBuf));
            if (!CurrentChar) Fatal("Unexpected EOF in string literal");
            StringBuf[Len++] = CurrentChar;
            ReadChar();
        }
        StringBuf[Len] = 0;
        ReadChar();
        TokenType = TOK_STRING;
        TokenVal.Str.Len = Len;
        TokenVal.Str.Data = StringBuf;
        return;
    }

    ReadChar();

    switch (TokenType) {
    case TOK_PLUS:
    case TOK_MINUS:
    case TOK_COLON:
    case TOK_COMMA:
    case TOK_LBRACKET:
    case TOK_RBRACKET:
    case TOK_PIPE:
    case TOK_TILDE:
        return;
    case '<':
        if (CurrentChar == '<') {
            ReadChar();
            TokenType = TOK_LSH;
            return;
        }
        break;
    }
    Fatal("Unhandled char %c (%d)", TokenType, TokenType);
}

char TokenTextBuf[32];
const char* TokenText(void)
{
    switch (TokenType) {
    case TOK_NUMBER: sprintf(TokenTextBuf, "%d", TokenVal.Num); break;
    case TOK_STRING: return TokenVal.Str.Data;
    case TOK_LSH:    return "<<";
    default:
        if (TokenType >= ' ' && TokenType < 127) {
            sprintf(TokenTextBuf, "%c", TokenType);
        } else {
            return IdText(TokenType);
        }
    }
    return TokenTextBuf;
}

int NumFromString(void)
{
    assert(TokenType == TOK_STRING);
    if (TokenVal.Str.Len == 0 || TokenVal.Str.Len > 2)
        Fatal("Expected number got %s", TokenText());
    const int n = (TokenVal.Str.Data[0] & 0xff) | ((TokenVal.Str.Data[1]&0xff)<<8);
    GetToken();
    return n;
}

int ParsePrimary(void)
{
    if (TokenType == TOK_NUMBER) {
        const int n = TokenVal.Num;
        GetToken();
        return n;
    } else if (TokenType == TOK_STRING) {
        return NumFromString();
    } else {
        Fatal("Expected number got %s", TokenText());
        return 0;
    }
}

int IsUnaryOp(void)
{
    return TokenType == TOK_PLUS || TokenType == TOK_MINUS || TokenType == TOK_TILDE;
}


int ParseUnary(void)
{
    if (IsUnaryOp()) {
        int Op = TokenType;
        GetToken();
        int n = ParseUnary();
        if (Op == TOK_MINUS) n = -n;
        else if (Op == TOK_TILDE) n = ~n;
        return n;
    }
    return ParsePrimary();
}

int OperatorPrecedence(void)
{
    if (TokenType == TOK_PLUS || TokenType == TOK_MINUS) return 4;
    if (TokenType == TOK_LSH) return 5;
    if (TokenType == TOK_PIPE) return 10;
    return 100;
}

int ParseExpr(int LHS, int OuterPrecedence)
{
    for (;;) {
        const int Prec = OperatorPrecedence();
        if (Prec > OuterPrecedence) {
            break;
        }
        const int Op = TokenType;
        GetToken();

        int RHS = ParseUnary();
        for (;;) {
            const int LookAheadPrecedence = OperatorPrecedence();
            if (LookAheadPrecedence >= Prec)
                break;
            RHS = ParseExpr(RHS, LookAheadPrecedence);
        }

        switch (Op) {
        case TOK_PLUS:  LHS += RHS; break;
        case TOK_MINUS: LHS -= RHS; break;
        case TOK_PIPE:  LHS |= RHS; break;
        case TOK_LSH:   LHS <<= RHS; break;
        default:
            Fatal("TODO operator %c\n", Op);
        }
    }
    return LHS;
}

int ParseNumber(void)
{
    return ParseExpr(ParseUnary(), 15);
}

void Expect(int Tok)
{
    if (TokenType != Tok) {
        assert(Tok > ' ' && Tok < 128);
        Fatal("Expected %c got %s\n", Tok, TokenText());
    }
    GetToken();
}

int Accept(int Tok)
{
    if (TokenType == Tok) {
        GetToken();
        return 1;
    }
    return 0;
}

struct Label* ReferenceLabel(void)
{
    assert(TokenType >= TOK_USER_ID);
    struct Label* L = &Labels[TokenType - TOK_USER_ID];
    if (L->Type == LABEL_NONE) {
        L->Type   = LABEL_NORMAL;
        L->Val    = -1;
        L->Fixups = 0;
    } else {
        assert(L->Type == LABEL_NORMAL);
    }
    GetToken();
    return L;
}

int IsShort(int V)
{
    return (V&0xffff) == ((char)V & 0xffff);
}

int TranslateMemMod(int Start)
{
    switch (TokenType) {
    case TOK_BX:
        if (Start == -1)
            return 7;
        else if (Start == 4)
            return 0;
        break;
    case TOK_BP:
        if (Start == -1)
            return 6;
        break;
    case TOK_SI:
        if (Start == -1)
            return 4;
        break;
    case TOK_DI:
        if (Start == -1)
            return 5;
        break;
    }
    Fatal("TODO %s in mem parsing orig=%02X\n", TokenText(), Start);
    return -1;
}

void ParseOperand(struct Operand* Oper)
{
    int neg = 0;
    Oper->Disp  = 0;
    Oper->Fixup = 0;
    if (TokenType == TOK_NUMBER || TokenType == TOK_STRING || IsUnaryOp()) {
        Oper->Type = OPTYPE_LIT;
        Oper->Disp = ParseNumber();
    } else if (TokenType == TOK_LBRACKET) {
        GetToken();
DoMem:
        Oper->Type  = OPTYPE_MEM;
        Oper->ModRM = -1;
        for (;;) {
            if (TokenType >= TOK_ES && TokenType <= TOK_DS) {
                // Segment override
                OutputByte(0x26 | (TokenType-TOK_ES));
                GetToken();
                Expect(TOK_COLON);
                continue;
            } else if (TokenType == TOK_BX || TokenType == TOK_BP || TokenType == TOK_SI || TokenType == TOK_DI) {
                Oper->ModRM = TranslateMemMod(Oper->ModRM);
                GetToken();
            } else if (TokenType == TOK_NUMBER) {
                if (neg)
                    Oper->Disp -= TokenVal.Num;
                else
                    Oper->Disp += TokenVal.Num;
                GetToken();
            } else if (TokenType >= TOK_USER_ID) {
                assert(!Oper->Fixup);
                struct Label* L = ReferenceLabel();
                assert(L->Type == LABEL_NORMAL);
                if (L->Val == -1)
                    Oper->Fixup = L;
                else
                    Oper->Disp += L->Val;
            } else {
                Fatal("Invalid memory operand. %s", TokenText());
            }
            if (Accept(TOK_MINUS)) {
                neg = 1;
                continue;
            }
            neg = 0;
            if (!Accept(TOK_PLUS))
                break;
        }
        Expect(TOK_RBRACKET);

        if (Oper->ModRM == 6 && !Oper->Disp)
            Oper->ModRM |= 0x40;

        if (Oper->ModRM == -1)
            Oper->ModRM = 6;
        else if (Oper->Fixup)
            Oper->ModRM |= 0x80; // Disp16
        else if (Oper->Disp) {
            if (IsShort(Oper->Disp))
                Oper->ModRM |= 0x40; // Disp8
            else
                Oper->ModRM |= 0x80; // Disp16
        }
    } else if (TokenType == TOK_BYTE || TokenType == TOK_WORD) {
        SizeOverride = TokenType == TOK_BYTE? 1 : 2;
        GetToken();
        Expect(TOK_LBRACKET);
        goto DoMem;
    } else if (TokenType >= TOK_AL && TokenType <= TOK_DS) {
        Oper->Type = OPTYPE_REG;
        Oper->Disp = TokenType;
        GetToken();
    } else if (TokenType >= TOK_USER_ID) {
        struct Label* L = ReferenceLabel();
        assert(L->Type == LABEL_NORMAL);
        Oper->Type = OPTYPE_LIT;
        if (L->Val == -1)
            Oper->Fixup = L;
        else
            Oper->Disp  = L->Val;
    } else {
        Fatal("Expected operand got %s", TokenText());
    }
}

void Parse2Operands(struct Operand* op1, struct Operand* op2)
{
    ParseOperand(op1);
    Expect(TOK_COMMA);
    ParseOperand(op2);
}

void HandleDx(int size)
{
    int n = 0; // Keep count of string output for alignment
    do {
        if (TokenType == TOK_NUMBER) {
            if (size == 1)
                OutputByte(TokenVal.Num);
            else
                OutputWord(TokenVal.Num);
            GetToken();
        } else if (TokenType == TOK_STRING) {
            n += TokenVal.Str.Len;
            int i;
            for (i = 0; i < TokenVal.Str.Len; ++i)
                OutputByte(TokenVal.Str.Data[i]);
            GetToken();
        } else if (TokenType >= TOK_USER_ID) {
            assert(size == 2);
            struct Label* L = ReferenceLabel();
            assert(L->Type == LABEL_NORMAL);
            if (L->Val != -1) {
                OutputWord(L->Val);
            } else {
                OutputWord(0);
                printf("TODO: Need DISP16 fixup for DW\n");
            }
        } else {
            Fatal("Unexpected token in DB/DW directive: %s", TokenText());
        }
    } while (Accept(TOK_COMMA));
    if (size == 2 && (n&1))
        OutputByte(0);
}

void HandleResx(int size)
{
    InBSS = 1;
    VirtAddress += size * ParseNumber();
}

void HandleDirective(int Dir)
{
    switch (Dir) {
    case TOK_CPU:
        {
            int CPU = ParseNumber();
            if (CPU != 8086)
                Fatal("Unsupported CPU %d", CPU);
        }
        return;
    case TOK_DB:
        HandleDx(1);
        return;
    case TOK_DW:
        HandleDx(2);
        return;
    case TOK_ORG:
        VirtAddress = ParseNumber();
        return;
    case TOK_RESB:
        HandleResx(1);
        return;
    case TOK_RESW:
        HandleResx(2);
        return;
    }
    Fatal("Directive not implemented: %s\n", IdText(Dir));
}

void AddFixupHere(struct Label* L, char Type)
{
    assert(NextFixup);
    struct Fixup* F = NextFixup;
    NextFixup = NextFixup->Next;
    F->Type   = Type;
    F->Offset = OutputBufPos;
    F->Next   = L->Fixups;
    L->Fixups = F;
}

void OutputImm8(struct Operand* Op)
{
    assert(!Op->Fixup);
    OutputByte(Op->Disp);
}

void OutputImm16(struct Operand* Op)
{
    if (Op->Fixup) {
        AddFixupHere(Op->Fixup, FIXUP_DISP16);
    }
    OutputWord(Op->Disp);
}

void OutputImm(struct Operand* Op, int W)
{
    if (W) {
        OutputImm16(Op);
    } else {
        OutputImm8(Op);
    }
}

void OutputModRM(struct Operand* Op, int r)
{
    assert(Op->Type == OPTYPE_MEM);
    assert(r>=0 && r<8);
    OutputByte(Op->ModRM | r<<3);
    if (Op->ModRM == 6 || (Op->ModRM & 0xc0) == 0x80) {
        if (Op->Fixup)
            AddFixupHere(Op->Fixup, FIXUP_DISP16);
        OutputWord(Op->Disp);
    } else if ((Op->ModRM & 0xc0) == 0x40) {
        OutputByte(Op->Disp);
    }
}

void OutputMR(int Inst, struct Operand* L, struct Operand* R)
{
    assert(L->Type == OPTYPE_MEM && R->Type == OPTYPE_REG);
    OutputByte(Inst | (R->Disp >= TOK_AX && R->Disp < TOK_ES));
    OutputModRM(L, (R->Disp&7));
}

void OutputMImm(int Inst, int r, struct Operand* M, struct Operand* Imm)
{
    assert(M->Type == OPTYPE_MEM && Imm->Type == OPTYPE_LIT);
    assert(SizeOverride);
    int W = SizeOverride!=1;
    if (W && Inst == 0x80 && !Imm->Fixup && IsShort(Imm->Disp)) {
        Inst |= 3;
        W = 0;
    }
    OutputByte(Inst|W);
    OutputModRM(M, r);
    OutputImm(Imm, W);
}

void HandleMOV(void)
{
    struct Operand op1, op2;
    Parse2Operands(&op1, &op2);
    if (op1.Type == OPTYPE_REG) {
        int W = op1.Disp >= TOK_AX;
        if (op2.Type == OPTYPE_REG) {
            int Inst;
            int ModRM = (op1.Disp&7) | (op2.Disp&7)<<3;
            if (op1.Disp < TOK_ES) {
                Inst = op2.Disp < TOK_ES ? 0x88|W : 0x8c;
            } else {
                Inst  = 0x8e;
                ModRM = (op2.Disp&7) | (op1.Disp&7)<<3;
            }
            OutputByte(Inst);
            OutputByte(0xC0 | ModRM);
        } else if (op2.Type == OPTYPE_LIT) {
            OutputByte(0xB0 + op1.Disp - TOK_AL);
            OutputImm(&op2, W);
        } else {
            assert(op2.Type == OPTYPE_MEM);
            if (op2.ModRM == 6 && (op1.Disp == TOK_AL || op1.Disp == TOK_AX)) {
                OutputByte(0xA0 | (op1.Disp == TOK_AX));
                OutputImm16(&op2);
            } else {
                OutputMR(op1.Disp >= TOK_ES ? 0x8E: 0x8A, &op2, &op1);
            }
        }
    } else if (op1.Type == OPTYPE_MEM) {
        if (op2.Type == OPTYPE_REG) {
            if (op1.ModRM == 6 && (op2.Disp == TOK_AL || op2.Disp == TOK_AX)) {
                OutputByte(0xA2 | (op2.Disp == TOK_AX));
                OutputImm16(&op1);
            } else {
                OutputMR(op2.Disp >= TOK_ES ? 0x8C: 0x88, &op1, &op2);
            }
        } else {
            assert(op2.Type == OPTYPE_LIT);
            OutputMImm(0xC6, 0, &op1, &op2);
        }
    } else {
        Fatal("TODO: MOV");
    }
}

void HandleALU(int Base)
{
    (void)Base;
    struct Operand op1, op2;
    Parse2Operands(&op1, &op2);
    if (op1.Type == OPTYPE_REG) {
        const int W = op1.Disp >= TOK_AX;
        if (op2.Type == OPTYPE_REG) {
            OutputByte(Base|W);
            OutputByte(0xC0 | (op2.Disp&7)<<3 | (op1.Disp&7));
        } else if (op2.Type == OPTYPE_LIT) {
            if (op1.Disp == TOK_AL) {
                OutputByte(Base+4);
                OutputImm8(&op2);
            } else if (op1.Disp == TOK_AX) {
                OutputByte(Base+5);
                OutputImm16(&op2);
            } else {
                int inst = 0x80|W;
                if (W && !op2.Fixup && IsShort(op2.Disp))
                    inst |= 2;
                OutputByte(inst);
                OutputByte(0xC0 | (op1.Disp&7) | Base);
                OutputImm(&op2, inst == 0x81);
            }
        } else {
            assert(op2.Type == OPTYPE_MEM);
            OutputMR(Base+2, &op2, &op1);
        }
    } else if (op1.Type == OPTYPE_MEM) {
        if (op2.Type == OPTYPE_REG) {
            OutputMR(Base, &op1, &op2);
        } else {
            assert(op2.Type == OPTYPE_LIT);
            OutputMImm(0x80, Base>>3, &op1, &op2);
        }
    } else {
        Fatal("TODO: ALU");
    }
}

void HandleROT(int r)
{
    struct Operand op1, op2;
    Parse2Operands(&op1, &op2);
    assert(op1.Type == OPTYPE_REG);
    const int W = op1.Disp >= TOK_AX;
    int Inst = 0xD0 | W;
    if (op2.Type == OPTYPE_REG) {
        assert(op2.Disp == TOK_CL);
        Inst |= 2;
    }
    OutputByte(Inst | W);
    OutputByte(0xC0 | r<<3 | (op1.Disp&7));
}

void HandleRel8(struct Operand* Op)
{
    if (Op->Fixup)
        AddFixupHere(Op->Fixup, FIXUP_REL8);
    OutputByte(Op->Disp-(VirtAddress+1));
}

void HandleRel16(struct Operand* Op)
{
    if (Op->Fixup)
        AddFixupHere(Op->Fixup, FIXUP_REL16);
    OutputWord(Op->Disp-(VirtAddress+2));
}

void HandleCALL(void)
{
    struct Operand op;
    ParseOperand(&op);
    if (op.Type == OPTYPE_REG) {
        OutputByte(0xFF);
        OutputByte(0xC0 | 2<<3 | (op.Disp&7));
    } else {
        assert(op.Type == OPTYPE_LIT);
        OutputByte(0xE8);
        HandleRel16(&op);
    }
}

void HandleJMP(void)
{
    if (Accept(TOK_SHORT))
        SizeOverride = 1;
    struct Operand op;
    ParseOperand(&op);
    assert(op.Type == OPTYPE_LIT);
    if (SizeOverride == 1 || (!op.Fixup && IsShort(op.Disp - (VirtAddress+2)))) {
        OutputByte(0xEb);
        HandleRel8(&op);
    } else {
        OutputByte(0xE9);
        HandleRel16(&op);
    }
}

void HandleJCC(int CC)
{
    if (Accept(TOK_SHORT))
        SizeOverride = 1;
    struct Operand op;
    ParseOperand(&op);
    assert(op.Type == OPTYPE_LIT);
    OutputByte(0x70|CC);
    if (op.Fixup) {
        AddFixupHere(op.Fixup, FIXUP_REL8);
        OutputByte(-(VirtAddress+1));
    } else {
        OutputByte(op.Disp-(VirtAddress+1));
    }
}

void HandleIncDec(int Dec)
{
    (void)Dec;
    struct Operand op;
    ParseOperand(&op);
    if (op.Type == OPTYPE_REG) {
        if (op.Disp >= TOK_AX) {
            OutputByte(0x40 | Dec<<3 | (op.Disp&7));
        } else {
            OutputByte(0xFE);
            OutputByte(0xC0 | Dec<<3 | (op.Disp&7));
        }
    } else {
        assert(op.Type == OPTYPE_MEM && SizeOverride);
        OutputByte(0xFE | (SizeOverride-1));
        OutputModRM(&op, Dec);
    }
}

void HandlePUSH(void)
{
    struct Operand op;
    ParseOperand(&op);
    if (op.Type == OPTYPE_REG) {
        if (op.Disp >= TOK_ES) {
            OutputByte(0x06 | (op.Disp-TOK_ES)<<3);
        } else {
            assert(op.Disp >= TOK_AX);
            OutputByte(0x50 | (op.Disp-TOK_AX));
        }
    } else {
        assert(op.Type == OPTYPE_MEM && SizeOverride == 2);
        OutputByte(0xFF);
        OutputModRM(&op, 6);
    }
}

void HandlePOP(void)
{
    struct Operand op;
    ParseOperand(&op);
    assert(op.Type == OPTYPE_REG);
    if (op.Disp >= TOK_ES) {
        OutputByte(0x07 | (op.Disp-TOK_ES)<<3);
    } else {
        assert(op.Disp >= TOK_AX);
        OutputByte(0x58 | (op.Disp-TOK_AX));
    }
}

void HandleINT(void)
{
    int n = ParseNumber();
    assert(n>=0 && n<256);
    OutputByte(0xCD);
    OutputByte(n);
}

void HandleTEST(void)
{
    struct Operand op1, op2;
    Parse2Operands(&op1, &op2);
    if (op1.Type == OPTYPE_REG) {
        assert(op2.Type == OPTYPE_LIT);
        const int W = op1.Disp >= TOK_AX;
        if (op1.Disp == TOK_AL || op1.Disp == TOK_AX) {
            OutputByte(0xA8 | W);
        } else {
            OutputByte(0xF6 | W);
            OutputByte(0xC0 | (op1.Disp&7));
        }
        OutputImm(&op2, W);
    } else {
        assert(op1.Type == OPTYPE_MEM);
        assert(op2.Type == OPTYPE_LIT);
        assert(SizeOverride);
        OutputByte(0xF6|(SizeOverride-1));
        OutputModRM(&op1, 0);
        OutputImm(&op2, SizeOverride-1);
    }
}

void HandleXCHG(void)
{
    struct Operand op1, op2;
    Parse2Operands(&op1, &op2);
    if (op1.Type == OPTYPE_REG) {
        int W = op1.Disp >= TOK_AX;
        if (op2.Type == OPTYPE_REG) {
            assert(op1.Disp == TOK_AX);
            OutputByte(0x90 | (op2.Disp&7));
        } else {
            assert(op2.Type == OPTYPE_MEM);
            OutputByte(0x86|W);
            OutputModRM(&op2, op1.Disp&7);
        }
    } else {
        Fatal("TODO: XCHG");
    }
}

void HandleMulDiv(int r)
{
    (void)r;
    struct Operand op;
    ParseOperand(&op);
    assert(op.Type == OPTYPE_REG);
    OutputByte(0xF6 | (op.Disp >= TOK_AX));
    OutputByte(0xC0 | r<<3 | (op.Disp&7));
}

void HandleNotNeg(int r)
{
    (void)r;
    struct Operand op;
    ParseOperand(&op);
    assert(op.Type == OPTYPE_REG);
    OutputByte(0xF6 | (op.Disp >= TOK_AX));
    OutputByte(0xC0 | r<<3 | (op.Disp&7));
}

void HandleLEA(void)
{
    struct Operand op1, op2;
    Parse2Operands(&op1, &op2);
    assert(op1.Type == OPTYPE_REG);
    assert(op2.Type == OPTYPE_MEM);
    OutputMR(0x8D, &op2, &op1);
}

void HandleInstruction(int Inst)
{
    switch (Inst) {
    case TOK_ADC:
        HandleALU(0x10);
        return;
    case TOK_ADD:
        HandleALU(0x00);
        return;
    case TOK_AND:
        HandleALU(0x20);
        return;
    case TOK_CALL:
        HandleCALL();
        return;
    case TOK_CBW:
        OutputByte(0x98);
        return;
    case TOK_CLC:
        OutputByte(0xF8);
        return;
    case TOK_CLI:
        OutputByte(0xFA);
        return;
    case TOK_CMP:
        HandleALU(0x38);
        return;
    case TOK_CMPSB:
        OutputByte(0xA6);
        return;
    case TOK_CWD:
        OutputByte(0x99);
        return;
    case TOK_DEC:
        HandleIncDec(1);
        return;
    case TOK_DIV:
        HandleMulDiv(6);
        return;
    case TOK_IDIV:
        HandleMulDiv(7);
        return;
    case TOK_IMUL:
        HandleMulDiv(5);
        return;
    case TOK_INC:
        HandleIncDec(0);
        return;
    case TOK_INT:
        HandleINT();
        return;
    case TOK_JMP:
        HandleJMP();
        return;
    case TOK_LEA:
        HandleLEA();
        return;
    case TOK_LODSB:
        OutputByte(0xAC);
        return;
    case TOK_MOV:
        HandleMOV();
        return;
    case TOK_MOVSB:
        OutputByte(0xA4);
        return;
    case TOK_MUL:
        HandleMulDiv(4);
        return;
    case TOK_NEG:
        HandleNotNeg(3);
        return;
    case TOK_NOT:
        HandleNotNeg(2);
        return;
    case TOK_OR:
        HandleALU(0x08);
        return;
    case TOK_POP:
        HandlePOP();
        return;
    case TOK_PUSH:
        HandlePUSH();
        return;
    case TOK_REP:
        OutputByte(0xF3);
        return;
    case TOK_RET:
        OutputByte(0xC3);
        return;
    case TOK_SAR:
        HandleROT(7);
        return;
    case TOK_SBB:
        HandleALU(0x18);
        return;
    case TOK_SHL:
        HandleROT(4);
        return;
    case TOK_SHR:
        HandleROT(5);
        return;
    case TOK_STC:
        OutputByte(0xF9);
        return;
    case TOK_STI:
        OutputByte(0xFB);
        return;
    case TOK_STOSB:
        OutputByte(0xAA);
        return;
    case TOK_STOSW:
        OutputByte(0xAB);
        return;
    case TOK_SUB:
        HandleALU(0x28);
        return;
    case TOK_TEST:
        HandleTEST();
        return;
    case TOK_XCHG:
        HandleXCHG();
        return;
    case TOK_XOR:
        HandleALU(0x30);
        return;
    case TOK_JO:
    case TOK_JNO:
    case TOK_JC:
    case TOK_JNC:
    case TOK_JZ:
    case TOK_JNZ:
    case TOK_JNA:
    case TOK_JA:
    case TOK_JS:
    case TOK_JNS:
    case TOK_JPE:
    case TOK_JPO:
    case TOK_JL:
    case TOK_JNL:
    case TOK_JNG:
    case TOK_JG:
        HandleJCC(Inst - TOK_JO);
        return;
    }
    Fatal("Instruction not implemented: %s\n", IdText(Inst));
}

void AddBuiltins(const char* S)
{
    char buf[16];
    int i, c;
    do {
        i = 0;
        while ((c = *S++) > ' ') {
            buf[i++] = c;
        }
        buf[i] = 0;
        AddId(buf);
    } while (c);
}

void MakeOutputFilename(char* dest, const char* n)
{
    char* LastDot;
    LastDot = 0;
    while (*n) {
        if (*n == '.')
            LastDot = dest;
        *dest++ = *n++;
    }
    strcpy(LastDot ? LastDot : dest, ".com");
}


int main(int argc, char** argv)
{
    if (argc < 2) {
        Fatal("Need argument");
    }

    InFile = open(argv[1], 0); // O_RDONLY
    if (InFile < 0) {
        Fatal("Error opening input file");
    }

    memset(IdHash, -1, sizeof(IdHash));

    AddBuiltins(
         "AL CL DL BL AH CH DH BH"
        " AX CX DX BX SP BP SI DI"
        " ES CS SS DS"
        " BYTE SHORT WORD"
        " CPU DB DW EQU ORG RESB RESW"
        " ADC ADD AND CALL CBW CLC CLI CMP CMPSB CWD DEC DIV IDIV IMUL INC INT JMP"
        " LEA LODSB MOV MOVSB MUL NEG NOT OR POP PUSH REP RET SAR SBB SHL SHR STC STI"
        " STOSB STOSW SUB TEST XCHG XOR"
        " JO JNO JC JNC JZ JNZ JNA JA JS JNS JPE JPO JL JNL JNG JG"
        " JB JNAE JNB JAE JE JNE JBE JNBE JNGE JGE JLE JNLE"
    );
    assert(IdCount == TOK_JNLE+1-TOK_ID);

    int i;
    for (i = 0; i < FIXUP_MAX-1; ++i) {
        Fixups[i].Next = &Fixups[i+1];
    }
    NextFixup = &Fixups[0];

    ReadChar();
    GetToken();
    while (TokenType) {
        if (TokenType >= TOK_USER_ID) {
            struct Label* L = &Labels[TokenType - TOK_USER_ID];
            const int global = IdText(TokenType)[0] != '.';
            GetToken();
            if (Accept(TOK_COLON)) {
                if (L->Type == LABEL_NONE) {
                    L->Type   = LABEL_NORMAL;
                    L->Fixups = 0;
                } else {
                    assert(L->Val == -1);
                    struct Fixup* F = L->Fixups;
                    if (F) {
                        struct Fixup* Last = F;
                        while (F) {
                            switch (F->Type) {
                            case FIXUP_REL8:
                                OutputBuf[F->Offset] += VirtAddress;
                                break;
                            case FIXUP_REL16:
                            case FIXUP_DISP16:
                                {
                                    const int v = VirtAddress + ((OutputBuf[F->Offset]&0xff)|((OutputBuf[F->Offset+1]&0xff)<<8));
                                    OutputBuf[F->Offset] = v;
                                    OutputBuf[F->Offset+1] = v>>8;
                                }
                                break;
                            }
                            Last = F;
                            F = F->Next;
                        }
                        Last->Next = NextFixup;
                        NextFixup = L->Fixups;
                    }
                }
                L->Val = VirtAddress;
                if (global) {
                    // Retire local labels
                    int i;
                    for (i = 0; i < IdCount - (TOK_USER_ID-TOK_ID); ++i) {
                        if (Labels[i].Type && *IdText(i+TOK_USER_ID) == '.') {
                            Labels[i].Type = LABEL_NONE;
                        }
                    }
                }
                continue;
            } else if (Accept(TOK_EQU)) {
                assert(L->Type == LABEL_NONE);
                L->Type = LABEL_EQU;
                L->Val  = ParseNumber();
                continue;
            }
            goto Error;
        } else if (TokenType < TOK_DIRECTIVE) {
        Error:
            Fatal("Unexpected token %s", TokenText());
        }
        const int Op = TokenType;
        GetToken();
        SizeOverride = 0;

        if (Op < TOK_INST) {
            HandleDirective(Op);
        } else {
            HandleInstruction(Op);
        }
    }

    if (argv[2])
       strcpy(StringBuf, argv[2]);
    else
        MakeOutputFilename(StringBuf, argv[1]);
    const int OutFile = open(StringBuf, CREATE_FLAGS, 0600);
    if (OutFile < 0) {
        Fatal("Error creating output file");
    }
    write(OutFile, OutputBuf, OutputBufPos);
    close(OutFile);
}

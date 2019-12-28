#ifndef __SCC__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
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
#else
#ifndef O_BINARY
#define O_BINARY 0
#endif
#include <unistd.h>
#endif

#else
// Only used when self-compiling
// Small "standard library"

enum {
    O_RDONLY = 0,
    O_WRONLY = 1,
    O_CREAT  = 1,
    O_TRUNC  = 1,
    O_BINARY = 0,
};

int main(int argc, char** argv);

int DosCall(int* ax, int bx, int cx, int dx)
{
    _emit 0x8B _emit 0x5E _emit 0x04 // MOV BX,[BP+0x4]
    _emit 0x8B _emit 0x07            // MOV AX,[BX]
    _emit 0x8B _emit 0x5E _emit 0x06 // MOV BX,[BP+0x6]
    _emit 0x8B _emit 0x4E _emit 0x08 // MOV CX,[BP+0x8]
    _emit 0x8B _emit 0x56 _emit 0x0A // MOV DX,[BP+0xA]
    _emit 0xCD _emit 0x21            // INT 0x21
    _emit 0x8B _emit 0x5E _emit 0x04 // MOV BX,[BP+0x4]
    _emit 0x89 _emit 0x07            // MOV [BX],AX
    _emit 0xB8 _emit 0x00 _emit 0x00 // MOV AX,0x0
    _emit 0x19 _emit 0xC0            // SBB AX,AX
}

void memset(void* ptr, int val, int size)
{
    _emit 0x8B _emit 0x7E _emit 0x04 // MOV DI,[BP+0x4]
    _emit 0x8B _emit 0x46 _emit 0x06 // MOV AX,[BP+0x6]
    _emit 0x8B _emit 0x4E _emit 0x08 // MOV CX,[BP+0x8]
    _emit 0xF3 _emit 0xAA            // REP STOSB
}

int memcmp(void* l, void* r, int size)
{
    _emit 0x31 _emit 0xC0            // XOR AX, AX
    _emit 0x8B _emit 0x7E _emit 0x04 // MOV DI,[BP+0x4]
    _emit 0x8B _emit 0x76 _emit 0x06 // MOV SI,[BP+0x6]
    _emit 0x8B _emit 0x4E _emit 0x08 // MOV CX,[BP+0x8]
    _emit 0xF3 _emit 0xA6            // REPE CMPSB
    _emit 0x74 _emit 0x01            // JE  $+3
    _emit 0x40                       // INC AX // XXX: TODO sometimes output -1
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

enum { ARGS = 0x5C }; // Overwrite unopened FCBs

int ParseArgs()
{
    char** Args = ARGS;
    char* CmdLine = (char*)0x80;
    int Len = *CmdLine++ & 0x7f;
    CmdLine[Len] = 0;
    Args[0] = "scc";
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

void _start(void)
{
    // Clear BSS
    memset(&_SBSS, 0, &_EBSS-&_SBSS);
    exit(main(ParseArgs(), ARGS));
}

int isdigit(int c)
{
    _emit 0x8A _emit 0x46 _emit 0x04    // MOV AL, [BP+4]
    _emit 0x2C _emit 0x30               // SUB AL, 0x30
    _emit 0x3C _emit 0x0A               // CMP AL, 10
    _emit 0x19 _emit 0xC0               // SBB AX, AX
}

int isalpha(int c)
{
    _emit 0x8A _emit 0x46 _emit 0x04    // MOV AL, [BP+4]
    _emit 0x24 _emit 0xDF               // AND AL, 0xDF
    _emit 0x2C _emit 0x41               // SUB AL, 'A'
    _emit 0x3C _emit 0x1A               // CMP AL, 'Z'-'A'+1
    _emit 0x19 _emit 0xC0               // SBB AX, AX
}

int clock()
{
    _emit 0x31 _emit 0xC0              // XOR  AX, AX
    _emit 0x8E _emit 0xD8              // MOV  DS, AX
    _emit 0xA1 _emit 0x6C _emit 0x04   // MOV  AX, [0x46C]
    _emit 0x0E                         // PUSH CS
    _emit 0x1F                         // POP  DS
}

#endif

// Misc. constants
enum {
    CODESTART = 0x100,
    INBUF_MAX = 512,
    SCOPE_MAX = 16,
    VARDECL_MAX = 380,
    ID_MAX = 580,
    ID_HASHMAX = 1024,      // Must be power of 2 and (some what) greater than ID_MAX
    IDBUFFER_MAX = 5200,
    LABEL_MAX = 300,
    NAMED_LABEL_MAX = 8,
    OUTPUT_MAX = 0x6000,    // Always try to reduce this if something fails unexpectedly... ~600bytes of stack is needed.
    STRUCT_MAX = 8,
    STRUCT_MEMBER_MAX = 32,
    ARRAY_MAX = 32,
    GLOBAL_RESERVE = 4,     // How many globals symbols to allow to be defined in functions (where static variables count as globals)
    ARGS_MAX = 8,

    // Brute forced best values (when change was made)
    HASHINIT = 17,
    HASHMUL  = 89,
};

// Type flags and values
enum {
    VT_VOID,
    VT_BOOL,            // CurrentValue holds condition code for "true"
    VT_CHAR,
    VT_INT,
    VT_STRUCT,          // CurrentTypeExtra holds index into StructDecls
    VT_ARRAY,           // CurrentTypeExtra holds index into ArrayDecls

    VT_BASEMASK = 7,

    VT_UNSIGNED = 1<<3,
    VT_LVAL     = 1<<4,

    VT_FUN      = 1<<5,
    VT_FUNPTR   = 3<<5, // HACK

    VT_PTR1     = 1<<7,
    VT_PTRMASK  = 3<<7, // 3 levels of indirection should be enough..

    VT_LOCLIT   = 1<<9, // CurrentVal holds a literal value (or label)
    VT_LOCOFF   = 2<<9, // CurrentVal holds BP offset
    VT_LOCGLOB  = 3<<9, // CurrentVal holds the VarDecl index of a global
    VT_LOCMASK  = 3<<9,

    VT_STATIC   = 1<<11,
    VT_TYPEDEF  = 1<<12,
};

// Token types
enum {
    TOK_EOF, // Must be zero

    TOK_NUM,
    TOK_STRLIT,

    // Single character operators have their ASCII value as token type

    TOK_EQEQ = 128,
    TOK_NOTEQ,
    TOK_LTEQ,
    TOK_GTEQ,
    TOK_PLUSPLUS,
    TOK_MINUSMINUS,
    TOK_ANDAND,
    TOK_OROR,
    TOK_LSH,
    TOK_RSH,
    TOK_PLUSEQ,
    TOK_MINUSEQ,
    TOK_STAREQ,
    TOK_SLASHEQ,
    TOK_MODEQ,
    TOK_LSHEQ,
    TOK_RSHEQ,
    TOK_ANDEQ,
    TOK_XOREQ,
    TOK_OREQ,
    TOK_ELLIPSIS,
    TOK_ARROW,

    TOK_KEYWORD,
    // NOTE: Must match order of registration in main
    TOK_AUTO = TOK_KEYWORD,
    TOK_BREAK,
    TOK_CASE,
    TOK_CHAR,
    TOK_CONST,
    TOK_CONTINUE,
    TOK_DEFAULT,
    TOK_DO,
    TOK_DOUBLE,
    TOK_ELSE,
    TOK_ENUM,
    TOK_EXTERN,
    TOK_FLOAT,
    TOK_FOR,
    TOK_GOTO,
    TOK_IF,
    TOK_INT,
    TOK_LONG,
    TOK_REGISTER,
    TOK_RETURN,
    TOK_SHORT,
    TOK_SIGNED,
    TOK_SIZEOF,
    TOK_STATIC,
    TOK_STRUCT,
    TOK_SWITCH,
    TOK_TYPEDEF,
    TOK_UNION,
    TOK_UNSIGNED,
    TOK_VOID,
    TOK_VOLATILE,
    TOK_WHILE,

    TOK_VA_LIST,
    TOK_VA_START,
    TOK_VA_END,
    TOK_VA_ARG,

    TOK__EMIT,

    // Start of user-defined literals (well and some compiler defined ones)
    TOK_ID,

    TOK__START = TOK_ID,
    TOK__SBSS,
    TOK__EBSS,
};

// Operator precedence
enum {
    PREC_ASSIGN = 14,
    PREC_COMMA,
    PREC_STOP = 100,
};

struct Label {
    int Addr;
    int Ref;
};

struct NamedLabel {
    int Id;
    int LabelId;
};

struct StructMember {
    int Id;
    int Type;
    int TypeExtra;
    struct StructMember* Next;
};

enum {
    IS_UNION_FLAG = 0x8000,
};

struct StructDecl {
    int Id; // OR'ed with IS_UNION_FLAG if union, id is ID_MAX for unnamed structs/unions
    struct StructMember* Members;
};

struct ArrayDecl {
    int Bound;
    int Type;
    int TypeExtra;
};

struct VarDecl {
    int Id;
    int Type;
    int TypeExtra;
    int Offset;
    int Ref; // Fixup address list for globals
    int Prev;
};

struct Argument {
    int Id;
    int Type;
    int TypeExtra;
};

int CodeAddress = CODESTART;
int BssSize;

int MapFile;

int InFile;
char InBuf[INBUF_MAX+1];
char* InBufPtr;
char* InBufEnd;
char CurChar;

int Line = 1;

int TokenType;
int TokenNumVal;
unsigned char* TokenStrLit;
char OperatorPrecedence;

char IdBuffer[IDBUFFER_MAX];
int IdBufferIndex;

char* IdText[ID_MAX];
int IdHashTab[ID_HASHMAX];
int IdCount;

struct StructMember StructMembers[STRUCT_MEMBER_MAX];
int StructMemCount;

struct StructDecl StructDecls[STRUCT_MAX];
int StructCount;

struct ArrayDecl ArrayDecls[ARRAY_MAX];
int ArrayCount;

struct VarDecl VarDecls[VARDECL_MAX];
int VarLookup[ID_MAX];
int IgnoreRedef;

int Scopes[SCOPE_MAX]; // -> Next free VarDecl index (conversely one more than index of last defined variable/id)
int ScopesCount = 1; // First scope is global

struct Label Labels[LABEL_MAX];
int LocalLabelCounter;

struct NamedLabel NamedLabels[NAMED_LABEL_MAX];
int NamedLabelCount;

struct Argument Args[ARGS_MAX];
int ArgsCount;

int LocalOffset;
// Break/Continue stack level (== LocalOffset of block)
int BStackLevel;
int CStackLevel;
int ReturnLabel;
int BreakLabel;
int ContinueLabel;

int CurrentType;
int CurrentVal;
int CurrentTypeExtra;

int IsUnsignedOp;

int BaseType;
int BaseTypeExtra;

char ReturnUsed;
enum { PENDING_PUSHAX = 1 };
int Pending; // Bit0: PENDING_PUSHAX, Rest: remaing bits: SP adjustment
int LastFixup;
int IsDeadCode;

int NextSwitchCase = -1; // Where to go if we haven't matched
int NextSwitchStmt;      // Where to go if we've matched (-1 if already emitted)
int SwitchDefault;       // Optional switch default label

unsigned char Output[OUTPUT_MAX]; // Place this last to allow partial stack overflows!

///////////////////////////////////////////////////////////////////////
// Helper functions
///////////////////////////////////////////////////////////////////////

char* CopyStr(char* dst, const char* src)
{
    while (*src) *dst++ = *src++;
    *dst = 0;
    return dst;
}

const char HexD[] = "0123456789ABCDEF";
char* CvtHex(char* dest, int n)
{
    int i;
    for (i = 3; i >= 0; --i) {
        *dest++ = HexD[(n>>i*4)&0xf];
    }
    return dest;
}

char* VSPrintf(char* dest, const char* format, va_list vl)
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
            *dest++ = va_arg(vl, int);
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
            dest = CvtHex(dest, va_arg(vl, int));
        }
    }
    *dest = 0;
    return dest;
}

void Printf(const char* format, ...)
{
    char LineBuf[80];
    va_list vl;
    va_start(vl, format);
    VSPrintf(LineBuf, format, vl);
    char* s = LineBuf;
    while (*s)
        putchar(*s++);
    va_end(vl);
}

#ifdef __SCC__
int* GetBP(void)
{
    _emit 0x8B _emit 0x46 _emit 0x00 // MOV AX, [BP]
}
#endif

void Fatal(const char* Msg)
{
#ifdef __SCC__
    int* BP = GetBP();
    Printf("\nBP   Return address\n");
    int i = 0;
    while (*BP && ++i < 10) {
        Printf("%X %X\n", BP[0], BP[1]);
        BP = (int*)BP[0];
    }
#endif
    Printf("In line %d: %s\n", Line, Msg);
    exit(1);
}

void Fail(void)
{
    Fatal("Check failed");
}

#ifndef __SCC__
#define Fail() do { Printf("In %s:%d: ", __FILE__, __LINE__); Fail(); } while (0)
#endif


///////////////////////////////////////////////////////////////////////
// Tokenizer
///////////////////////////////////////////////////////////////////////

void NextChar(void)
{
    if ((CurChar = *InBufPtr++))
        return;
    InBufPtr = InBuf;
    InBufEnd = InBuf + read(InFile, InBuf, INBUF_MAX);
    *InBufEnd = 0;
    if (InBufPtr == InBufEnd) {
        CurChar = 0;
        return;
    }
    CurChar = *InBufPtr++;
}

int TryGetChar(int ch, int t, int p)
{
    if (CurChar == ch) {
        TokenType = t;
        OperatorPrecedence = p;
        NextChar();
        return 1;
    }
    return 0;
}

int SkipWhitespace(void)
{
    for (;;) {
        if (CurChar <= ' ') {
            while ((CurChar = *InBufPtr++) <= ' ') {
                switch (CurChar) {
                case ' ':  continue;
                case '\t': continue;
                case '\n':
                    ++Line;
                    continue;
                case 0:
                    --InBufPtr;
                    NextChar();
                    if (!CurChar)
                        return 0;
                    --InBufPtr; // Re-read character instead of duplicating logic
                    continue;
                }
            }
        }
        if (CurChar > '/')
            return 0;
        switch (CurChar) {
        case '/':
            NextChar();
            switch (CurChar) {
            case '/':
            SkipLine:
                while ((CurChar = *InBufPtr++) != '\n') {
                    if (CurChar)
                        continue;
                    --InBufPtr;
                    NextChar();
                    if (CurChar == '\n')
                        break;
                    if (!CurChar)
                        return 0;
                }
                NextChar();
                ++Line;
                continue;
            case '*':
                {
                    NextChar();
                    int star = 0;
                    while (!star || CurChar != '/') {
                        star = CurChar == '*';
                        if (CurChar == '\n') ++Line;
                        NextChar();
                        if (!CurChar) Fail(); // Unterminated comment
                    }
                    NextChar();
                }
                continue;
            }
            return 1;
        case '#':
            goto SkipLine;
        }
        return 0;
    }
}

int GetDigit(void)
{
    if (isdigit(CurChar)) {
        return CurChar - '0';
    } else if (isalpha(CurChar)) {
        return (CurChar & 0xdf) - ('A'-10);
    }
    return 256;
}

char Unescape(void)
{
    int ch = CurChar;
    NextChar();
    switch (ch) {
    case 'n':  return '\n';
    case 'r':  return '\r';
    case 't':  return '\t';
    case '\'': return '\'';
    case '"':  return '"';
    case '\\': return '\\';
    case 'x':  break;
    default: Fail();
    }
    ch = GetDigit()<<4;
    NextChar();
    ch |= GetDigit();
    NextChar();
    if (ch&0xff00) Fail();
    return ch;
}

void GetStringLiteral(void)
{
    TokenStrLit = &Output[CodeAddress + 64 - CODESTART]; // Just leave a little head room for code to be outputted before consuming the string literal
    TokenNumVal = 0;

    for (;;) {
        while (CurChar != '"') {
            if (!CurChar) {
                Fatal("Unterminated string literal");
            }
            char ch = CurChar;
            NextChar();
            if (ch == '\\') {
                ch = Unescape();
            }
            TokenStrLit[TokenNumVal++] = ch;
        }
        NextChar();
        SkipWhitespace();
        if (CurChar != '"')
            break;
        NextChar();
    }
    TokenStrLit[TokenNumVal++] = 0;
    if (TokenStrLit+TokenNumVal-Output > OUTPUT_MAX) Fail();
}

const char IsIdChar[] = "\x00\x00\x00\x00\x00\x00\xFF\x03\xFE\xFF\xFF\x87\xFE\xFF\xFF\x07\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";

void GetToken(void)
{
    if (CurChar <= '/') {
        if (SkipWhitespace()) {
            TokenType = '/';
            goto Slash;
        }
    }
    TokenType = CurChar;
    if (!(CurChar = *InBufPtr++)) {
        --InBufPtr;
        NextChar();
    }
    OperatorPrecedence = PREC_STOP;

    if (TokenType < 'A') {
        if ((unsigned)(TokenType - '0') < 10) {
            TokenNumVal = TokenType - '0';
            int base = 10;
            if (!TokenNumVal) {
                base = 8;
                if (CurChar == 'x' || CurChar == 'X') {
                    base = 16;
                    NextChar();
                }
            }
            for (;;) {
                int dig = GetDigit();
                if (dig >= base) {
                    break;
                }
                TokenNumVal = TokenNumVal*base + dig;
                NextChar();
            }
            TokenType = TOK_NUM;
            return;
        }
    } else if ((TokenType & 0xDF) <= 'Z') {
    Identifier: ;
        char* start = &IdBuffer[IdBufferIndex];
        char* pc = start;
        unsigned Hash = HASHINIT*HASHMUL+(*pc++ = TokenType);
        while ((IsIdChar[(unsigned char)CurChar>>3]>>(CurChar&7)) & 1) {
            Hash = Hash*HASHMUL+(*pc++ = CurChar);
            if ((CurChar = *InBufPtr++))
                continue;
            --InBufPtr;
            NextChar();
        }
        *pc++ = 0;
        int Slot = 0;
        for (;;) {
            Hash &= ID_HASHMAX-1;
            if ((TokenType = IdHashTab[Hash]) == -1) {
                if (IdCount == ID_MAX) Fail();
                TokenType = IdHashTab[Hash] = IdCount++;
                IdText[TokenType] = start;
                IdBufferIndex += (int)(pc - start);
                if (IdBufferIndex > IDBUFFER_MAX) Fail();
                break;
            }
            if (memcmp(start, IdText[TokenType], pc - start)) {
                Hash += 1 + Slot++;
                continue;
            }
            break;
        }
        TokenType += TOK_KEYWORD;
        return;
    }

    switch (TokenType) {
    case ';':
    case '(':
    case ')':
    case '{':
    case '}':
    case '[':
    case ']':
    case ':':
    case '~':
    case 0:
        return;
    case '=':
        OperatorPrecedence = PREC_ASSIGN;
        TryGetChar('=', TOK_EQEQ, 7);
        return;
    case ',':
        OperatorPrecedence = PREC_COMMA;
        return;
    case '?':
        OperatorPrecedence = 13;
        return;
    case '!':
        TryGetChar('=', TOK_NOTEQ, 7);
        return;
    case '<':
        OperatorPrecedence = 6;
        if (TryGetChar('<', TOK_LSH, 5)) {
            TryGetChar('=', TOK_LSHEQ, PREC_ASSIGN);
        } else{
            TryGetChar('=', TOK_LTEQ, 6);
        }
        return;
    case '>':
        OperatorPrecedence = 6;
        if (TryGetChar('>', TOK_RSH, 5)) {
            TryGetChar('=', TOK_RSHEQ, PREC_ASSIGN);
        } else {
            TryGetChar('=', TOK_GTEQ, 6);
        }
        return;
    case '&':
        OperatorPrecedence = 8;
        if (!TryGetChar('&', TOK_ANDAND, 11)) {
            TryGetChar('=', TOK_ANDEQ, PREC_ASSIGN);
        }
        return;
    case '|':
        OperatorPrecedence = 10;
        if (!TryGetChar('|', TOK_OROR, 12)) {
            TryGetChar('=', TOK_OREQ, PREC_ASSIGN);
        }
        return;
    case '^':
        OperatorPrecedence = 9;
        TryGetChar('=', TOK_XOREQ, PREC_ASSIGN);
        return;
    case '+':
        OperatorPrecedence = 4;
        if (!TryGetChar('+', TOK_PLUSPLUS, PREC_STOP)) {
            TryGetChar('=', TOK_PLUSEQ, PREC_ASSIGN);
        }
        return;
    case '-':
        OperatorPrecedence = 4;
        if (!TryGetChar('-', TOK_MINUSMINUS, PREC_STOP)) {
            if (!TryGetChar('=', TOK_MINUSEQ, PREC_ASSIGN))
                TryGetChar('>', TOK_ARROW, PREC_STOP);
        }
        return;
    case '*':
        OperatorPrecedence = 3;
        TryGetChar('=', TOK_STAREQ, PREC_ASSIGN);
        return;
    case '/':
 Slash:
        OperatorPrecedence = 3;
        TryGetChar('=', TOK_SLASHEQ, PREC_ASSIGN);
        return;
    case '%':
        OperatorPrecedence = 3;
        TryGetChar('=', TOK_MODEQ, PREC_ASSIGN);
        return;
    case '_':
        goto Identifier;
    case '.':
        if (CurChar == '.') {
            NextChar();
            if (!TryGetChar('.', TOK_ELLIPSIS, PREC_STOP)) {
                break;
            }
        }
        return;
    case '\'':
        TokenNumVal = CurChar;
        NextChar();
        if (TokenNumVal == '\\') {
            TokenNumVal = Unescape();
        }
        if (CurChar != '\'') {
            Fatal("Invalid character literal");
        }
        NextChar();
        TokenType = TOK_NUM;
        return;
    case '"':
        GetStringLiteral();
        TokenType = TOK_STRLIT;
        return;
    }
    Fatal("Unknown token encountered");
}

void PrintTokenType(int T)
{
    if (T >= TOK_KEYWORD) Printf("%s ", IdText[T-TOK_KEYWORD]);
    else if (T > ' ' && T < 128) Printf("%c ", T);
    else Printf("%d ", T);
}

///////////////////////////////////////////////////////////////////////
// Code output
///////////////////////////////////////////////////////////////////////

enum {
    JO , //  0x0
    JNO, //  0x1
    JC , //  0x2
    JNC, //  0x3
    JZ , //  0x4
    JNZ, //  0x5
    JNA, //  0x6
    JA , //  0x7
    JS , //  0x8
    JNS, //  0x9
    JPE, //  0xa
    JPO, //  0xb
    JL , //  0xc
    JNL, //  0xd
    JNG, //  0xe
    JG , //  0xf
};

enum {
    R_AX, R_CX, R_DX, R_BX, R_SP, R_BP, R_SI, R_DI,
};

enum {
    MODRM_DISP16    = 0x06, // [disp16]
    MODRM_SI        = 0x04, // [SI]
    //MODRM_BX        = 0x07, // [BX]
    MODRM_BP_DISP8  = 0x46, // [BP+disp8]
    MODRM_BP_DISP16 = 0x86, // [BP+disp16]
    MODRM_REG       = 0xC0,
};

enum {
    I_ADD            = 0x00,
    I_OR             = 0x08,
    I_AND            = 0x20,
    I_SUB            = 0x28,
    I_XOR            = 0x30,
    I_CMP            = 0x38,
    I_INC            = 0x40,
    I_DEC            = 0x48,
    I_PUSH           = 0x50,
    I_POP            = 0x58,
    I_ALU_RM16_IMM16 = 0x81,
    I_ALU_RM16_IMM8  = 0x83,
    I_MOV_R_RM       = 0x88,
    I_LEA            = 0x8D,
    I_XCHG_AX        = 0x90,
    I_MOV_R_IMM16    = 0xB8,
    I_STOSB          = 0xAA,
    I_RET            = 0xC3,
    I_JMP_REL16      = 0xE9,
    I_JMP_REL8       = 0xEB,
};

enum {
    SHROT_SHL = 4,
    SHROT_SHR = 5,
    SHROT_SAR = 7,
};

int EmitChecks(void);

void Output1Byte(int b0)
{
    if (IsDeadCode|Pending) {
        if (EmitChecks()) return;
    }
    Output[CodeAddress++ - CODESTART] = b0;
}

void Output2Bytes(int b0, int b1)
{
    if (IsDeadCode|Pending) {
        if (EmitChecks()) return;
    }
    unsigned char* o = &Output[(CodeAddress += 2) - (CODESTART+2)];
    o[0] = b0;
    o[1] = b1;
}

void OutputWord(int w)
{
    if (IsDeadCode|Pending) {
        if (EmitChecks()) return;
    }
    unsigned char* o = &Output[(CodeAddress += 2) - (CODESTART+2)];
    o[0] = w;
    o[1] = w>>8;
}

void Output3Bytes(int b0, int b1, int b2)
{
    if (IsDeadCode|Pending) {
        if (EmitChecks()) return;
    }
    unsigned char* o = &Output[(CodeAddress += 3) - (CODESTART+3)];
    o[0] = b0;
    o[1] = b1;
    o[2] = b2;
}

int MakeLabel(void)
{
    if (LocalLabelCounter == LABEL_MAX) Fail();
    const int l = LocalLabelCounter++;
    memset(&Labels[l], 0, sizeof(struct Label));
    return l;
}

struct NamedLabel* GetNamedLabel(int Id)
{
    struct NamedLabel* NL = NamedLabels;
    int l = NamedLabelCount;
    for (; l--; ++NL) {
        if (NL->Id == Id) {
            return NL;
        }
    }
    if (NamedLabelCount == NAMED_LABEL_MAX) Fail();
    ++NamedLabelCount;
    NL->Id = Id;
    NL->LabelId = MakeLabel();
    return NL;
}

void DoFixups(int r, int relative)
{
    int f;
    unsigned char* c;
    while (r) {
        int o = 0;
        f = CodeAddress;
        if (relative) {
            o = r + 2;
            f -= o;
        }
        c = &Output[r - CODESTART];
        r = c[0]|c[1]<<8;
        if (!c[-1]) {
            // Fused Jcc/JMP
            if (f <= 0x7f-3) {
                // Invert condition and NOP out jump
                if ((c[-3]&0xf0) != 0x70 || c[-2] != 3) Fail();
                c[-3] ^= 1;
                c[-2] = f+3;
                // If the next instruction is a known jump / return, copy it in
                c[1]  = I_XCHG_AX;
                switch (c[2]) {
                case I_JMP_REL8:
                    if (((char *)c)[3] > 0x7f-3)
                        break;
                    c[-1] = I_JMP_REL8;
                    c[0] = c[3]+3;
                    continue;
                case I_POP|R_BP:
                    c[-1] = I_POP|R_BP;
                    c[0]  = I_RET;
                    continue;
                }
                c[-1] = I_XCHG_AX;
                c[0]  = I_XCHG_AX;
                continue;
            }
            c[-1] = I_JMP_REL16;
        } else {
            // Is this a uselss jump we can avoid? (Too cumbersome if we already resolved fixups at this address)
            if (CodeAddress == o && LastFixup != CodeAddress) {
                CodeAddress -= 3;
                continue;
            }
        }
        if (c[-1] == I_JMP_REL16) {
            ++f;
            if (f == (char)f) {
                // Convert to short jump + nop (the nop is just to not throw off disassemblers more than necessary)
                c[-1] = I_JMP_REL8;
                f = (unsigned char)f|0x9000;
            } else {
                --f;
            }
        }
        c[0] = f;
        c[1] = f>>8;
    }
    LastFixup = CodeAddress;
}

void AddFixup(int* f)
{
    if (IsDeadCode)
        return;
    OutputWord(*f);
    *f = CodeAddress - 2;
}

void FlushSpAdj(void);

void EmitLocalLabel(int l)
{
    if (l >= LocalLabelCounter || Labels[l].Addr) Fail();
    FlushSpAdj();
    IsDeadCode = 0;
    DoFixups(Labels[l].Ref, 1);
    Labels[l].Addr = CodeAddress;
    Labels[l].Ref = 0;
}

const char StaticName[] = "@static";
void EmitGlobalLabel(struct VarDecl* vd)
{
    if (vd->Offset) Fail();
    vd->Offset = CodeAddress;
    IsDeadCode = 0;
    DoFixups(vd->Ref, vd->Type & VT_FUN);

    char* Buf = &IdBuffer[IdBufferIndex];
    char* P = CvtHex(Buf, CodeAddress);
    *P++ = ' ';
    P = CopyStr(P, vd->Id == -2 ? StaticName : IdText[vd->Id]);
    *P++ = '\r';
    *P++ = '\n';
    write(MapFile, Buf, (int)(P - Buf));
}

void EmitGlobalRef(struct VarDecl* vd)
{
    int Addr = vd->Offset;
    if (Addr)
        OutputWord(Addr);
    else
        AddFixup(&vd->Ref);
}

void EmitGlobalRefRel(struct VarDecl* vd)
{
    int Addr = vd->Offset;
    if (Addr)
        OutputWord(Addr - CodeAddress - 2);
    else
        AddFixup(&vd->Ref);
}

void EmitModrm(int Inst, int R, int Loc, int Val)
{
    R <<= 3;
    switch (Loc) {
    case VT_LOCOFF:
        Output3Bytes(Inst, MODRM_BP_DISP8|R, Val);
        if (Val != (char)Val && !IsDeadCode) {
            Output[CodeAddress-(CODESTART+2)] += MODRM_BP_DISP16-MODRM_BP_DISP8;
            Output1Byte(Val >> 8);
        }
        return;
    case VT_LOCGLOB:
        Output2Bytes(Inst, MODRM_DISP16|R);
        EmitGlobalRef(&VarDecls[Val]);
        return;
    }
    if (Loc) Fail();
    Output2Bytes(Inst, MODRM_SI|R);
}

void EmitLoadAx(int Size, int Loc, int Val)
{
    switch (Loc) {
    case 0:
        Output1Byte(0xAC-1+Size); // LODS
        break;
    case VT_LOCGLOB:
        Output1Byte(0xA0-1+Size);
        EmitGlobalRef(&VarDecls[Val]);
        break;
    default:
        EmitModrm(0x8A-1+Size, R_AX, Loc, Val);
    }
}

void EmitStoreAx(int Size, int Loc, int Val)
{
    switch (Loc) {
    case 0:
        Output1Byte(I_STOSB-1+Size);
        return;
    case VT_LOCGLOB:
        Output1Byte(0xA2-1+Size);
        EmitGlobalRef(&VarDecls[Val]);
        return;
    }
    EmitModrm(I_MOV_R_RM-1+Size, R_AX, Loc, Val);
}

void EmitStoreConst(int Size, int Loc, int Val)
{
    EmitModrm(0xc6-1+Size, 0, Loc, Val);
    Output1Byte(CurrentVal);
    if (Size == 2)
        Output1Byte(CurrentVal >> 8);
}

void EmitAddRegConst(int r, int Amm)
{
    if (!Amm) return;
    int Op = I_ADD;
    if (Amm < 0) {
        Amm = -Amm;
        Op = I_SUB;
    }
    if (Amm <= 2) {
        if (Op == I_ADD)
            Op = I_INC;
        else
            Op = I_DEC;
        Op |= r;
        Output1Byte(Op);
        if (Amm > 1)
            Output1Byte(Op);
    } else if (Amm < 128) {
        Output3Bytes(I_ALU_RM16_IMM8, MODRM_REG|Op|r, Amm);
    } else {
        if (r == R_AX)
            Output1Byte(Op|5);
        else
            Output2Bytes(I_ALU_RM16_IMM16, MODRM_REG|Op|r);
        OutputWord(Amm);
    }
}

void EmitMovRR(int d, int s)
{
    Output2Bytes(I_MOV_R_RM|1, MODRM_REG|s<<3|d);
}

void EmitMovRImm(int r, int val)
{
    Output3Bytes(I_MOV_R_IMM16|r, val, val>>8);
}

void EmitScaleAx(int Scale)
{
    if (Scale & (Scale-1)) {
        EmitMovRImm(R_CX, Scale);
        Output2Bytes(0xF7, MODRM_REG|(5<<3)|R_CX); // IMUL CX
    } else {
        while (Scale >>= 1) {
            Output2Bytes(I_ADD|1, MODRM_REG);
        }
    }
}

void EmitDivCX(void)
{
    if (IsUnsignedOp) {
        Output2Bytes(I_XOR|1, 0xC0|R_DX<<3|R_DX);  // XOR DX, DX
        Output2Bytes(0xF7, MODRM_REG|(6<<3)|R_CX); // DIV CX
    } else {
        Output3Bytes(0x99, 0xF7, MODRM_REG | (7<<3) | R_CX); // CWD \ IDIV CX
    }
}

void EmitDivAxConst(int Amm)
{
    if (Amm & (Amm-1)) {
        EmitMovRImm(R_CX, Amm);
        EmitDivCX();
    } else {
        const int ModRM = IsUnsignedOp ? MODRM_REG|SHROT_SHR<<3 : MODRM_REG|SHROT_SAR<<3;
        while (Amm >>= 1) {
            Output2Bytes(0xD1, ModRM);
        }
    }
}

int EmitLocalJump(int l)
{
    if ((unsigned)l >= (unsigned)LocalLabelCounter) Fail();
    int Addr = Labels[l].Addr;
    if (Addr) {
        Addr -= CodeAddress+2;
        if (Addr == (char)Addr) {
            Output2Bytes(I_JMP_REL8, Addr);
        } else {
            Output1Byte(I_JMP_REL16);
            OutputWord(Addr-1);
        }
        return 1;
    } else {
        Output1Byte(I_JMP_REL16);
        AddFixup(&Labels[l].Ref);
        return 0;
    }
}

void EmitJmp(int l)
{
    Pending &= ~PENDING_PUSHAX;
    if (EmitChecks()) {
        return;
    }
    EmitLocalJump(l);
    IsDeadCode = 1;
}

void EmitJcc(int cc, int l)
{
    if (EmitChecks())
        return;

    int a = Labels[l].Addr;
    if (a) {
        a -= CodeAddress+2;
        if (a == (char)a) {
            Output2Bytes(0x70 | cc, a);
            return;
        }
    }

    Output2Bytes(0x70 | (cc^1), 3); // Skip jump
    if (!EmitLocalJump(l))
        Output[CodeAddress-(CODESTART+3)] = 0; // Mark as fused Jcc/JMP
}

void EmitLoadAddr(int Reg, int Loc, int Val)
{
    if (Loc == VT_LOCOFF) {
        EmitModrm(I_LEA, Reg, VT_LOCOFF, Val);
    } else if (Loc == VT_LOCGLOB) {
        Output1Byte(I_MOV_R_IMM16 | Reg);
        EmitGlobalRef(&VarDecls[Val]);
    } else {
        Fail();
    }
}

void EmitExtend(int Unsigned)
{
    if (Unsigned)
        Output2Bytes(I_XOR, MODRM_REG|4<<3|4); // XOR AH, AH
    else
        Output1Byte(0x98); // CBW
}

void FlushSpAdj(void)
{
    if (Pending & ~PENDING_PUSHAX) {
        // Preserve state of PendingPushAx and avoid emitting
        // a push before adjusting the stack
        const int Orig = Pending;
        Pending = 0;
        EmitAddRegConst(R_SP, Orig & ~PENDING_PUSHAX);
        Pending = Orig & PENDING_PUSHAX;
    }
}

void FlushPushAx(void)
{
    if (Pending & PENDING_PUSHAX) {
        Pending &= ~PENDING_PUSHAX;
        Output1Byte(I_PUSH|R_AX);
        LocalOffset -= 2;
    }
}

int EmitChecks(void)
{
    if (IsDeadCode) {
        return 1;
    }
    if (Pending) {
        FlushSpAdj();
        FlushPushAx();
    }
    if (CodeAddress > OUTPUT_MAX+CODESTART) Fail();
    return 0;
}

///////////////////////////////////////////////////////////////////////
// Parser/Codegen
///////////////////////////////////////////////////////////////////////

void Unexpected(void)
{
    PrintTokenType(TokenType);
    Fatal("Unexpected token");
}

void Expect(int type)
{
    if (TokenType == type) {
        GetToken();
        return;
    }
    PrintTokenType(type);
    Printf("expected got ");
    Unexpected();
}

int ExpectId(void)
{
    int id;
    id = TokenType;
    if (id >= TOK_ID) {
        GetToken();
        return id - TOK_KEYWORD;
    }
    Printf("Expected identifier got ");
    Unexpected();
}

struct VarDecl* LookupTypedef(void)
{
    if (TokenType >= TOK_ID) {
        int VI = VarLookup[TokenType - TOK_KEYWORD];
        if (VI >= 0) {
            struct VarDecl* VD = &VarDecls[VI];
            if (VD->Type & VT_TYPEDEF) {
                return VD;
            }
        }
    }
    return 0;
}

int IsTypeStart(void)
{
    switch (TokenType) {
    case TOK_AUTO:
    case TOK_CONST:
    case TOK_VOID:
    case TOK_CHAR:
    case TOK_INT:
    case TOK_ENUM:
    case TOK_EXTERN:
    case TOK_REGISTER:
    case TOK_SHORT:
    case TOK_SIGNED:
    case TOK_STATIC:
    case TOK_STRUCT:
    case TOK_TYPEDEF:
    case TOK_UNION:
    case TOK_UNSIGNED:
    case TOK_VOLATILE:
    case TOK_VA_LIST:
        return 1;
    }
    return LookupTypedef() != 0;
}

int SizeofType(int Type, int Extra)
{
    if (Type & (VT_PTRMASK|VT_FUNPTR))
        return 2;
    switch (Type & VT_BASEMASK) {
    case VT_INT:  return 2;
    case VT_CHAR: return 1;
    case VT_ARRAY:
        {
            struct ArrayDecl* AD = &ArrayDecls[Extra];
            if (!AD->Bound) Fail();
            return AD->Bound * SizeofType(AD->Type, AD->TypeExtra);
        }
    case VT_STRUCT:
        {
            const int IsUnion = StructDecls[Extra].Id & IS_UNION_FLAG;
            int Size = 0;
            struct StructMember* SM = StructDecls[Extra].Members;
            for (; SM; SM = SM->Next) {
                const int MSize = SizeofType(SM->Type, SM->TypeExtra);
                if (!IsUnion)
                    Size += MSize;
                else if (MSize > Size)
                    Size = MSize;
            }
            return Size;
        }
    }
    Fail();
}

int SizeofCurrentType(void)
{
    return SizeofType(CurrentType, CurrentTypeExtra);
}

void LvalToRval(void)
{
    if (CurrentType & VT_LVAL) {
        const int loc = CurrentType & VT_LOCMASK;
        CurrentType &= ~(VT_LVAL | VT_LOCMASK);

        if (CurrentType == VT_ARRAY) {
            // Decay array
            struct ArrayDecl* AD = &ArrayDecls[CurrentTypeExtra];
            CurrentType = AD->Type + VT_PTR1;
            CurrentTypeExtra = AD->TypeExtra;
            if (loc)
                EmitLoadAddr(R_AX, loc, CurrentVal);
            return;
        }

        if (CurrentType & VT_FUNPTR) {
            if (loc)
                EmitLoadAddr(R_AX, loc, CurrentVal);
            return;
        }

        const int sz = ((CurrentType&~VT_UNSIGNED) != VT_CHAR) + 1;
        if (!loc) {
            Output1Byte(I_XCHG_AX|R_SI);
        }
        EmitLoadAx(sz, loc, CurrentVal);
        if (sz == 1) {
            EmitExtend(CurrentType&VT_UNSIGNED);
            CurrentType = VT_INT;
        }
    }
}

void DoIncDecOp(int Op, int Post)
{
    if (!(CurrentType & VT_LVAL)) Fail();
    const int WordOp = ((CurrentType&(VT_BASEMASK|VT_PTRMASK)) != VT_CHAR);
    const int Loc = CurrentType & VT_LOCMASK;
    char Size = 1;
    if (CurrentType & VT_PTRMASK) {
        Size = SizeofType(CurrentType-VT_PTR1, CurrentTypeExtra);
    }
    if (!Loc) {
        Output1Byte(I_XCHG_AX|R_SI);
    }
    if (Post) {
        EmitLoadAx(1+WordOp, Loc, CurrentVal);
        if (!WordOp) {
            EmitExtend(CurrentType&VT_UNSIGNED);
        }
        if (!Loc)
            EmitAddRegConst(R_SI, -WordOp-1); // Undo increment done by LODS (for increment/decrement below)
        if (WordOp)
            CurrentType &= ~(VT_LVAL|VT_LOCMASK);
        else
            CurrentType = VT_INT;
    }
    Op = Op == TOK_MINUSMINUS;

    if (Size != 1) {
        EmitModrm(I_ALU_RM16_IMM8, Op*(I_SUB>>3), Loc, CurrentVal);
        Output1Byte(Size);
        return;
    }
    EmitModrm(0xFE|WordOp, Op, Loc, CurrentVal);
}

struct VarDecl* AddVarDeclScope(int Scope, int Id)
{
    struct VarDecl* vd = &VarDecls[Scopes[Scope]++];
    vd->Type      = CurrentType;
    vd->TypeExtra = CurrentTypeExtra;
    vd->Id        = Id;
    vd->Offset    = 0;
    vd->Ref       = 0;
    return vd;
}

struct VarDecl* AddVarDecl(int Id)
{
    const int idx = Scopes[ScopesCount-1];
    if (idx >= VARDECL_MAX) Fail();

    // Check if definition/re-declaration in global scope
    int *C = &VarLookup[Id];
    if (!IgnoreRedef) {
        if (*C >= 0) {
            return &VarDecls[*C];
        }
    }

    VarDecls[idx].Prev = *C;
    *C = idx;
    return AddVarDeclScope(ScopesCount-1, Id);
}

struct VarDecl* AddLateGlobalVar(int Id)
{
    CurrentVal = Scopes[0];
    if (CurrentVal >= Scopes[1]) Fail();
    return AddVarDeclScope(0, Id);
}

void HandlePrimaryId(int id)
{
    const int idx = VarLookup[id];
    if (idx < 0) {
        // Lookup failed. Assume function returning int.
        CurrentType = VT_LOCGLOB|VT_FUN|VT_INT;
        CurrentTypeExtra = 0;
        if (IsDeadCode) {
            CurrentVal = 0;
        } else {
            VarLookup[id] = Scopes[0];
            AddLateGlobalVar(id);
        }
        return;
    }
    struct VarDecl* vd = &VarDecls[idx];
    CurrentType      = vd->Type;
    CurrentVal       = vd->Offset;
    if (CurrentType == (VT_LOCLIT | VT_INT)) {
        return;
    }
    CurrentTypeExtra = vd->TypeExtra;
    if ((CurrentType & VT_LOCMASK) == VT_LOCGLOB) {
        CurrentVal = idx;
    }
    CurrentType |= VT_LVAL;
}

void ParseExpr(void);
void ParseAbstractDecl(void);

void HandleVarArg(void)
{
    // Handle var arg builtins
    const int func = TokenType;
    GetToken();
    Expect('(');
    int id = ExpectId();
    int vd = VarLookup[id];
    if (vd < 0 || VarDecls[vd].Type != (VT_LOCOFF|VT_CHAR|VT_PTR1)) Fail();
    const int offset = VarDecls[vd].Offset;
    if (func == TOK_VA_START) {
        Expect(',');
        id = ExpectId();
        vd = VarLookup[id];
        if (vd < 0 || (VarDecls[vd].Type & VT_LOCMASK) != VT_LOCOFF) Fail();
        EmitModrm(I_LEA, R_AX, VT_LOCOFF, VarDecls[vd].Offset);
        EmitStoreAx(2, VT_LOCOFF, offset);
    } else if (func == TOK_VA_ARG) {
        Expect(',');
        EmitLoadAx(2, VT_LOCOFF, offset);
        EmitAddRegConst(R_AX, 2);
        EmitStoreAx(2, VT_LOCOFF, offset);
        ParseAbstractDecl();
        CurrentType |= VT_LVAL;
    }
    Expect(')');
}

void ParsePrimaryExpression(void)
{
    switch (TokenType) {
    case '(':
        GetToken();
        ParseExpr();
        Expect(')');
        return;
    case TOK_NUM:
        CurrentType = VT_LOCLIT | VT_INT;
        CurrentVal  = TokenNumVal;
        GetToken();
        return;
    case TOK_STRLIT:
        if (ArrayCount == ARRAY_MAX) Fail();
        struct ArrayDecl* AD = &ArrayDecls[ArrayCount];
        AD->Bound     = TokenNumVal;
        AD->Type      = VT_CHAR;
        AD->TypeExtra = 0;
        CurrentType = VT_ARRAY | VT_LVAL;
        CurrentTypeExtra = ArrayCount++;
        if (!IsDeadCode) {
            int EndLab = -1;
            if (ScopesCount != 1) {
                Output1Byte(I_MOV_R_IMM16);
                OutputWord(CodeAddress+5);
                EmitJmp(EndLab = MakeLabel());
                IsDeadCode = 0;
            }
            while (TokenNumVal--)
                Output1Byte(*TokenStrLit++);
            if (EndLab != -1)
                EmitLocalLabel(EndLab);
        }
        GetToken();
        return;
    case TOK_VA_ARG:
    case TOK_VA_START:
    case TOK_VA_END:
        HandleVarArg();
        return;
    }
    HandlePrimaryId(ExpectId());
}

// Get current value to AX
void GetVal(void)
{
    if (CurrentType == VT_BOOL) {
        EmitMovRImm(R_AX, 0);
        Output3Bytes(0x70 | (CurrentVal^1), 1, I_INC);
        CurrentType = VT_INT;
        return;
    }

    LvalToRval();
    if (CurrentType & VT_LOCMASK) {
        if (CurrentType != (VT_INT|VT_LOCLIT)) Fail();
        CurrentType = VT_INT;
        if (CurrentVal)
            EmitMovRImm(R_AX, CurrentVal);
        else
            Output2Bytes(I_XOR|1, 0xC0);
    }
}

void HandleStructMember(void)
{
    const int MemId = ExpectId();
    if ((unsigned)CurrentTypeExtra >= (unsigned)StructCount) Fail();
    struct StructMember* SM = StructDecls[CurrentTypeExtra].Members;
    int Off = 0;
    for (; SM && SM->Id != MemId; SM = SM->Next) {
        Off += SizeofType(SM->Type, SM->TypeExtra);
    }
    if (!SM) {
        Fatal("Invalid struct member");
    }
    int Loc = CurrentType & VT_LOCMASK;
    if (Off && !(StructDecls[CurrentTypeExtra].Id & IS_UNION_FLAG)) {
        if (Loc == VT_LOCOFF) {
            CurrentVal += Off;
        } else {
            if (Loc) {
                EmitLoadAddr(R_AX, Loc, CurrentVal);
                Loc = 0;
            }
            EmitAddRegConst(R_AX, Off);
        }
    }
    CurrentType      = SM->Type | VT_LVAL | Loc;
    CurrentTypeExtra = SM->TypeExtra;
}

void ParseAssignmentExpression(void);

void ParsePostfixExpression(void)
{
    for (;;) {
        switch (TokenType) {
        case '(':
            {
                // Function call
                GetToken();
                if (!(CurrentType & VT_FUNPTR)) Fail();
                const int RetType  = CurrentType & ~(VT_FUNPTR|VT_LOCMASK|VT_LVAL);
                int RetExtra = CurrentTypeExtra;
                struct VarDecl* Func = 0;
                switch (CurrentType & VT_LOCMASK) {
                case VT_LOCGLOB:
                    Func = &VarDecls[CurrentVal];
                    RetExtra = Func->TypeExtra;
                    break;
                case 0:
                    Output1Byte(I_XCHG_AX|R_SI);
                    // Fall through
                default:
                    EmitLoadAx(2, CurrentType & VT_LOCMASK, CurrentVal);
                    Output1Byte(I_PUSH|R_AX); // TODO: Could possibly optimize as PENDING_PUSHAX
                    LocalOffset -= 2;
                }
                int ArgSize = 0;
                enum { ArgChunkSize = 8 }; // Must be power of 2
                while (TokenType != ')') {
                    if (!(ArgSize & (ArgChunkSize-1))) {
                        FlushPushAx();
                        LocalOffset  -= ArgChunkSize;
                        Pending -= ArgChunkSize;
                        if (ArgSize) {
                            // Move arguments to new stack top
                            EmitModrm(I_LEA, R_SI, VT_LOCOFF, LocalOffset + ArgChunkSize);
                            EmitModrm(I_LEA, R_DI, VT_LOCOFF, LocalOffset);
                            EmitMovRImm(R_CX, ArgSize>>1);
                            Output2Bytes(0xF3, 0xA5); // REP MOVSW
                        }
                    }
                    ParseAssignmentExpression();
                    if (CurrentType == (VT_INT|VT_LOCLIT)) {
                        EmitStoreConst(2, VT_LOCOFF, LocalOffset + ArgSize);
                    } else {
                        GetVal();
                        if ((CurrentType&~VT_UNSIGNED) != VT_INT && !(CurrentType & VT_PTRMASK)) Fail();
                        EmitStoreAx(2, VT_LOCOFF, LocalOffset + ArgSize);
                    }
                    ArgSize += 2;
                    if (TokenType == ',') {
                        GetToken();
                        continue;
                    }
                    break;
                }
                Expect(')');
                ArgSize = ((ArgSize+ArgChunkSize-1)&-ArgChunkSize);
                if (Func) {
                    Output1Byte(0xE8); // CALL re16
                    EmitGlobalRefRel(Func);
                } else {
                    EmitLoadAx(2, VT_LOCOFF, LocalOffset + ArgSize); // Can't pop if arguments were used
                    Output2Bytes(0xFF, MODRM_REG|2<<3); // CALL AX
                    ArgSize += 2;
                }
                Pending += ArgSize;
                LocalOffset  += ArgSize;
                CurrentType = RetType;
                if ((CurrentType&~VT_UNSIGNED) == VT_CHAR) {
                    CurrentType = VT_INT;
                }
                CurrentTypeExtra = RetExtra;
                continue;
            }
        case '[':
            {
                GetToken();
                struct VarDecl* GlobalArr = 0;
                if ((CurrentType & (VT_BASEMASK|VT_LOCMASK)) == (VT_ARRAY|VT_LOCGLOB)) {
                    struct ArrayDecl* AD = &ArrayDecls[CurrentTypeExtra];
                    GlobalArr = &VarDecls[CurrentVal];
                    CurrentType = AD->Type;
                    CurrentTypeExtra = AD->TypeExtra;
                } else {
                    LvalToRval();
                    if (!(CurrentType & VT_PTRMASK)) {
                        Fatal("Expected pointer");
                    }
                    CurrentType -= VT_PTR1;
                    if (!IsDeadCode) Pending |= PENDING_PUSHAX;
                }
                const int Scale    = SizeofCurrentType();
                const int ResType  = CurrentType | VT_LVAL;
                const int ResExtra = CurrentTypeExtra;
                ParseExpr();
                Expect(']');

                if (!IsDeadCode) {
                    if (CurrentType == (VT_INT|VT_LOCLIT)) {
                        if (GlobalArr) {
                            Output1Byte(I_MOV_R_IMM16|R_AX);
                            EmitGlobalRef(GlobalArr);
                        } else {
                            if (!(Pending & PENDING_PUSHAX)) Fail();
                            Pending &= ~PENDING_PUSHAX;
                        }
                        EmitAddRegConst(R_AX, CurrentVal * Scale);
                    } else {
                        LvalToRval();
                        if ((CurrentType&~VT_UNSIGNED) != VT_INT || (Pending & PENDING_PUSHAX)) Fail();
                        EmitScaleAx(Scale);
                        if (GlobalArr) {
                            Output1Byte(I_ADD|5);
                            EmitGlobalRef(GlobalArr);
                        } else {
                            Output1Byte(I_POP|R_CX);
                            LocalOffset += 2;
                            Output2Bytes(I_ADD|1, MODRM_REG|R_CX<<3);
                        }
                    }
                }
                CurrentType      = ResType;
                CurrentTypeExtra = ResExtra;
                if (CurrentType & VT_LOCMASK) Fail();
                continue;
            }
        case '.':
            GetToken();
            if ((CurrentType & ~VT_LOCMASK) != (VT_STRUCT|VT_LVAL)) Fail();
            HandleStructMember();
            continue;
        case TOK_ARROW:
            GetToken();
            LvalToRval();
            if ((CurrentType & ~VT_LOCMASK) != (VT_STRUCT|VT_PTR1)) Fail();
            CurrentType -= VT_PTR1;
            HandleStructMember();
            continue;
        case TOK_PLUSPLUS:
        case TOK_MINUSMINUS:
            {
                int Op = TokenType;
                GetToken();
                DoIncDecOp(Op, 1);
                continue;
            }
        }
        return;
    }
}

void ParseCastExpression(void);

void ToBool(void)
{
    LvalToRval();
    Output2Bytes(I_AND|1, MODRM_REG); // AND AX, AX
    CurrentType = VT_BOOL;
    CurrentVal  = JNZ;
}

void ParseUnaryExpression(void)
{
    switch (TokenType) {
    case TOK_NUM:
        break; // Fast path to ParsePrimaryExpression()
    case '-':
    case '!':
    case '*':
    case '&':
    case '~':
    case '+':
        {
            const int Op = TokenType;

            GetToken();
            ParseCastExpression();
            if (Op == '&') {
                if (!(CurrentType & VT_LVAL)) {
                    Fatal("Lvalue required for address-of operator");
                }
                if (CurrentType & VT_FUNPTR)
                    return;
                const int loc = CurrentType & VT_LOCMASK;
                if (loc) {
                    EmitLoadAddr(R_AX, loc, CurrentVal);
                }
                CurrentType = (CurrentType&~(VT_LVAL|VT_LOCMASK)) + VT_PTR1;
                return;
            }

            int IsConst = 0;
            if ((CurrentType & VT_LOCMASK) == VT_LOCLIT) {
                if (CurrentType != (VT_INT|VT_LOCLIT)) Fail();
                IsConst = 1;
            } else {
                LvalToRval();
            }
            if (Op == '-') {
                if (IsConst) {
                    CurrentVal = -CurrentVal;
                } else {
                    if ((CurrentType&~VT_UNSIGNED) != VT_INT) Fail();
                    Output2Bytes(0xF7, 0xD8); // NEG AX
                }
            } else if (Op == '!') {
                if (IsConst) {
                    CurrentVal = !CurrentVal;
                } else {
                    if (CurrentType != VT_BOOL)
                        ToBool();
                    CurrentVal ^= 1;
                }
            } else if (Op == '*') {
                if (CurrentType & VT_FUNPTR)
                    return;
                if (!(CurrentType & VT_PTRMASK)) {
                    Fatal("Pointer required for dereference");
                }
                CurrentType = (CurrentType-VT_PTR1) | VT_LVAL;
            } else if (Op == '~') {
                if (IsConst) {
                    CurrentVal = ~CurrentVal;
                } else {
                    if ((CurrentType&~VT_UNSIGNED) != VT_INT) Fail();
                    Output2Bytes(0xF7, 0xD0); // NOT AX
                }
            } else if (Op != '+') {
                Fail();
            }
            return;
        }
    case TOK_PLUSPLUS:
    case TOK_MINUSMINUS:
        {
            const int Op = TokenType;
            GetToken();
            ParseUnaryExpression();
            DoIncDecOp(Op, 0);
            return;
        }
    case TOK_SIZEOF:
        {
            GetToken();
            const int WasDead = IsDeadCode;
            IsDeadCode = 1;
            if (TokenType == '(') {
                GetToken();
                if (IsTypeStart()) {
                    ParseAbstractDecl();
                    CurrentVal = SizeofCurrentType();
                } else {
                    ParseAssignmentExpression();
                }
                Expect(')');
            } else {
                ParseUnaryExpression();
            }
            if (!IsDeadCode) Fail();
            IsDeadCode = WasDead;
            CurrentVal = SizeofCurrentType();
            CurrentType = VT_LOCLIT | VT_INT;
            return;
        }
    }
    ParsePrimaryExpression();
    ParsePostfixExpression();
}

void ParseCastExpression(void)
{
    if (TokenType == '(') {
        GetToken();
        if (IsTypeStart()) {
            ParseAbstractDecl();
            Expect(')');
            const int T = CurrentType;
            const int E = CurrentTypeExtra;
            if (T & VT_LOCMASK) Fail();
            ParseCastExpression();
            GetVal(); // TODO: could optimize some constant expressions here
            CurrentType      = T;
            CurrentTypeExtra = E;
            if ((CurrentType&~VT_UNSIGNED) == VT_CHAR) {
                EmitExtend(CurrentType&VT_UNSIGNED);
                CurrentType = VT_INT;
            }
        } else {
            ParseExpr();
            Expect(')');
            ParsePostfixExpression();
        }
    } else {
        ParseUnaryExpression();
    }
}

int RemoveAssign(int Op)
{
    switch (Op) {
    case TOK_PLUSEQ:  return '+';
    case TOK_MINUSEQ: return '-';
    case TOK_STAREQ:  return '*';
    case TOK_SLASHEQ: return '/';
    case TOK_MODEQ:   return '%';
    case TOK_LSHEQ:   return TOK_LSH;
    case TOK_RSHEQ:   return TOK_RSH;
    case TOK_ANDEQ:   return '&';
    case TOK_XOREQ:   return '^';
    case TOK_OREQ:    return '|';
    }
    Fail();
}

int GetSimpleALU(int Op)
{
    switch (Op) {
    case '+':       return I_ADD|1;
    case '-':       return I_SUB|1;
    case '&':       return I_AND|1;
    case '^':       return I_XOR|1;
    case '|':       return I_OR|1;
    case '<':       return IsUnsignedOp ? I_CMP|1|JC<<8  : I_CMP|1|JL<<8;
    case TOK_LTEQ:  return IsUnsignedOp ? I_CMP|1|JNA<<8 : I_CMP|1|JNG<<8;
    case '>':       return IsUnsignedOp ? I_CMP|1|JA<<8  : I_CMP|1|JG<<8;
    case TOK_GTEQ:  return IsUnsignedOp ? I_CMP|1|JNC<<8 : I_CMP|1|JNL<<8;
    case TOK_EQEQ:  return I_CMP|1|JZ<<8;
    case TOK_NOTEQ: return I_CMP|1|JNZ<<8;
    }
    return 0;
}

void FinishOp(int Inst)
{
    if (Inst >>= 8) {
        CurrentType = VT_BOOL;
        CurrentVal  = Inst;
    }
}

// Emit: AX <- AX 'OP' CX, Must preserve BX.
void DoBinOp(int Op)
{
    int Inst = GetSimpleALU(Op);
    int RM = MODRM_REG|R_CX<<3;
    if (Inst)
        goto HasInst;
    if (Op == '*') {
        // IMUL : F7 /5
        Inst = 0xF7;
        RM = MODRM_REG | (5<<3) | R_CX;
    } else if (Op == '/' || Op == '%') {
        EmitDivCX();
        if (Op == '%') {
            Output1Byte(I_XCHG_AX|R_DX);
        }
        return;
    } else if (Op == TOK_LSH) {
        Inst = 0xd3;
        RM = MODRM_REG|SHROT_SHL<<3;
    } else if (Op == TOK_RSH) {
        Inst = 0xd3;
        if (IsUnsignedOp)
            RM = MODRM_REG|SHROT_SHR<<3;
        else
            RM = MODRM_REG|SHROT_SAR<<3;
    } else {
        Fail();
    }
HasInst:
    Output2Bytes(Inst, RM);
    FinishOp(Inst);
}

int DoRhsLvalBinOp(int Op)
{
    const int Loc = CurrentType & VT_LOCMASK;
    CurrentType &= ~(VT_LOCMASK|VT_LVAL);
    int Inst = GetSimpleALU(Op);
    if (!Inst) {
        Output1Byte(I_XCHG_AX|R_CX);
        EmitLoadAx(2, Loc, CurrentVal);
        return 0;
    }
    EmitModrm(Inst|3, R_AX, Loc, CurrentVal);
    FinishOp(Inst);
    return 1;
}

void DoRhsConstBinOp(int Op)
{
    switch (Op) {
    case '+': EmitAddRegConst(R_AX, CurrentVal); return;
    case '-': EmitAddRegConst(R_AX, -CurrentVal); return;
    case '*': EmitScaleAx(CurrentVal); return;
    case '/': EmitDivAxConst(CurrentVal); return;
    }

    int Inst = GetSimpleALU(Op);
    if (Inst) {
        Output3Bytes(Inst|5, CurrentVal, CurrentVal>>8);
        FinishOp(Inst);
    } else {
        EmitMovRImm(R_CX, CurrentVal);
        DoBinOp(Op);
    }
}

int DoConstBinOp(int Op, int L, int R)
{
    switch (Op) {
    case '+':     return L + R;
    case '-':     return L - R;
    case '*':     return L * R;
    case '/':     return L / R;
    case '%':     return L % R;
    case '&':     return L & R;
    case '^':     return L ^ R;
    case '|':     return L | R;
    case TOK_LSH: return L << R;
    case TOK_RSH: return L >> R;
    }
    Fail();
}

int OpCommutes(int Op)
{
    switch (Op) {
    case '+':
    case '*':
    case TOK_EQEQ:
    case TOK_NOTEQ:
    case '&':
    case '^':
    case '|':
        return 1;
    }
    return 0;
}

void DoCondHasExpr(int Label, int Forward) // forward => jump if label is false
{
    if (CurrentType == (VT_LOCLIT|VT_INT)) {
        // Constant condition
        if (!CurrentVal && Forward)
            EmitJmp(Label);
        return;
    }

    if (CurrentType != VT_BOOL)
        ToBool();
    EmitJcc(CurrentVal^Forward, Label);
}


void DoCond(int Label, int Forward) // forward => jump if label is false
{
    ParseExpr();
    DoCondHasExpr(Label, Forward);
}

void ForceToReg(void)
{
    // Always decay arrays
    if ((CurrentType & VT_BASEMASK) == VT_ARRAY)
        LvalToRval();

    const int Loc = CurrentType & VT_LOCMASK;
    if (!Loc) return;
    if (Loc == VT_LOCGLOB || Loc == VT_LOCOFF) {
        EmitLoadAddr(R_AX, Loc, CurrentVal);
        CurrentType &= ~VT_LOCMASK;
    } else {
        GetVal();
    }
}

void ParseMaybeDead(int Live)
{
    if (Live) {
        ParseAssignmentExpression();
    } else {
        const int OldType  = CurrentType;
        const int OldVal   = CurrentVal;
        const int OldExtra = CurrentTypeExtra;
        const int WasDead  = IsDeadCode;
        IsDeadCode = 1;
        ParseAssignmentExpression();
        if (!IsDeadCode) Fail();
        IsDeadCode       = WasDead;
        CurrentType      = OldType;
        CurrentVal       = OldVal;
        CurrentTypeExtra = OldExtra;
    }
}

void HandleCondOp(void)
{
    if (CurrentType == (VT_LOCLIT|VT_INT)) {
        // Handle potentially constant conditions
        const int V = CurrentVal;
        ParseMaybeDead(V);
        Expect(':');
        ParseMaybeDead(!V);
        return;
    }

    const int FalseLabel = MakeLabel();
    const int EndLabel = MakeLabel();
    DoCondHasExpr(FalseLabel, 1);
    ParseAssignmentExpression();
    ForceToReg();
    const int LhsType = CurrentType;
    EmitJmp(EndLabel);
    Expect(':');
    EmitLocalLabel(FalseLabel);
    ParseAssignmentExpression();
    ForceToReg();
    // Check if type unification is needed.
    // This is certainly the case if only one side is an lvalue
    // or if the types are "too different".
    int TypeDiff = LhsType ^ CurrentType;
    if ((TypeDiff & VT_LVAL) || ((TypeDiff & VT_BASEMASK) && !(CurrentType & VT_PTRMASK))) {
        const int RealEnd = MakeLabel();
        LvalToRval();
        EmitJmp(RealEnd);
        EmitLocalLabel(EndLabel);
        CurrentType = LhsType;
        LvalToRval();
        EmitLocalLabel(RealEnd);
    } else {
        EmitLocalLabel(EndLabel);
    }
}


void HandleLhsLvalLoc(int LhsLoc)
{
    if (!LhsLoc) {
        if (Pending & PENDING_PUSHAX) {
            Pending &= ~PENDING_PUSHAX;
            Output1Byte(I_XCHG_AX|R_DI);
        } else {
            Output1Byte(I_POP|R_DI);
            LocalOffset += 2;
        }
    } else {
        if (LhsLoc != VT_LOCOFF && LhsLoc != VT_LOCGLOB) Fail();
    }
}

void ParseExpr1(int OuterPrecedence)
{
    int LEnd;
    int Temp;
    int Op;
    int Prec;
    int LhsType;
    int LhsTypeExtra;
    int LhsVal;
    int LhsLoc;
    do {
        Op   = TokenType;
        Prec = OperatorPrecedence;
        GetToken();

        if (Op == '?') {
            HandleCondOp();
            continue;
        }

        if (Prec == PREC_ASSIGN) {
            if (!(CurrentType & VT_LVAL)) {
                Fatal("L-value required");
            }
            CurrentType &= ~VT_LVAL;
            if (Op != '=')
                Op = RemoveAssign(Op);
        } else {
            LvalToRval();
        }

        Temp = Op == TOK_OROR;
        if (Temp || Op == TOK_ANDAND) {
            LEnd = MakeLabel();
            if (CurrentType != VT_BOOL)
                ToBool();
            EmitMovRImm(R_AX, Temp);
            EmitJcc(CurrentVal ^ !Temp, LEnd);
        } else {
            LEnd = -1;
            if (CurrentType == VT_BOOL)
                GetVal();
            if (!(CurrentType & VT_LOCMASK) && Op != ',' && !IsDeadCode) {
                Pending |= PENDING_PUSHAX;
            }
        }
        LhsType        = CurrentType;
        LhsTypeExtra   = CurrentTypeExtra;
        LhsVal         = CurrentVal;

        ParseCastExpression(); // RHS
        for (;;) {
            if (OperatorPrecedence > Prec || (OperatorPrecedence == Prec && OperatorPrecedence != PREC_ASSIGN)) // Precedence == PREC_ASSIGN <=> IsRightAssociative
                break;
            ParseExpr1(OperatorPrecedence);
        }

        if (Op == ',') {
            continue;
        }

        LhsLoc = LhsType & VT_LOCMASK;
        LhsType &= ~VT_LOCMASK;

        if (LhsLoc == VT_LOCLIT && CurrentType == (VT_LOCLIT|VT_INT)) {
            if (LhsType != VT_INT) Fail();
            CurrentVal = DoConstBinOp(Op, LhsVal, CurrentVal);
            continue;
        }

        if (LEnd >= 0) {
            if (CurrentType != VT_BOOL)
                ToBool();
            if (LhsVal != CurrentVal)
                GetVal();
            EmitLocalLabel(LEnd);
            continue;
        }

        IsUnsignedOp = LhsType == (VT_UNSIGNED|VT_INT) || (CurrentType&(VT_UNSIGNED|VT_PTRMASK|VT_BASEMASK)) == (VT_UNSIGNED|VT_INT);

        if (Prec == PREC_ASSIGN) {
            HandleLhsLvalLoc(LhsLoc);
            if (LhsType == VT_STRUCT) {
                // Struct assignment
                if ((CurrentType&~VT_LOCMASK) != (VT_STRUCT|VT_LVAL) || CurrentTypeExtra != LhsTypeExtra)
                    Fail();
                Temp = CurrentType & VT_LOCMASK;

                if (LhsLoc)
                    EmitLoadAddr(R_DI, LhsLoc, LhsVal);
                if (Temp)
                    EmitLoadAddr(R_SI, Temp, CurrentVal);
                else
                    Output1Byte(I_XCHG_AX|R_SI);
                EmitMovRImm(R_CX, SizeofCurrentType());
                Output2Bytes(0xF3, 0xA4); // REP MOVSB
                continue;
            }

            LvalToRval();
            int Size = 2;
            if ((LhsType&~VT_UNSIGNED) == VT_CHAR) {
                Size = 1;
            }
            if (Op != '=') {
                if (!LhsLoc)
                    EmitMovRR(R_SI, R_DI); // TODO: Can sometimes be avoided
                Temp = CurrentType == (VT_INT|VT_LOCLIT);
                if (Size == 2) {
                    int Inst = GetSimpleALU(Op);
                    if (Inst) {
                        if (Temp) {
                            EmitModrm(I_ALU_RM16_IMM16, Inst>>3, LhsLoc, LhsVal);
                            OutputWord(CurrentVal);
                        } else {
                            GetVal();
                            EmitModrm(Inst, R_AX, LhsLoc, LhsVal);
                        }
                        if (!LhsLoc)
                            Output1Byte(I_XCHG_AX|R_DI);
                        CurrentType = LhsType | LhsLoc | VT_LVAL;
                        CurrentVal = LhsVal;
                        continue;
                    }
                }
                if (!Temp) {
                    if ((CurrentType&~VT_UNSIGNED) != VT_INT) Fail();
                    Output1Byte(I_XCHG_AX|R_CX);
                }
                EmitLoadAx(Size, LhsLoc, LhsVal);
                if (Size == 1) { 
                    EmitExtend(LhsType&VT_UNSIGNED);
                }
                if (Temp) {
                    DoRhsConstBinOp(Op);
                    CurrentType &= ~VT_LOCMASK;
                } else {
                    DoBinOp(Op);
                }
            } else if (CurrentType == (VT_INT|VT_LOCLIT)) {
                // Constant assignment
                if (!LhsLoc) {
                    GetVal();
                    Output1Byte(I_STOSB-1+Size);
                } else {
                    EmitStoreConst(Size, LhsLoc, LhsVal);
                }
                continue;
            } else {
                GetVal();
            }
            EmitStoreAx(Size, LhsLoc, LhsVal);
            continue;
        }
        int LhsPointeeSize = 0;
        if (LhsType & VT_PTRMASK) {
            LhsPointeeSize = SizeofType(LhsType-VT_PTR1, LhsTypeExtra);
            CurrentTypeExtra = LhsTypeExtra;
            IsUnsignedOp = 1;
        }
        if (CurrentType == (VT_LOCLIT|VT_INT)) {
            if (!((Pending&PENDING_PUSHAX)|IsDeadCode)) Fail();
            Pending &= ~PENDING_PUSHAX;
            CurrentType = VT_INT;
            if (LhsType & VT_PTRMASK) {
                CurrentVal *= LhsPointeeSize;
                CurrentType = LhsType;
            }
RhsConst:
            DoRhsConstBinOp(Op);
        } else {
            Temp = OpCommutes(Op);
            if (LhsLoc == VT_LOCLIT) {
                if (LhsType != VT_INT) Fail();
                GetVal();
                if (Temp) {
                    CurrentVal = LhsVal;
                    goto RhsConst;
                } else {
                    EmitMovRImm(R_CX, LhsVal);
                }
            } else {
                if ((LhsType&~VT_UNSIGNED) != VT_INT && !LhsPointeeSize) Fail();
                if (Op == '+' && LhsPointeeSize) {
                    GetVal();
                    EmitScaleAx(LhsPointeeSize);
                    CurrentType = LhsType;
                } else if ((Pending & PENDING_PUSHAX) && (CurrentType & VT_BASEMASK) != VT_ARRAY) {
                    if ((CurrentType&(VT_BASEMASK|VT_PTRMASK)) != VT_CHAR) {
                        if (!(CurrentType&VT_LVAL)) Fail();
                        Pending &= ~PENDING_PUSHAX;
                        if (!DoRhsLvalBinOp(Op))
                            goto DoNormal;
                        goto CheckSub;
                    }
                }
                GetVal();
                if (Pending & PENDING_PUSHAX) Fail();
                Output1Byte(I_POP|R_CX);
                LocalOffset += 2;
            }
DoNormal:
            if (!Temp)
                Output1Byte(I_XCHG_AX|R_CX);
            DoBinOp(Op);
        }
CheckSub:
        if (Op == '-' && LhsPointeeSize) {
            EmitDivAxConst(LhsPointeeSize);
            CurrentType = VT_INT;
        }
    } while (OperatorPrecedence <= OuterPrecedence);
}

void ParseExpr(void)
{
    ParseCastExpression();
    if (OperatorPrecedence <= PREC_COMMA)
        ParseExpr1(PREC_COMMA);
}

void ParseAssignmentExpression(void)
{
    ParseCastExpression();
    if (OperatorPrecedence <= PREC_ASSIGN)
        ParseExpr1(PREC_ASSIGN);
}

void SaveBaseType(void)
{
    BaseType      = CurrentType & ~VT_PTRMASK;
    BaseTypeExtra = CurrentTypeExtra;
    if ((CurrentType & VT_BASEMASK) == VT_ARRAY) {
        struct ArrayDecl* AD = &ArrayDecls[CurrentTypeExtra];
        BaseType      = AD->Type;
        BaseTypeExtra = AD->TypeExtra;
    }
}

void ParseDeclarator(int* Id);

void ParseDeclSpecs(void)
{
    int Flags = 0;
    // Could check if legal type, but we want to keep the code small...
    CurrentType = VT_INT;
    for (;;) {
        switch (TokenType) {
        case TOK_VOID:
            CurrentType = VT_VOID;
            break;
        case TOK_CHAR:
            CurrentType = VT_CHAR;
            break;
        case TOK_SHORT:
        case TOK_INT:
            CurrentType = VT_INT;
            break;
        case TOK_VA_LIST:
            CurrentType = VT_CHAR | VT_PTR1;
            break;
        case TOK_AUTO:
        case TOK_CONST:
        case TOK_EXTERN: // Should only really be allowed in global scope
        case TOK_REGISTER:
        case TOK_VOLATILE:
            // Ignore but accept for now
            break;
        case TOK_STATIC:
            Flags |= VT_STATIC;
            break;
        case TOK_SIGNED:
            Flags &= ~VT_UNSIGNED;
            break;
        case TOK_TYPEDEF:
            Flags |= VT_TYPEDEF;
            break;
        case TOK_UNSIGNED:
            Flags |= VT_UNSIGNED;
            break;
        case TOK_ENUM:
            GetToken();
            if (TokenType >= TOK_ID) {
                // TODO: Store and use the enum identifier
                GetToken();
            }
            if (TokenType == '{') {
                GetToken();
                ++IgnoreRedef;
                int EnumVal = 0;
                while (TokenType != '}') {
                    const int id = ExpectId();
                    if (TokenType == '=') {
                        GetToken();
                        ParseAssignmentExpression();
                        if (CurrentType != (VT_INT|VT_LOCLIT)) Fail();
                        EnumVal = CurrentVal;
                    }
                    CurrentType = VT_INT|VT_LOCLIT;
                    struct VarDecl* vd = AddVarDecl(id);
                    vd->Offset = EnumVal;
                    if (TokenType == ',') {
                        GetToken();
                        ++EnumVal;
                        continue;
                    }
                    break;
                }
                Expect('}');
                --IgnoreRedef;
            }
            CurrentType = VT_INT;
            continue;
        case TOK_STRUCT:
        case TOK_UNION:
            {
                int id = 0;
                if (TokenType == TOK_UNION)
                    id = IS_UNION_FLAG;
                GetToken();
                CurrentTypeExtra = -1;
                if (TokenType >= TOK_ID) {
                    id |= ExpectId();
                    int i;
                    for (i = StructCount - 1; i >= 0; --i) {
                        if (StructDecls[i].Id == id) {
                            CurrentTypeExtra = i;
                            break;
                        }
                    }
                } else {
                    id |= ID_MAX; // Should never match in above loop
                }
                if (TokenType == '{') {
                    GetToken();
                    if (StructCount == STRUCT_MAX) Fail();
                    const int SI = StructCount++;
                    struct StructDecl* SD = &StructDecls[SI];
                    SD->Id = id;
                    struct StructMember** Last = &SD->Members;
                    while (TokenType != '}') {
                        /// XXX Issue here - Save Base type before this?
                        ParseDeclSpecs();
                        SaveBaseType();
                        while (TokenType != ';') {
                            if (StructMemCount == STRUCT_MEMBER_MAX) Fail();
                            struct StructMember* SM = &StructMembers[StructMemCount++];
                            ParseDeclarator(&SM->Id);
                            if (SM->Id < 0) Fail();
                            SM->Type      = CurrentType;
                            SM->TypeExtra = CurrentTypeExtra;
                            *Last = SM;
                            Last = &SM->Next;
                            if (TokenType == ',') {
                                GetToken();                                
                                CurrentType      = BaseType;
                                CurrentTypeExtra = BaseTypeExtra;
                                continue;
                            }
                            break;
                        }
                        Expect(';');
                    }
                    GetToken();
                    *Last = 0;
                    CurrentTypeExtra = SI;
                }
                CurrentType = VT_STRUCT;
            }
            continue;
        default:
            {
                struct VarDecl* Typedef = LookupTypedef();
                if (Typedef) {
                    CurrentType      = Typedef->Type & ~VT_TYPEDEF;
                    CurrentTypeExtra = Typedef->TypeExtra;
                    break;
                }
                CurrentType |= Flags;
                return;
            }
        }
        GetToken();
    }
}

void ParseDeclarator(int* Id)
{
Redo:
    switch (TokenType) {
    case '*':
        GetToken();
        CurrentType += VT_PTR1;
        goto Redo;
    case TOK_CONST:
    case TOK_VOLATILE:
        // Ignore for now
        GetToken();
        goto Redo;
    }

    int IsFunPtr = 0;
    if (TokenType == '(') {
        // Hacks
        GetToken();
        Expect('*');
        CurrentType |= VT_FUNPTR;
        IsFunPtr = 1;
    }

    if (Id) {
        if (TokenType >= TOK_ID) {
            *Id = TokenType - TOK_KEYWORD;
            GetToken();
        } else {
            *Id = -1;
        }
    }

    if (IsFunPtr) {
        // More hacks
        Expect(')');
    }

    if (TokenType == '(') {
        GetToken();
        const int T = CurrentType | VT_FUN;
        const int E = CurrentTypeExtra;

        ArgsCount = 0;
        while (TokenType != ')') {
            if (TokenType == TOK_ELLIPSIS) {
                GetToken();
                break;
            }
            ParseDeclSpecs();
            if (TokenType == ')')
                break;
            if (ArgsCount == ARGS_MAX) Fail();
            struct Argument* A = &Args[ArgsCount++];
            ParseDeclarator(&A->Id);
            A->Type      = CurrentType;
            A->TypeExtra = CurrentTypeExtra;
            if (TokenType == ',') {
                GetToken();
                continue;
            }
            break;
        }
        Expect(')');
        CurrentType      = T;
        CurrentTypeExtra = E;
        return;
    }

    while (TokenType == '[') {
        GetToken();
        if (ArrayCount == ARRAY_MAX) Fail();
        struct ArrayDecl* AD = &ArrayDecls[ArrayCount];
        const int F = CurrentType & VT_STATIC;
        AD->Type      = CurrentType & ~VT_STATIC;
        AD->TypeExtra = CurrentTypeExtra;
        if (TokenType != ']') {
            ParseExpr();
            if (CurrentType != (VT_INT|VT_LOCLIT)) Fail(); // Need constant expression
            AD->Bound = CurrentVal;
            if (AD->Bound <= 0) Fail();
        } else {
            AD->Bound = 0;
        }
        Expect(']');
        CurrentType      = VT_ARRAY | F;
        CurrentTypeExtra = ArrayCount++;

        // Move bounds "inwards"
        // char a[2][3][4] goes
        //    (char,[])
        // -> (array{0},[[char,2]])
        // -> (array{1},[[char,3],[array{0},2]])
        // -> (array{2},[[char,4],[array{0},3],[array{1},2]])
        while (AD->Type == VT_ARRAY) {
            struct ArrayDecl* AD2 = &ArrayDecls[AD->TypeExtra];
            const int Temp = AD->Bound;
            AD->Bound = AD2->Bound;
            AD2->Bound = Temp;
            AD = AD2;
        }
    }
}

void ParseAbstractDecl(void)
{
    ParseDeclSpecs();
    ParseDeclarator(0);
}

struct VarDecl* DoDecl(void)
{
    int Id;
    ParseDeclarator(&Id);
    if (Id < 0) Fail();
    return AddVarDecl(Id);
}

struct VarDecl* ParseFirstDecl(void)
{
    ParseDeclSpecs();
    if (TokenType == ';')
        return 0;
    struct VarDecl* VD = DoDecl();
    SaveBaseType();
    return VD;
}

// Remember to reset CurrentType/CurrentTypeExtra before calling
struct VarDecl* NextDecl(void)
{
    if (TokenType != ',')
        return 0;
    GetToken();
    CurrentType      = BaseType;
    CurrentTypeExtra = BaseTypeExtra;
    return DoDecl();
}

void ParseCompoundStatement(void);
void ParseStatement(void);

void DoLoopStatements(int BLabel, int CLabel)
{
    const int OldBreak    = BreakLabel;
    const int OldContinue = ContinueLabel;
    const int OldBStack   = BStackLevel;
    const int OldCStack   = CStackLevel;
    BreakLabel    = BLabel;
    ContinueLabel = CLabel;
    BStackLevel   = LocalOffset;
    CStackLevel   = LocalOffset;
    ParseStatement();
    BStackLevel   = OldBStack;
    CStackLevel   = OldCStack;
    BreakLabel    = OldBreak;
    ContinueLabel = OldContinue;
}

void EnsureSwitchStmt(void)
{
    if (NextSwitchStmt < 0) {
        // Continue execution after the case
        NextSwitchStmt = MakeLabel();
        EmitJmp(NextSwitchStmt);
    }
}

void ParseStatement(void)
{
Redo:
    // Get compund statements out of the way to simplify switch handling
    if (TokenType == '{') {
        ParseCompoundStatement();
        return;
    }

    if (NextSwitchCase >= 0) {
        // switch is active
        // NextSwitchStmt == -1 ? Handling statements : handling cases

        switch (TokenType) {
        case TOK_CASE:
            GetToken();
            EnsureSwitchStmt();
            for (;;) {
                ParseExpr();
                Expect(':');
                if (CurrentType != (VT_INT|VT_LOCLIT)) Fail(); // Need constant expression
                EmitLocalLabel(NextSwitchCase);
                NextSwitchCase = MakeLabel();
                Output3Bytes(I_CMP|5, CurrentVal, CurrentVal>>8);
                if (TokenType != TOK_CASE)
                    break;
                GetToken();
                EmitJcc(JZ, NextSwitchStmt);
            }
            EmitJcc(JNZ, NextSwitchCase);
            goto Stmt;
        case TOK_DEFAULT:
            GetToken();
            Expect(':');
            if (SwitchDefault != -1) Fail();
            EnsureSwitchStmt();
            SwitchDefault = NextSwitchStmt;
            goto Redo;
        }
        if (NextSwitchStmt >= 0) {
            EmitJmp(NextSwitchCase);
        Stmt:
            EmitLocalLabel(NextSwitchStmt);
            NextSwitchStmt = -1;
            goto Redo;
        }
    }

    switch (TokenType) {
    case ';':
        GetToken();
        return;
    case TOK_RETURN:
        GetToken();
        if (TokenType != ';') {
            ParseExpr();
            GetVal();
        }
        if (LocalOffset) {
            ReturnUsed = 1;
            EmitJmp(ReturnLabel);
        } else {
            Output2Bytes(I_POP|R_BP, I_RET);
            IsDeadCode = 1;
        }
    GetSemicolon:
        Expect(';');
        return;
    case TOK_BREAK:
        GetToken();
        Pending += BStackLevel - LocalOffset;
        EmitJmp(BreakLabel);
        goto GetSemicolon;
    case TOK_CONTINUE:
        GetToken();
        Pending += CStackLevel - LocalOffset;
        EmitJmp(ContinueLabel);
        goto GetSemicolon;
    case TOK_GOTO:
        GetToken();
        EmitJmp(GetNamedLabel(ExpectId())->LabelId);
        goto GetSemicolon;
    case TOK__EMIT:
        GetToken();
        ParseExpr();
        if (CurrentType != (VT_INT|VT_LOCLIT)) Fail(); // Constant expression expected
        Output1Byte(CurrentVal & 0xff);
        return;
    case TOK_IF:
        {
            GetToken();
            const int ElseLabel = MakeLabel();
            Expect('(');
            DoCond(ElseLabel, 1);
            Expect(')');
            ParseStatement();
            if (TokenType == TOK_ELSE) {
                GetToken();
                const int EndLabel  = MakeLabel();
                EmitJmp(EndLabel);
                EmitLocalLabel(ElseLabel);
                ParseStatement();
                EmitLocalLabel(EndLabel);
            } else {
                EmitLocalLabel(ElseLabel);
            }
        }
        return;
    case TOK_FOR:
        {
            GetToken();
            const int CondLabel = MakeLabel();
            const int BodyLabel = MakeLabel();
            const int EndLabel  = MakeLabel();
            int IterLabel       = CondLabel;
        
            Expect('(');

            // Init
            if (TokenType != ';') {
                ParseExpr();
            }
            Expect(';');

            // Cond
            EmitLocalLabel(CondLabel);
            if (TokenType != ';') {
                DoCond(EndLabel, 1);
            }
            Expect(';');

            // Iter
            if (TokenType != ')') {
                EmitJmp(BodyLabel);
                IterLabel  = MakeLabel();
                EmitLocalLabel(IterLabel);
                ParseExpr();
                EmitJmp(CondLabel);
            }
            Expect(')');

            EmitLocalLabel(BodyLabel);
            DoLoopStatements(EndLabel, IterLabel);
            EmitJmp(IterLabel);
            EmitLocalLabel(EndLabel);
        }
        return;
    case TOK_WHILE:
        {
            GetToken();
            const int StartLabel = MakeLabel();
            const int EndLabel   = MakeLabel();
            EmitLocalLabel(StartLabel);
            Expect('(');
            DoCond(EndLabel, 1);
            Expect(')');
            DoLoopStatements(EndLabel, StartLabel);
            EmitJmp(StartLabel);
            EmitLocalLabel(EndLabel);
        }
        return;
    case TOK_DO:
        {
            GetToken();
            const int StartLabel = MakeLabel();
            const int CondLabel  = MakeLabel();
            const int EndLabel   = MakeLabel();
            EmitLocalLabel(StartLabel);
            DoLoopStatements(EndLabel, CondLabel);
            EmitLocalLabel(CondLabel);
            Expect(TOK_WHILE);
            Expect('(');
            DoCond(StartLabel, 0);
            Expect(')');            
            EmitLocalLabel(EndLabel);
            goto GetSemicolon;
        }
    case TOK_SWITCH:
        {
            GetToken();
            Expect('(');
            ParseExpr();
            GetVal();
            Expect(')');

            const int OldBreakLevel  = BStackLevel;
            const int OldBreakLabel  = BreakLabel;
            const int LastSwitchCase = NextSwitchCase;
            const int LastSwitchStmt = NextSwitchStmt;
            const int LastSwitchDef  = SwitchDefault;
            BStackLevel    = LocalOffset;
            BreakLabel     = MakeLabel();
            NextSwitchCase = MakeLabel();
            NextSwitchStmt = MakeLabel();
            SwitchDefault  = -1;
            ParseStatement();
            if (SwitchDefault >= 0) {
                if (NextSwitchStmt < 0) {
                    // We exited the switch by falling through, make sure we don't go to the default case
                    NextSwitchStmt = MakeLabel();
                    EmitJmp(NextSwitchStmt);
                }
                EmitLocalLabel(NextSwitchCase);
                EmitJmp(SwitchDefault);
            } else {
                EmitLocalLabel(NextSwitchCase);
            }
            if (NextSwitchStmt >= 0)
                EmitLocalLabel(NextSwitchStmt);
            EmitLocalLabel(BreakLabel);

            NextSwitchCase = LastSwitchCase;
            NextSwitchStmt = LastSwitchStmt;
            SwitchDefault  = LastSwitchDef;
            BStackLevel    = OldBreakLevel;
            BreakLabel     = OldBreakLabel;
        }
        return;
    }

    if (IsTypeStart()) {
        struct VarDecl* vd = ParseFirstDecl();
        while (vd) {
            int size = SizeofCurrentType();

            if (CurrentType & VT_STATIC) {
                vd->Type = (CurrentType & ~VT_STATIC) | VT_LOCGLOB;
                BssSize += size;
            } else {
                vd->Type |= VT_LOCOFF;
                size = (size+1)&-2;
                if (TokenType == '=') {
                    GetToken();
                    ParseAssignmentExpression();
                    GetVal();
                    Output1Byte(I_PUSH|R_AX);
                } else {
                    Pending -= size;
                }
                LocalOffset -= size;
                vd->Offset = LocalOffset;
            }
            vd = NextDecl();
        }
    } else if (TokenType >= TOK_ID) {
        // Expression statement / Labelled statement
        const int id = TokenType - TOK_KEYWORD;
        GetToken();
        if (TokenType == ':') {
            GetToken();
            EmitLocalLabel(GetNamedLabel(id)->LabelId);
            EmitModrm(I_LEA, R_SP, VT_LOCOFF, LocalOffset);
            goto Redo;
        } else {
            HandlePrimaryId(id);
            ParsePostfixExpression();
            if (OperatorPrecedence <= PREC_COMMA)
                ParseExpr1(PREC_COMMA);
        }
    } else {
        ParseExpr();
    }
    goto GetSemicolon;
}

void PushScope(void)
{
    if (ScopesCount == SCOPE_MAX) Fail();
    Scopes[ScopesCount] = Scopes[ScopesCount-1];
    ++ScopesCount;
    ++IgnoreRedef;
}

void PopScope(void)
{
    if (ScopesCount == 1) Fail();
    --ScopesCount;
    --IgnoreRedef;
    int i = Scopes[ScopesCount-1], e = Scopes[ScopesCount];
    for (; i < e; ++i) {
        struct VarDecl* vd = &VarDecls[i];
        if (vd->Id < 0)
            continue;
        if (vd->Ref) {
            // Move static variable for real to global scope
            CurrentType = vd->Type;
            CurrentTypeExtra = vd->TypeExtra;
            AddLateGlobalVar(-2)->Ref = vd->Ref;
        }
        VarLookup[vd->Id] = vd->Prev;
    }
}

void ParseCompoundStatement(void)
{
    const int OldOffset         = LocalOffset;
    const int OldStructMemCount = StructMemCount;
    const int OldStructCount    = StructCount;
    const int OldArrayCount     = ArrayCount;

    PushScope();
    Expect('{');
    while (TokenType != '}') {
        ParseStatement();
    }
    GetToken();
    PopScope();
    if (OldOffset != LocalOffset) {
        Pending += OldOffset - LocalOffset;
        LocalOffset = OldOffset;
    }

    StructMemCount = OldStructMemCount;
    StructCount    = OldStructCount;
    ArrayCount     = OldArrayCount;
}

// Arg 2              BP + 6
// Arg 1              BP + 4
// Return Address     BP + 2
// Old BP             BP      <-- BP
// Local 1            BP - 2
// Local 2            BP - 4  <-- SP

void ParseExternalDefition(void)
{
    struct VarDecl* vd = ParseFirstDecl();
    if (!vd)
        goto End;

    CurrentType = vd->Type &= ~VT_STATIC;

    if (vd->Type & VT_FUN) {
        vd->Type |= VT_LOCGLOB;
        if (TokenType == ';') {
            GetToken();
            return;
        }
        int i;
        // Make room for "late globals"
        for (i = 0; i < GLOBAL_RESERVE; ++i) {
            VarDecls[Scopes[0]++].Id = -1;
        }
        PushScope();
        Scopes[0] -= GLOBAL_RESERVE;
        for (i = 0; i < ArgsCount; ++i) {
            struct Argument* A = &Args[i];
            CurrentType = A->Type | VT_LOCOFF;
            CurrentTypeExtra = A->TypeExtra;
            struct VarDecl* arg = AddVarDecl(A->Id);
            arg->Offset = 4 + i*2;
        }

        if (LocalLabelCounter) Fail();
        LocalOffset = 0;
        ReturnUsed = 0;
        ReturnLabel = MakeLabel();
        BreakLabel = ContinueLabel = -1;

        EmitGlobalLabel(vd);
        Output1Byte(I_PUSH|R_BP);
        EmitMovRR(R_BP, R_SP);
        ParseCompoundStatement();
        if (ReturnUsed) {
            EmitLocalLabel(ReturnLabel);
            EmitMovRR(R_SP, R_BP);
        }
        Output2Bytes(I_POP|R_BP, I_RET);

        // When debugging:
        //int l;for (l = 0; l < LocalLabelCounter; ++l)if (!(Labels[l].Ref == 0)) Fail();
        LocalLabelCounter = 0;
        NamedLabelCount = 0;
        PopScope();
        return;
    }

    while (vd) {
        vd->Type |= VT_LOCGLOB;

        if (TokenType == '=') {
            GetToken();
            EmitGlobalLabel(vd);
            ParseAssignmentExpression();
            switch (CurrentType) {
            case VT_INT|VT_LOCLIT:
                // TODO: Could save one byte per global char...
                OutputWord(CurrentVal);
                break;
            case VT_LVAL|VT_ARRAY:
                {
                    struct ArrayDecl* AD = &ArrayDecls[vd->TypeExtra];
                    if (AD->Type != VT_CHAR) Fail();
                    if (AD->Bound) Fail(); // TODO
                    AD->Bound = ArrayDecls[CurrentTypeExtra].Bound;
                    if (CurrentTypeExtra != ArrayCount-1) Fail();
                    --ArrayCount;
                }
                break;
            default:
                Fail();
            }
        } else {
            BssSize += SizeofCurrentType();
        }
        vd = NextDecl();
    }

End:
    Expect(';');
}

void MakeOutputFilename(const char* n, const char* ext)
{
    char* dest = IdBuffer;
    char* LastDot;
    LastDot = 0;
    while (*n) {
        if (*n == '.')
            LastDot = dest;
        *dest++ = *n++;
    }
    if (!LastDot) LastDot = dest;
    CopyStr(LastDot, ext);
}

int OpenOutput(void)
{
    const int OutFile = open(IdBuffer, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0600);
    if (OutFile < 0) {
        Fatal("Error creating output file");
    }
    return OutFile;
}

void AddBuiltins(const char* s)
{
    char ch;
    do {
        const int Id = IdCount++;
        unsigned Hash = HASHINIT;
        IdText[Id] = &IdBuffer[IdBufferIndex];
        while ((ch = *s++) > ' ') {
            IdBuffer[IdBufferIndex++] = ch;
            Hash = Hash*HASHMUL+ch;
        }
        IdBuffer[IdBufferIndex++] = 0;
        int i = 0;
        for (;;) {
            Hash &= ID_HASHMAX-1;
            if (IdHashTab[Hash] == -1) {
                IdHashTab[Hash] = Id;
                break;
            }
            Hash += 1 + i++;
        }
    } while(ch);
}

int main(int argc, char** argv)
{
    if (argc < 2) {
        Printf("Usage: scc input-file [output-file]\n");
        return 1;
    }

    const int StartTime = clock();

    InFile = open(argv[1], O_RDONLY | O_BINARY);
    if (InFile < 0) {
        Fatal("Error opening input file");
    }

    MakeOutputFilename(argv[2] ? argv[2] : argv[1], ".map");
    MapFile = OpenOutput();

    memset(IdHashTab, -1, sizeof(IdHashTab));
    memset(VarLookup, -1, sizeof(VarLookup));

    AddBuiltins("auto break case char const continue default do double"
        " else enum extern float for goto if int long register return"
        " short signed sizeof static struct switch typedef union unsigned"
        " void volatile while va_list va_start va_end va_arg _emit _start"
        " _SBSS _EBSS");
    if (IdCount+TOK_KEYWORD-1 != TOK__EBSS) Fail();

    // Prelude
    Output2Bytes(I_XOR|1, MODRM_REG|R_BP<<3|R_BP);
    CurrentType = VT_FUN|VT_VOID|VT_LOCGLOB;
    Output1Byte(I_JMP_REL16);
    EmitGlobalRefRel(AddVarDecl(TOK__START-TOK_KEYWORD));

    CurrentType = VT_CHAR|VT_LOCGLOB;
    struct VarDecl* SBSS = AddVarDecl(TOK__SBSS-TOK_KEYWORD);
    struct VarDecl* EBSS = AddVarDecl(TOK__EBSS-TOK_KEYWORD);

    InBufPtr = InBuf;
    NextChar();
    GetToken();
    while (TokenType != TOK_EOF) {
        ParseExternalDefition();
    }
    close(InFile);
    if (ScopesCount != 1) Fail();

    EmitGlobalLabel(SBSS);
    struct VarDecl* vd = VarDecls;
    struct VarDecl* end = &VarDecls[*Scopes];
    for (; vd != end; ++vd) {
        CurrentType      = vd->Type;
        CurrentTypeExtra = vd->TypeExtra;
        if ((CurrentType & VT_LOCMASK) != VT_LOCGLOB || vd->Offset || vd == EBSS)
            continue;
        if (CurrentType & VT_FUN) {
            Printf("%s is undefined\n", IdText[vd->Id]);
            Fatal("Undefined function");
        }
        EmitGlobalLabel(vd);
        CodeAddress += SizeofCurrentType();
    }
    if (CodeAddress - SBSS->Offset != BssSize) Fail();
    EmitGlobalLabel(EBSS);

    close(MapFile);

    if (argv[2])
       CopyStr(IdBuffer, argv[2]);
    else
        MakeOutputFilename(argv[1], ".com");
    const int OutFile = OpenOutput();
    write(OutFile, Output, CodeAddress - BssSize - CODESTART);
    close(OutFile);

    Printf("%s %d\n", IdBuffer, clock()-StartTime);

    return 0;
}

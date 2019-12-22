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

#define CREATE_FLAGS O_WRONLY | O_CREAT | O_TRUNC | O_BINARY

#else
// Only used when self-compiling
// Small "standard library"

enum { CREATE_FLAGS = 1 }; // Ugly hack

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
    _emit 0x56                       // PUSH SI
    _emit 0x57                       // PUSH DI
    _emit 0x8B _emit 0x7E _emit 0x04 // MOV DI,[BP+0x4]
    _emit 0x8B _emit 0x46 _emit 0x06 // MOV AX,[BP+0x6]
    _emit 0x8B _emit 0x4E _emit 0x08 // MOV CX,[BP+0x8]
    _emit 0xF3 _emit 0xAA            // REP STOSB
    _emit 0x5F                       // POP DI
    _emit 0x5E                       // POP SI
}

int memcmp(void* l, void* r, int size)
{
    _emit 0x56                       // PUSH SI
    _emit 0x57                       // PUSH DI
    _emit 0x31 _emit 0xC0            // XOR AX, AX
    _emit 0x8B _emit 0x7E _emit 0x04 // MOV DI,[BP+0x4]
    _emit 0x8B _emit 0x76 _emit 0x06 // MOV SI,[BP+0x6]
    _emit 0x8B _emit 0x4E _emit 0x08 // MOV CX,[BP+0x8]
    _emit 0xF3 _emit 0xA6            // REPE CMPSB
    _emit 0x74 _emit 0x01            // JE  $+3
    _emit 0x40                       // INC AX // XXX: TODO sometimes output -1
    _emit 0x5F                       // POP DI
    _emit 0x5E                       // POP SI
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
    VARDECL_MAX = 400,
    ID_MAX = 550,
    ID_HASHMAX = 1024,      // Must be power of 2 and (some what) greater than ID_MAX
    IDBUFFER_MAX = 4800,
    LABEL_MAX = 300,
    NAMED_LABEL_MAX = 10,
    OUTPUT_MAX = 0x6000,    // Always try to reduce this if something fails unexpectedly... ~600bytes of stack is needed.
    STRUCT_MAX = 8,
    STRUCT_MEMBER_MAX = 32,
    ARRAY_MAX = 32,
    GLOBAL_RESERVE = 4,     // How many globals symbols to allow to be defined in functions (where static variables count as globals)

    // Brute forced best values (when change was made)
    HASHINIT = 17,
    HASHMUL  = 89,
};

// Type flags and values
enum {
    VT_VOID,
    VT_BOOL,                // CurrentValue holds condition code for "true"
    VT_CHAR,
    VT_INT,
    VT_STRUCT,

    VT_BASEMASK = 7,

    VT_LVAL    = 1<<3,
    VT_FUN     = 1<<4,
    VT_ARRAY   = 1<<5,

    VT_PTR1    = 1<<6,
    VT_PTRMASK = 3<<6,      // 3 levels of indirection should be enough..

    VT_LOCLIT  = 1<<8,      // CurrentVal holds a literal value (or label)
    VT_LOCOFF  = 2<<8,      // CurrentVal holds BP offset
    VT_LOCGLOB = 3<<8,      // CurrentVal holds the VarDecl index of a global
    VT_LOCMASK = 3<<8,

    VT_STATIC  = 1<<10,
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

    // NOTE: Must match order of registration in main
    //       TOK_BREAK must be first
    TOK_BREAK,
    TOK_CASE,
    TOK_CHAR,
    TOK_CONST,
    TOK_CONTINUE,
    TOK_DEFAULT,
    TOK_DO,
    TOK_ELSE,
    TOK_ENUM,
    TOK_FOR,
    TOK_GOTO,
    TOK_IF,
    TOK_INT,
    TOK_RETURN,
    TOK_SIZEOF,
    TOK_STATIC,
    TOK_STRUCT,
    TOK_SWITCH,
    TOK_UNION,
    TOK_VOID,
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
    PRED_EQ = 14,
    PRED_COMMA,
    PRED_STOP = 100,
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
    int Extra;
};

struct VarDecl {
    int Id;
    int Type;
    int TypeExtra;
    int Offset;
    int Ref; // Fixup address list for globals
    int Prev;
};

int CodeAddress = CODESTART;
int BssSize;

int MapFile;

int InFile;
char InBuf[INBUF_MAX];
char* InBufPtr;
char* InBufEnd;
char CurChar;
char StoredSlash;

int Line = 1;

int TokenType;
int TokenNumVal;
char* TokenStrLit;
char OperatorPrecedence;

char IdBuffer[IDBUFFER_MAX];
int IdBufferIndex;

int IdOffset[ID_MAX];
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

int ReturnUsed;
char PendingPushAx; // Remember to adjsust LocalOffset!
int PendingSpAdj;
int LastFixup;
int IsDeadCode;

int NextSwitchCase = -1; // Where to go if we haven't matched
int NextSwitchStmt;      // Where to go if we've matched (-1 if already emitted)
int SwitchDefault;       // Optional switch default label

char Output[OUTPUT_MAX]; // Place this last to allow partial stack overflows!

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

const char* IdText(int id)
{
    if (id < 0 || id >= IdCount) Fail();
    return IdBuffer + IdOffset[id];
}

int NextChar(void)
{
    if (InBufPtr == InBufEnd) {
        InBufPtr = InBuf;
        InBufEnd = InBufPtr + read(InFile, InBuf, INBUF_MAX);
        if (InBufPtr == InBufEnd) {
            return CurChar = 0;
        }
    }
    return CurChar = *InBufPtr++;
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

void SkipLine(void)
{
    while (CurChar != '\n') {
        while (InBufPtr != InBufEnd) {
            if ((CurChar = *InBufPtr++) == '\n')
                goto Found;
        }
        if (!NextChar())
            return;
    }
Found:
    NextChar();
    ++Line;
}

void SkipWhitespace(void)
{
    for (;;) {
        while (CurChar <= ' ') {
            if (CurChar == '\n')
                ++Line;
            while (InBufPtr != InBufEnd) {
                if ((CurChar = *InBufPtr++) > ' ') {
                    if (CurChar != '/')
                        return;
                    goto Slash;
                }
                if (CurChar == '\n')
                    ++Line;
            }
            if (!NextChar()) {
                return;
            }
        }
        if (CurChar != '/')
            return;
Slash:
        switch (NextChar()) {
        case '/':
            SkipLine();
            continue;
        case '*':
            {
                NextChar();
                int star = 0;
                while (!star || CurChar != '/') {
                    star = CurChar == '*';
                    if (CurChar == '\n') ++Line;
                    if (!NextChar()) Fail(); // Unterminated comment
                }
                NextChar();
            }
            continue;
        }
        StoredSlash = 1;
        return;
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
        if (StoredSlash || CurChar != '"')
            break;
        NextChar();
    }
    TokenStrLit[TokenNumVal++] = 0;
    if (TokenStrLit+TokenNumVal-Output > OUTPUT_MAX) Fail();
}

const char IsIdChar[] = "\x00\x00\x00\x00\x00\x00\xFF\x03\xFE\xFF\xFF\x87\xFE\xFF\xFF\x07";

void GetToken(void)
{
Redo:
    SkipWhitespace();
    if (StoredSlash) {
        TokenType = '/';
        StoredSlash = 0;
        goto Slash;
    }
    TokenType = CurChar;
    NextChar();
    OperatorPrecedence = PRED_STOP;

    if (TokenType < 'A') {
        if (TokenType < '0') {
        } else if (TokenType <= '9') {
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
        int Hash = HASHINIT*HASHMUL+(*pc++ = TokenType);
        while ((IsIdChar[CurChar>>3]>>(CurChar&7)) & 1) {
            Hash = Hash*HASHMUL+(*pc++ = CurChar);
            if (InBufPtr != InBufEnd) {
                CurChar = *InBufPtr++;
                continue;
            }
            NextChar();
        }
        *pc++ = 0;
        int Slot = 0;
        for (;;) {
            Hash &= ID_HASHMAX-1;
            if ((TokenType = IdHashTab[Hash]) == -1) {
                if (IdCount == ID_MAX) Fail();
                TokenType = IdHashTab[Hash] = IdCount++;
                IdOffset[TokenType] = IdBufferIndex;
                IdBufferIndex += (int)(pc - start);
                if (IdBufferIndex > IDBUFFER_MAX) Fail();
                break;
            }
            if (memcmp(start, IdBuffer + IdOffset[TokenType], pc - start)) {
                Hash += 1 + Slot++;
                continue;
            }
            break;
        }
        TokenType += TOK_BREAK;
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
        OperatorPrecedence = PRED_EQ;
        TryGetChar('=', TOK_EQEQ, 7);
        return;
    case ',':
        OperatorPrecedence = PRED_COMMA;
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
            TryGetChar('=', TOK_LSHEQ, PRED_EQ);
        } else{
            TryGetChar('=', TOK_LTEQ, 6);
        }
        return;
    case '>':
        OperatorPrecedence = 6;
        if (TryGetChar('>', TOK_RSH, 5)) {
            TryGetChar('=', TOK_RSHEQ, PRED_EQ);
        } else {
            TryGetChar('=', TOK_GTEQ, 6);
        }
        return;
    case '&':
        OperatorPrecedence = 8;
        if (!TryGetChar('&', TOK_ANDAND, 11)) {
            TryGetChar('=', TOK_ANDEQ, PRED_EQ);
        }
        return;
    case '|':
        OperatorPrecedence = 10;
        if (!TryGetChar('|', TOK_OROR, 12)) {
            TryGetChar('=', TOK_OREQ, PRED_EQ);
        }
        return;
    case '^':
        OperatorPrecedence = 9;
        TryGetChar('=', TOK_XOREQ, PRED_EQ);
        return;
    case '+':
        OperatorPrecedence = 4;
        if (!TryGetChar('+', TOK_PLUSPLUS, PRED_STOP)) {
            TryGetChar('=', TOK_PLUSEQ, PRED_EQ);
        }
        return;
    case '-':
        OperatorPrecedence = 4;
        if (!TryGetChar('-', TOK_MINUSMINUS, PRED_STOP)) {
            if (!TryGetChar('=', TOK_MINUSEQ, PRED_EQ))
                TryGetChar('>', TOK_ARROW, PRED_STOP);
        }
        return;
    case '*':
        OperatorPrecedence = 3;
        TryGetChar('=', TOK_STAREQ, PRED_EQ);
        return;
    case '/':
 Slash:
        OperatorPrecedence = 3;
        TryGetChar('=', TOK_SLASHEQ, PRED_EQ);
        return;
    case '%':
        OperatorPrecedence = 3;
        TryGetChar('=', TOK_MODEQ, PRED_EQ);
        return;
    case '_':
        goto Identifier;
    case '.':
        if (CurChar == '.') {
            NextChar();
            if (!TryGetChar('.', TOK_ELLIPSIS, PRED_STOP)) {
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
    case '#':
        SkipLine();
        goto Redo;
    }
    Fatal("Unknown token encountered");
}

void PrintTokenType(int T)
{
    if (T >= TOK_BREAK) Printf("%s ", IdText(T-TOK_BREAK));
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
    I_ADC            = 0x10,
    I_SBB            = 0x18,
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
    I_MOV_RM_R       = 0x8a,
    I_LEA            = 0x8d,
    I_XCHG_AX        = 0x90,
    I_CBW            = 0x98,
    I_CWD            = 0x99,
    I_MOV_R_IMM16    = 0xb8,
    I_MOVSB          = 0xa4,
    I_MOVSW          = 0xa5,
    I_STOSB          = 0xaa,
    I_LODSB          = 0xac,
    I_RET            = 0xc3,
    I_INT            = 0xcd,
    I_SHROT16_CL     = 0xd3,
    I_CALL           = 0xe8,
    I_JMP_REL16      = 0xe9,
    I_JMP_REL8       = 0xeb,
    I_REP            = 0xf3,
    I_INCDEC_RM      = 0xfe,
};

enum {
    SHROT_SHL = 4,
    SHROT_SAR = 7,
};

void FlushSpAdj(void);
void FlushPushAx(void);
int EmitChecks(void);

void OutputBytes(int first, ...)
{
    // Inlined EmitChecks()
    if (IsDeadCode)
        return;
    if (PendingPushAx|PendingSpAdj) {
        FlushSpAdj();
        FlushPushAx();
    }
    va_list vl;
    va_start(vl, first);
    do {
        Output[CodeAddress++ - CODESTART] = first;
    } while ((first = va_arg(vl, int)) != -1);
    va_end(vl);
    if (CodeAddress > OUTPUT_MAX+CODESTART) Fail();
}

void OutputWord(int w)
{
    OutputBytes(w&0xff, (w>>8)&0xff, -1);
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
    char* c;
    while (r) {
        int o = 0;
        f = CodeAddress;
        if (relative) {
            o = r + 2;
            f -= o;
        }
        c = &Output[r - CODESTART];
        r = (c[0]&0xff)|c[1]<<8;
        if (!c[-1]) {
            // Fused Jcc/JMP
            if (f <= 0x7f-3) {
                // Invert condition and NOP out jump
                if ((c[-3]&0xf0) != 0x70 || c[-2] != 3) Fail();
                c[-3] ^= 1;
                c[-2] = f+3;
                c[-1] = I_XCHG_AX-256;
                c[0]  = I_XCHG_AX-256;
                c[1]  = I_XCHG_AX-256;
                continue;
            }
            c[-1] = I_JMP_REL16-256;
        } else {
            // Is this a uselss jump we can avoid? (Too cumbersome if we already resolved fixups at this address)
            if (CodeAddress == o && LastFixup != CodeAddress) {
                CodeAddress -= 3;
                continue;
            }
        }
        if (c[-1] == I_JMP_REL16-256) {
            ++f;
            if (f == (char)f) {
                // Convert to short jump + nop (the nop is just to not throw off disassemblers more than necessary)
                c[-1] = I_JMP_REL8-256;
                f = (f&0xff)|0x9000;
            } else {
                --f;
            }
        }
        c[0] = (char)(f);
        c[1] = (char)(f>>8);
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
    P = CopyStr(P, vd->Id == -2 ? StaticName : IdText(vd->Id));
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
        OutputBytes(Inst, MODRM_BP_DISP8|R, Val & 0xff, -1);
        if (Val != (char)Val && !IsDeadCode) {
            Output[CodeAddress-(CODESTART+2)] += MODRM_BP_DISP16-MODRM_BP_DISP8;
            OutputBytes((Val >> 8) & 0xff, -1);
        }
        return;
    case VT_LOCGLOB:
        OutputBytes(Inst, MODRM_DISP16|R, -1);
        EmitGlobalRef(&VarDecls[Val]);
        return;
    }
    if (Loc) Fail();
    OutputBytes(Inst, MODRM_SI|R, -1);
}

void EmitLoadAx(int Size, int Loc, int Val)
{
    switch (Loc) {
    case 0:
        OutputBytes(I_LODSB-1+Size, -1);
        break;
    case VT_LOCGLOB:
        OutputBytes(0xA0-1+Size, -1);
        EmitGlobalRef(&VarDecls[Val]);
        break;
    default:
        EmitModrm(I_MOV_RM_R-1+Size, R_AX, Loc, Val);
    }
    if (Size == 1)
        OutputBytes(I_CBW, -1);
}

void EmitStoreAx(int Size, int Loc, int Val)
{
    switch (Loc) {
    case 0:
        OutputBytes(I_STOSB-1+Size, -1);
        return;
    case VT_LOCGLOB:
        OutputBytes(0xA2-1+Size, -1);
        EmitGlobalRef(&VarDecls[Val]);
        return;
    }
    EmitModrm(I_MOV_R_RM-1+Size, R_AX, Loc, Val);
}

void EmitStoreConst(int Size, int Loc, int Val)
{
    EmitModrm(0xc6-1+Size, 0, Loc, Val);
    OutputBytes(CurrentVal & 0xff, - 1);
    if (Size == 2)
        OutputBytes((CurrentVal >> 8) & 0xff, - 1);
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
        OutputBytes(Op, -1);
        if (Amm > 1)
            OutputBytes(Op, -1);
    } else if (Amm < 128) {
        OutputBytes(I_ALU_RM16_IMM8, MODRM_REG|Op|r, Amm & 0xff, -1);
    } else {
        if (r == R_AX)
            OutputBytes(Op|5, -1);
        else
            OutputBytes(I_ALU_RM16_IMM16, MODRM_REG|Op|r, -1);
        OutputWord(Amm);
    }
}

void EmitPush(int r)
{
    OutputBytes(I_PUSH | r, -1);
}

void EmitPop(int r)
{
    OutputBytes(I_POP | r, -1);
}

void EmitMovRR(int d, int s)
{
    OutputBytes(I_MOV_R_RM | 1, MODRM_REG | s<<3 | d, -1);
}

void EmitMovRImm(int r, int val)
{
    OutputBytes(I_MOV_R_IMM16 | r, -1);
    OutputWord(val);
}

void EmitScaleAx(int Scale)
{
    if (Scale & (Scale-1)) {
        EmitMovRImm(R_CX, Scale);
        OutputBytes(0xF7, MODRM_REG | (5<<3) | R_CX, -1); // IMUL CX
    } else {
        while (Scale >>= 1) {
            OutputBytes(I_ADD|1, MODRM_REG, -1);
        }
    }
}

void EmitDivAxConst(int Amm)
{
    if (Amm & (Amm-1)) {
        EmitMovRImm(R_CX, Amm);
        OutputBytes(I_CWD, 0xF7, MODRM_REG | (7<<3) | R_CX, -1); // IDIV CX
    } else {
        while (Amm >>= 1) {
            OutputBytes(0xD1, MODRM_REG | SHROT_SAR<<3, -1); // SAR AX, 1
        }
    }
}

int EmitLocalJump(int l)
{
    if (l < 0 || l >= LocalLabelCounter) Fail();
    int Addr = Labels[l].Addr;
    if (Addr) {
        Addr -= CodeAddress+2;
        if (Addr == (char)Addr) {
            OutputBytes(I_JMP_REL8, Addr & 0xff, -1);
        } else {
            OutputBytes(I_JMP_REL16, -1);
            OutputWord(Addr-1);
        }
        return 1;
    } else {
        OutputBytes(I_JMP_REL16, -1);
        AddFixup(&Labels[l].Ref);
        return 0;
    }
}

void EmitJmp(int l)
{
    PendingPushAx = 0;
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
            OutputBytes(0x70 | cc, a & 0xff, -1);
            return;
        }
    }

    OutputBytes(0x70 | (cc^1), 3, -1); // Skip jump
    if (!EmitLocalJump(l))
        Output[CodeAddress-(CODESTART+3)] = 0; // Mark as fused Jcc/JMP
}

void EmitLoadAddr(int Reg, int Loc, int Val)
{
    if (Loc == VT_LOCOFF) {
        EmitModrm(I_LEA, Reg, VT_LOCOFF, Val);
    } else if (Loc == VT_LOCGLOB) {
        OutputBytes(I_MOV_R_IMM16 | Reg, -1);
        EmitGlobalRef(&VarDecls[Val]);
    } else {
        Fail();
    }
}

void FlushSpAdj(void)
{
    int Amm = PendingSpAdj;
    if (Amm) {
        // Preserve state of PendingPushAx and avoid emitting
        // a push before adjusting the stack
        const int ppa = PendingPushAx;
        PendingPushAx = 0;
        PendingSpAdj = 0;
        EmitAddRegConst(R_SP, Amm);
        PendingPushAx = ppa;
    }
}

void FlushPushAx(void)
{
    if (PendingPushAx) {
        PendingPushAx = 0;
        EmitPush(R_AX);
        LocalOffset -= 2;
    }
}

int EmitChecks(void)
{
    if (IsDeadCode) {
        return 1;
    }
    FlushSpAdj();
    FlushPushAx();
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

int Accept(int type)
{
    if (TokenType != type)
        return 0;
    GetToken();
    return 1;
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
        return id - TOK_BREAK;
    }
    Printf("Expected identifier got ");
    Unexpected();
}

int IsTypeStart(void)
{
    switch (TokenType) {
    case TOK_CONST:
    case TOK_VOID:
    case TOK_CHAR:
    case TOK_INT:
    case TOK_ENUM:
    case TOK_STATIC:
    case TOK_STRUCT:
    case TOK_UNION:
    case TOK_VA_LIST:
        return 1;
    }
    return 0;
}

int SizeofType(int Type, int Extra)
{
    Type &= VT_BASEMASK|VT_PTRMASK|VT_ARRAY;
    if (Type & VT_ARRAY) {
        struct ArrayDecl* AD = &ArrayDecls[Extra];
        if (!AD->Bound) Fail();
        return AD->Bound * SizeofType(Type & ~VT_ARRAY, AD->Extra);
    } else if (Type == VT_CHAR) {
        return 1;
    } else if (Type == VT_STRUCT) {
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
    if (Type != VT_INT && !(Type & VT_PTRMASK)) Fail();
    return 2;
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

        if (CurrentType & VT_ARRAY) {
            // Decay array
            CurrentType = (CurrentType & ~VT_ARRAY) + VT_PTR1;
            CurrentTypeExtra = ArrayDecls[CurrentTypeExtra].Extra;
            if (loc)
                EmitLoadAddr(R_AX, loc, CurrentVal);
            return;
        }

        int sz = 2;
        if (CurrentType == VT_CHAR) {
            sz = 1;
            CurrentType = VT_INT;
        }
        if (!loc) {
            OutputBytes(I_XCHG_AX|R_SI, -1);
        }
        EmitLoadAx(sz, loc, CurrentVal);
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
        OutputBytes(I_XCHG_AX|R_SI, -1);
    }
    if (Post) {
        EmitLoadAx(1+WordOp, Loc, CurrentVal);
        if (!Loc)
            EmitAddRegConst(R_SI, -1-WordOp); // Undo increment done by LODS (for increment/decrement below)
        if (WordOp)
            CurrentType &= ~(VT_LVAL|VT_LOCMASK);
        else
            CurrentType = VT_INT;
    }
    Op = Op == TOK_MINUSMINUS;

    if (Size != 1) {
        EmitModrm(I_ALU_RM16_IMM8, Op*(I_SUB>>3), Loc, CurrentVal);
        OutputBytes(Size, -1);
        return;
    }
    EmitModrm(I_INCDEC_RM|WordOp, Op, Loc, CurrentVal);
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
        AD->Bound = TokenNumVal;
        AD->Extra = 0;
        CurrentType = VT_CHAR | VT_ARRAY | VT_LVAL;
        CurrentTypeExtra = ArrayCount++;
        if (!IsDeadCode) {
            int EndLab = -1;
            if (ScopesCount != 1) {
                OutputBytes(I_MOV_R_IMM16, -1);
                OutputWord(CodeAddress+5);
                EmitJmp(EndLab = MakeLabel());
                IsDeadCode = 0;
            }
            while (TokenNumVal--)
                OutputBytes(*TokenStrLit++ & 0xff, -1);
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
        OutputBytes(0x70 | (CurrentVal^1), 1, I_INC, -1);
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
            OutputBytes(I_XOR|1, 0xC0, -1);
    }
}

void HandleStructMember(void)
{
    const int MemId = ExpectId();
    if (CurrentTypeExtra < 0 || CurrentTypeExtra >= StructCount) Fail();
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
                GetToken();
                // Function call
                if ((CurrentType & (VT_FUN|VT_LOCMASK)) != (VT_FUN|VT_LOCGLOB)) Fail();
                struct VarDecl* Func = &VarDecls[CurrentVal];
                const int RetType = CurrentType & ~(VT_FUN|VT_LOCMASK|VT_LVAL);
                int ArgSize = 0;
                enum { ArgChunkSize = 8 }; // Must be power of 2
                while (TokenType != ')') {
                    if (!(ArgSize & (ArgChunkSize-1))) {
                        FlushPushAx();
                        LocalOffset  -= ArgChunkSize;
                        PendingSpAdj -= ArgChunkSize;
                        if (ArgSize) {
                            // Move arguments to new stack top
                            EmitModrm(I_LEA, R_SI, VT_LOCOFF, LocalOffset + ArgChunkSize*2);
                            EmitModrm(I_LEA, R_DI, VT_LOCOFF, LocalOffset);
                            EmitMovRImm(R_CX, ArgSize*2);
                            OutputBytes(I_REP, I_MOVSW, -1);
                        }
                    }
                    ParseAssignmentExpression();
                    if (CurrentType == (VT_INT|VT_LOCLIT)) {
                        EmitStoreConst(2, VT_LOCOFF, LocalOffset + ArgSize);
                    } else {
                        GetVal();
                        if (CurrentType != VT_INT && !(CurrentType & VT_PTRMASK)) Fail();
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
                OutputBytes(I_CALL, -1);
                EmitGlobalRefRel(Func);
                PendingSpAdj += (ArgSize = ((ArgSize+ArgChunkSize-1)&-ArgChunkSize));
                LocalOffset  += ArgSize;
                CurrentType = RetType;
                if (CurrentType == VT_CHAR) {
                    CurrentType = VT_INT;
                }
                CurrentTypeExtra = Func->TypeExtra;
                continue;
            }
        case '[':
            {
                GetToken();
                struct VarDecl* GlobalArr = 0;
                if ((CurrentType & (VT_ARRAY|VT_LOCMASK)) == (VT_ARRAY|VT_LOCGLOB)) {
                    GlobalArr = &VarDecls[CurrentVal];
                    CurrentType &= ~(VT_ARRAY|VT_LOCMASK);
                    CurrentTypeExtra = ArrayDecls[CurrentTypeExtra].Extra;
                } else {
                    LvalToRval();
                    if (!(CurrentType & VT_PTRMASK)) {
                        Fatal("Expected pointer");
                    }
                    CurrentType -= VT_PTR1;
                    PendingPushAx = 1;
                }
                const int Scale    = SizeofCurrentType();
                const int ResType  = CurrentType | VT_LVAL;
                const int ResExtra = CurrentTypeExtra;
                ParseExpr();
                Expect(']');

                if (CurrentType == (VT_INT|VT_LOCLIT)) {
                    if (GlobalArr) {
                        OutputBytes(I_MOV_R_IMM16|R_AX, -1);
                        EmitGlobalRef(GlobalArr);
                    } else {
                        if (!PendingPushAx) Fail();
                        PendingPushAx = 0;
                    }
                    EmitAddRegConst(R_AX, CurrentVal * Scale);
                } else {
                    LvalToRval();
                    if (CurrentType != VT_INT || PendingPushAx) Fail();
                    EmitScaleAx(Scale);
                    if (GlobalArr) {
                        OutputBytes(I_ADD|5, -1);
                        EmitGlobalRef(GlobalArr);
                    } else {
                        EmitPop(R_CX);
                        LocalOffset += 2;
                        OutputBytes(I_ADD|1, MODRM_REG|R_CX<<3, -1);
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
    OutputBytes(I_AND|1, MODRM_REG, -1); // AND AX, AX
    CurrentType = VT_BOOL;
    CurrentVal  = JNZ;
}

void ParseUnaryExpression(void)
{
    const int Op = TokenType;
    int IsConst = 0;
    switch (Op) {
    case TOK_PLUSPLUS:
    case TOK_MINUSMINUS:
        GetToken();
        ParseUnaryExpression();
        DoIncDecOp(Op, 0);
        return;
    case '&':
    case '*':
    case '+':
    case '-':
    case '~':
    case '!':
        GetToken();
        ParseCastExpression();
        if (Op == '&') {
            if (!(CurrentType & VT_LVAL)) {
                Fatal("Lvalue required for address-of operator");
            }
            const int loc = CurrentType & VT_LOCMASK;
            if (loc) {
                EmitLoadAddr(R_AX, loc, CurrentVal);
            }
            CurrentType = (CurrentType&~(VT_LVAL|VT_LOCMASK)) + VT_PTR1;
            return;
        }

        if ((CurrentType & VT_LOCMASK) == VT_LOCLIT) {
            if (CurrentType != (VT_INT|VT_LOCLIT)) Fail();
            IsConst = 1;
        } else {
            LvalToRval();
        }
        if (Op == '*') {
            if (!(CurrentType & VT_PTRMASK)) {
                Fatal("Pointer required for dereference");
            }
            CurrentType = (CurrentType-VT_PTR1) | VT_LVAL;
        } else if (Op == '+') {
            if (!IsConst && CurrentType != VT_INT) Fail();
        } else if (Op == '-') {
            if (IsConst) {
                CurrentVal = -CurrentVal;
            } else {
                if (CurrentType != VT_INT) Fail();
                OutputBytes(0xF7, 0xD8, -1); // NEG AX
            }
        } else if (Op == '~') {
            if (IsConst) {
                CurrentVal = ~CurrentVal;
            } else {
                if (CurrentType != VT_INT) Fail();
                OutputBytes(0xF7, 0xD0, -1); // NOT AX
            }
        } else if (Op == '!') {
            if (IsConst) {
                CurrentVal = !CurrentVal;
            } else {
                if (CurrentType != VT_BOOL)
                    ToBool();
                CurrentVal ^= 1;
            }
        } else {
            Unexpected();
        }
        return;
    case TOK_SIZEOF:
        {
            GetToken();
            const int WasDead = IsDeadCode;
            IsDeadCode = 1;
            if (Accept('(')) {
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
    if (Accept('(')) {
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
            if (CurrentType == VT_CHAR) {
                OutputBytes(I_CBW, -1);
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
    case '<':       return I_CMP|1|JL<<8;
    case TOK_LTEQ:  return I_CMP|1|JNG<<8;
    case '>':       return I_CMP|1|JG<<8;
    case TOK_GTEQ:  return I_CMP|1|JNL<<8;
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
        OutputBytes(I_CWD, 0xF7, MODRM_REG | (7<<3) | R_CX, -1);
        if (Op == '%') {
            OutputBytes(I_XCHG_AX|R_DX, -1);
        }
        return;
    } else if (Op == TOK_LSH) {
        Inst = 0xd3;
        RM = MODRM_REG|SHROT_SHL<<3;
    } else if (Op == TOK_RSH) {
        Inst = 0xd3;
        RM = MODRM_REG|SHROT_SAR<<3;
    } else {
        Fail();
    }
HasInst:
    OutputBytes(Inst, RM, -1);
    FinishOp(Inst);
}

int DoRhsLvalBinOp(int Op)
{
    const int Loc = CurrentType & VT_LOCMASK;
    CurrentType &= ~(VT_LOCMASK|VT_LVAL);
    int Inst = GetSimpleALU(Op);
    if (!Inst) {
        OutputBytes(I_XCHG_AX|R_CX, -1);
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
        OutputBytes(Inst|5, -1);
        OutputWord(CurrentVal);
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
    if (CurrentType & VT_ARRAY)
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
        if (PendingPushAx) {
            PendingPushAx = 0;
            OutputBytes(I_XCHG_AX|R_DI, -1);
        } else {
            EmitPop(R_DI);
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
    int IsAssign;
    int LhsType;
    int LhsTypeExtra;
    int LhsVal;
    int LhsLoc;
    for (;;) {
        Op   = TokenType;
        Prec = OperatorPrecedence;
        if (Prec > OuterPrecedence) {
            break;
        }
        GetToken();

        if (Op == '?') {
            HandleCondOp();
            continue;
        }

        IsAssign = Prec == PRED_EQ;

        if (IsAssign) {
            if (!(CurrentType & VT_LVAL)) {
                Fatal("L-value required");
            }
            if (Op != '=')
                Op = RemoveAssign(Op);
            CurrentType &= ~VT_LVAL;
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
                PendingPushAx = 1;
            }
        }
        LhsType        = CurrentType;
        LhsTypeExtra   = CurrentTypeExtra;
        LhsVal         = CurrentVal;

        ParseCastExpression(); // RHS
        for (;;) {
            if (OperatorPrecedence > Prec || (OperatorPrecedence == Prec && OperatorPrecedence != PRED_EQ)) // LookAheadOp != PRED_EQ => !IsRightAssociative
                break;
            ParseExpr1(OperatorPrecedence);
        }

        LhsLoc = LhsType & VT_LOCMASK;
        LhsType &= ~VT_LOCMASK;

        if (Op == ',') {
            continue;
        } else if (Op == '=' && LhsType == VT_STRUCT) {
            // Struct assignment
            if ((CurrentType&~VT_LOCMASK) != (VT_STRUCT|VT_LVAL) || CurrentTypeExtra != LhsTypeExtra)
                Fail();
            Temp = CurrentType & VT_LOCMASK;

            HandleLhsLvalLoc(LhsLoc);
            if (LhsLoc)
                EmitLoadAddr(R_DI, LhsLoc, LhsVal);
            if (Temp)
                EmitLoadAddr(R_SI, Temp, CurrentVal);
            else
                OutputBytes(I_XCHG_AX|R_SI, -1);
            EmitMovRImm(R_CX, SizeofCurrentType());
            OutputBytes(I_REP, I_MOVSB, -1);
            continue;
        }

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
        } else if (IsAssign) {
            HandleLhsLvalLoc(LhsLoc);
            LvalToRval();
            int Size = 2;
            if (LhsType == VT_CHAR) {
                Size = 1;
            } else {
                if (LhsType != VT_INT && !(LhsType & VT_PTRMASK)) Fail();
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
                            OutputBytes(I_XCHG_AX|R_DI, -1);
                        CurrentType = LhsType | LhsLoc | VT_LVAL;
                        CurrentVal = LhsVal;
                        continue;
                    }
                }
                if (!Temp) {
                    if (CurrentType != VT_INT) Fail();
                    OutputBytes(I_XCHG_AX|R_CX, -1);
                }
                EmitLoadAx(Size, LhsLoc, LhsVal);
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
                    OutputBytes(I_STOSB-1+Size, -1);
                } else {
                    EmitStoreConst(Size, LhsLoc, LhsVal);
                }
                continue;
            } else {
                GetVal();
            }
            EmitStoreAx(Size, LhsLoc, LhsVal);
        } else {
            int LhsPointeeSize = 0;
            if (LhsType & VT_PTRMASK) {
                LhsPointeeSize = SizeofType(LhsType-VT_PTR1, LhsTypeExtra);
                CurrentTypeExtra = LhsTypeExtra;
            }
            if (CurrentType == (VT_LOCLIT|VT_INT)) {
                if (!(PendingPushAx|IsDeadCode)) Fail();
                PendingPushAx = 0;
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
                    if (LhsType != VT_INT && !LhsPointeeSize) Fail();
                    if (Op == '+' && LhsPointeeSize) {
                        GetVal();
                        EmitScaleAx(LhsPointeeSize);
                        CurrentType = LhsType;
                    } else if (PendingPushAx && !(CurrentType & VT_ARRAY)) {
                        if ((CurrentType&(VT_BASEMASK|VT_PTRMASK)) != VT_CHAR) {
                            if (!(CurrentType&VT_LVAL)) Fail();
                            PendingPushAx = 0;
                            if (!DoRhsLvalBinOp(Op))
                                goto DoNormal;
                            goto CheckSub;
                        }
                    }
                    GetVal();
                    if (PendingPushAx) Fail();
                    EmitPop(R_CX);
                    LocalOffset += 2;
                }
DoNormal:
                if (!Temp)
                    OutputBytes(I_XCHG_AX | R_CX, -1);
                DoBinOp(Op);
            }
CheckSub:
            if (Op == '-' && LhsPointeeSize) {
                EmitDivAxConst(LhsPointeeSize);
                CurrentType = VT_INT;
            }
        }
    }
}

void ParseExpr0(int OuterPrecedence)
{
    ParseCastExpression();
    ParseExpr1(OuterPrecedence);
}

void ParseExpr(void)
{
    ParseExpr0(PRED_COMMA);
}

void ParseAssignmentExpression(void)
{
    ParseExpr0(PRED_EQ);
}

void ParseDeclarator(int* Id);

void ParseDeclSpecs(void)
{
    int Storage = 0;
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
        case TOK_INT:
            CurrentType = VT_INT;
            break;
        case TOK_VA_LIST:
            CurrentType = VT_CHAR | VT_PTR1;
            break;
        case TOK_CONST: // Ignore but accept const for now
            break;
        case TOK_STATIC:
            Storage |= VT_STATIC;
            break;
        case TOK_ENUM:
            GetToken();
            if (TokenType >= TOK_ID) {
                // TODO: Store and use the enum identifier
                GetToken();
            }
            if (Accept('{')) {
                ++IgnoreRedef;
                int EnumVal = 0;
                while (TokenType != '}') {
                    const int id = ExpectId();
                    if (Accept('=')) {
                        ParseAssignmentExpression();
                        if (CurrentType != (VT_INT|VT_LOCLIT)) Fail();
                        EnumVal = CurrentVal;
                    }
                    CurrentType = VT_INT|VT_LOCLIT;
                    struct VarDecl* vd = AddVarDecl(id);
                    vd->Offset = EnumVal;
                    if (!Accept(',')) {
                        break;
                    }
                    ++EnumVal;
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
                if (Accept('{') || CurrentTypeExtra < 0) {
                    if (StructCount == STRUCT_MAX) Fail();
                    const int SI = StructCount++;
                    struct StructDecl* SD = &StructDecls[SI];
                    SD->Id = id;
                    struct StructMember** Last = &SD->Members;
                    while (!Accept('}')) {
                        ParseAbstractDecl();
                        int BaseType  = CurrentType;
                        int BaseExtra = CurrentTypeExtra;
                        while (TokenType != ';') {
                            if (StructMemCount == STRUCT_MEMBER_MAX) Fail();
                            struct StructMember* SM = &StructMembers[StructMemCount++];
                            ParseDeclarator(&SM->Id);
                            SM->Type      = CurrentType;
                            SM->TypeExtra = CurrentTypeExtra;
                            *Last = SM;
                            Last = &SM->Next;
                            if (!Accept(','))
                                break;
                            CurrentType      = BaseType;
                            CurrentTypeExtra = BaseExtra;
                        }
                        Expect(';');
                    }
                    *Last = 0;
                    CurrentTypeExtra = SI;
                }
                CurrentType = VT_STRUCT;
            }
            continue;
        default:
            CurrentType |= Storage;
            return;
        }
        GetToken();
    }
}

void ParseDeclarator(int* Id)
{
    // TODO: Could allow type qualifiers (const/volatile) here
    while (Accept('*')) {
        CurrentType += VT_PTR1;
    }
    if (Id) {
        *Id = ExpectId();
    }
    if (Accept('[')) {
        if (ArrayCount == ARRAY_MAX) Fail();
        struct ArrayDecl* AD = &ArrayDecls[ArrayCount];
        AD->Extra = CurrentTypeExtra;
        int T  = CurrentType | VT_ARRAY;
        int TE = ArrayCount++;
        if (TokenType != ']') {
            ParseExpr();
            if (CurrentType != (VT_INT|VT_LOCLIT)) Fail(); // Need constant expression
            AD->Bound = CurrentVal;
            if (AD->Bound <= 0) Fail();
        } else {
            AD->Bound = 0;
        }
        Expect(']');
        CurrentType      = T;
        CurrentTypeExtra = TE;
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
    return AddVarDecl(Id);
}

struct VarDecl* ParseFirstDecl(void)
{
    ParseDeclSpecs();
    if (TokenType == ';')
        return 0;
    return DoDecl();
}

// Remember to reset CurrentType/CurrentTypeExtra before calling
struct VarDecl* NextDecl(void)
{
    if (!Accept(','))
        return 0;
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
                OutputBytes(I_CMP|5, -1);
                OutputWord(CurrentVal);
                if (!Accept(TOK_CASE))
                    break;
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
            OutputBytes(I_POP|R_BP, I_RET, -1);
            IsDeadCode = 1;
        }
    GetSemicolon:
        Expect(';');
        return;
    case TOK_BREAK:
        GetToken();
        PendingSpAdj += BStackLevel - LocalOffset;
        EmitJmp(BreakLabel);
        goto GetSemicolon;
    case TOK_CONTINUE:
        GetToken();
        PendingSpAdj += CStackLevel - LocalOffset;
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
        OutputBytes(CurrentVal & 0xff, -1);
        return;
    case TOK_IF:
        {
            GetToken();
            const int ElseLabel = MakeLabel();
            Accept('(');
            DoCond(ElseLabel, 1);
            Accept(')');
            ParseStatement();
            if (Accept(TOK_ELSE)) {
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

    if (TokenType >= TOK_ID) {
        // Expression statement / Labelled statement
        const int id = TokenType - TOK_BREAK;
        GetToken();
        if (Accept(':')) {
            EmitLocalLabel(GetNamedLabel(id)->LabelId);
            EmitModrm(I_LEA, R_SP, VT_LOCOFF, LocalOffset);
            goto Redo;
        } else {
            HandlePrimaryId(id);
            ParsePostfixExpression();
            ParseExpr1(PRED_COMMA);
        }
    } else if (IsTypeStart()) {
        struct VarDecl* vd = ParseFirstDecl();
        const int BaseType   = CurrentType & ~VT_PTRMASK;
        const int BaseStruct = CurrentTypeExtra;
        while (vd) {
            int size = SizeofCurrentType();

            if (CurrentType & VT_STATIC) {
                vd->Type = (CurrentType & ~VT_STATIC) | VT_LOCGLOB;
                BssSize += size;
            } else {
                vd->Type |= VT_LOCOFF;
                size = (size+1)&-2;
                if (Accept('=')) {
                    ParseAssignmentExpression();
                    GetVal();
                    EmitPush(R_AX);
                } else {
                    PendingSpAdj -= size;
                }
                LocalOffset -= size;
                vd->Offset = LocalOffset;
            }
            CurrentType      = BaseType;
            CurrentTypeExtra = BaseStruct;
            vd = NextDecl();
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
    while (!Accept('}')) {
        ParseStatement();
    }
    PopScope();
    if (OldOffset != LocalOffset) {
        PendingSpAdj += OldOffset - LocalOffset;
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

    if (Accept('(')) {
        vd->Type |= VT_FUN | VT_LOCGLOB;
        // Make room for "late globals"
        int i;
        for (i = 0; i < GLOBAL_RESERVE; ++i) {
            VarDecls[Scopes[0]++].Id = -1;
        }
        PushScope();
        Scopes[0] -= GLOBAL_RESERVE;
        int ArgOffset;
        ArgOffset = 4;
        while (TokenType != ')') {
            if (Accept(TOK_ELLIPSIS))
                break;
            ParseAbstractDecl();
            if (CurrentType == VT_VOID)
                break;
            struct VarDecl* arg = AddVarDecl(ExpectId());
            arg->Type |= VT_LOCOFF;
            arg->Offset = ArgOffset;
            ArgOffset += 2;
            if (!Accept(',')) {
                break;
            }
        }
        Expect(')');
        if (!Accept(';')) {
            if (LocalLabelCounter) Fail();
            LocalOffset = 0;
            ReturnUsed = 0;
            ReturnLabel = MakeLabel();
            BreakLabel = ContinueLabel = -1;

            EmitGlobalLabel(vd);
            EmitPush(R_BP);
            EmitMovRR(R_BP, R_SP);
            ParseCompoundStatement();
            if (ReturnUsed) {
                EmitLocalLabel(ReturnLabel);
                EmitMovRR(R_SP, R_BP);
            }
            OutputBytes(I_POP|R_BP, I_RET, -1);

            // When debugging:
            //int l;for (l = 0; l < LocalLabelCounter; ++l)if (!(Labels[l].Ref == 0)) Fail();
            LocalLabelCounter = 0;
            NamedLabelCount = 0;
        }
        PopScope();
        return;
    }

    const int BaseType   = CurrentType;
    const int BaseStruct = CurrentTypeExtra;
    while (vd) {
        vd->Type |= VT_LOCGLOB;

        if (Accept('=')) {
            EmitGlobalLabel(vd);
            ParseAssignmentExpression();
            switch (CurrentType) {
            case VT_INT|VT_LOCLIT:
                // TODO: Could save one byte per global char...
                OutputWord(CurrentVal);
                break;
            case VT_LVAL|VT_ARRAY|VT_CHAR:
                {
                    if (vd->Type != (VT_LOCGLOB|VT_ARRAY|VT_CHAR)) Fail();
                    struct ArrayDecl* AD = &ArrayDecls[vd->TypeExtra];
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
        CurrentType      = BaseType;
        CurrentTypeExtra = BaseStruct;
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
    const int OutFile = open(IdBuffer, CREATE_FLAGS, 0600);
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
        int Hash = HASHINIT;
        IdOffset[Id] = IdBufferIndex;
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

    InFile = open(argv[1], 0); // O_RDONLY
    if (InFile < 0) {
        Fatal("Error opening input file");
    }

    MakeOutputFilename(argv[2] ? argv[2] : argv[1], ".map");
    MapFile = OpenOutput();

    memset(IdHashTab, -1, sizeof(IdHashTab));
    memset(VarLookup, -1, sizeof(VarLookup));

    AddBuiltins("break case char const continue default do else enum for goto if"
        " int return sizeof static struct switch union void while"
        " va_list va_start va_end va_arg _emit _start _SBSS _EBSS");
    if (IdCount+TOK_BREAK-1 != TOK__EBSS) Fail();

    // Prelude
    OutputBytes(I_XOR|1, MODRM_REG|R_BP<<3|R_BP, -1);
    CurrentType = VT_FUN|VT_VOID|VT_LOCGLOB;
    OutputBytes(I_JMP_REL16, -1);
    EmitGlobalRefRel(AddVarDecl(TOK__START-TOK_BREAK));

    CurrentType = VT_CHAR|VT_LOCGLOB;
    struct VarDecl* SBSS = AddVarDecl(TOK__SBSS-TOK_BREAK);
    struct VarDecl* EBSS = AddVarDecl(TOK__EBSS-TOK_BREAK);

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
            Printf("%s is undefined\n", IdText(vd->Id));
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

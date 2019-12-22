#ifndef __SCC__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

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
    _emit 0x31 _emit 0xC0               // XOR AX, AX
    _emit 0x8A _emit 0x4E _emit 0x04    // MOV CL, [BP+4]
    _emit 0x80 _emit 0xE9 _emit 0x30    // SUB CL, '0'
    _emit 0x80 _emit 0xF9 _emit 0x0A    // CMP CL, 10
    _emit 0x11 _emit 0xC0               // ADC AX, AX
}

int isalpha(int c)
{
    _emit 0x31 _emit 0xC0               // XOR AX, AX
    _emit 0x8A _emit 0x4E _emit 0x04    // MOV CL, [BP+4]
    _emit 0x80 _emit 0xE1 _emit 0xDF    // AND CL, 0xDF
    _emit 0x80 _emit 0xE9 _emit 0x41    // SUB CL, 'A'
    _emit 0x80 _emit 0xF9 _emit 0x1A    // CMP CL, 'Z'-'A'+1
    _emit 0x11 _emit 0xC0               // ADC AX, AX
}

#endif

enum {
    INBUF_MAX = 512,
    SCOPE_MAX = 16,
    VARDECL_MAX = 400,
    ID_MAX = 550,
    ID_HASHMAX = 1024, // Must be power of 2 and (some what) greater than ID_MAX
    IDBUFFER_MAX = 4800,
    LABEL_MAX = 300,
    NAMED_LABEL_MAX = 10,
    OUTPUT_MAX = 0x6000, // Always try to reduce this if something fails unexpectedly... ~600bytes of stack is needed.
    STRUCT_MAX = 8,
    STRUCT_MEMBER_MAX = 32,
    ARRAY_MAX = 32,
    GLOBAL_RESERVE = 4, // How many globals symbols to allow to be defined in functions (where static variables count as globals)
};

enum {
    // Brute forced best values (when change was made)
    HASHINIT = 17,
    HASHMUL  = 89,
};

enum {
    VT_VOID,
    VT_BOOL,        // CurrentValue holds condition code for "true"
    VT_CHAR,
    VT_INT,
    VT_STRUCT,

    VT_BASEMASK = 7,

    VT_LVAL    = 1<<3,
    VT_FUN     = 1<<4,
    VT_ARRAY   = 1<<5,

    VT_PTR1    = 1<<6,
    VT_PTRMASK = 3<<6, // 3 levels of indirection should be enough..

    VT_LOCLIT  = 1<<8,    // CurrentVal holds a literal value (or label)
    VT_LOCOFF  = 2<<8,    // CurrentVal holds BP offset
    VT_LOCGLOB = 3<<8,    // CurrentVal holds the VarDecl index of a global
    VT_LOCMASK = 3<<8,

    VT_STATIC  = 1<<10,
};

enum {
    TOK_EOF, // Must be zero

    TOK_NUM,
    TOK_STRLIT,

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

enum {
    PRED_EQ = 14,
    PRED_COMMA,
    PRED_STOP = 100,
};

enum {
    CODESTART = 0x100,
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
    int Id;
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
int InBufCnt;
int InBufSize;
char CurChar;
char StoredSlash;

int Line = 1;

int TokenType;
int TokenNumVal;
char* TokenStrLit;
int OperatorPrecedence;

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
int PendingPushAx; // Remember to adjsust LocalOffset!
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

char* CvtHex(char* dest, int n)
{
    const char* HexD = "0123456789ABCDEF";
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

void Check(int ok)
{
    if (!ok) {
        Fatal("Check failed");
    }
}

#ifndef __SCC__
// Takes up too much code...
#define Check(expr) do { if (!(expr)) { Printf("In %s:%d: ", __FILE__, __LINE__); Fatal(#expr " failed"); } } while (0)
#endif


///////////////////////////////////////////////////////////////////////
// Tokenizer
///////////////////////////////////////////////////////////////////////

const char* IdText(int id)
{
    Check(id >= 0 && id < IdCount);
    return IdBuffer + IdOffset[id];
}

void NextChar(void)
{
    if (InBufCnt == InBufSize) {
        InBufCnt = 0;
        InBufSize = read(InFile, InBuf, INBUF_MAX);
        if (!InBufSize) {
            CurChar = 0;
            return;
        }
    }
    CurChar = InBuf[InBufCnt++];
}

char GetChar(void)
{
    if (StoredSlash) {
        StoredSlash = 0;
        return '/';
    }
    char ch = CurChar;
    NextChar();
    return ch;
}

int TryGetChar(char ch, int t, int p)
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
        if (!CurChar)
            return;
        NextChar();
    }
    NextChar();
    ++Line;
}

void SkipWhitespace(void)
{
Redo:
    while (CurChar <= ' ') {
        if (!CurChar) {
            return;
        }
        if (CurChar == '\n') {
            ++Line;
        }
        NextChar();
    }
    if (CurChar != '/')
        return;
    NextChar();
    if (CurChar == '/') {
        SkipLine();
    } else if (CurChar == '*') {
        NextChar();
        int star = 0;
        while (!star || CurChar != '/') {
            star = CurChar == '*';
            if (CurChar == '\n') ++Line;
            NextChar();
            if (!CurChar)
                Fatal("Unterminated comment");
        }
        NextChar();
    } else {
        StoredSlash = 1;
        return;
    }
    goto Redo;
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
    switch (GetChar()) {
    case 'n':  return '\n';
    case 'r':  return '\r';
    case 't':  return '\t';
    case '\'': return '\'';
    case '"':  return '"';
    case '\\': return '\\';
    case 'x':  break;
    default: Check(0);
    }
    int x = GetDigit()<<4;
    NextChar();
    x |= GetDigit();
    NextChar();
    Check(!(x&0xff00));
    return x;
}

void GetStringLiteral(void)
{
    TokenStrLit = Output + CodeAddress + (64 - CODESTART); // Just leave a little head room for code to be outputted before consuming the string literal
    TokenNumVal = 0;

    char ch;
    for (;;) {
        while ((ch = GetChar()) != '"') {
            if (!ch) {
                Fatal("Unterminated string literal");
            }
            if (ch == '\\') {
                ch = Unescape();
            }
            TokenStrLit[TokenNumVal++] = ch;
        }
        SkipWhitespace();
        if (StoredSlash || CurChar != '"')
            break;
        NextChar();
    }
    TokenStrLit[TokenNumVal++] = 0;
    Check(TokenStrLit+TokenNumVal-Output <= OUTPUT_MAX);
}

void GetToken(void)
{
Redo:
    SkipWhitespace();
    TokenType = GetChar();
    OperatorPrecedence = PRED_STOP;

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
    } else if (TokenType < 'A') {
    } else if (TokenType <= 'Z') {
    Identifier: ;
        char* pc;
        char* start;
        start = pc = &IdBuffer[IdBufferIndex];
        int Hash = HASHINIT*HASHMUL+TokenType;
        *pc++ = TokenType;
        while (CurChar == '_' || isdigit(CurChar) || isalpha(CurChar)) {
            *pc++ = CurChar;
            Hash = Hash*HASHMUL+CurChar;
            NextChar();
        }
        *pc++ = 0;
        int Slot = 0;
        for (;;) {
            Hash &= ID_HASHMAX-1;
            TokenType = IdHashTab[Hash];
            if (TokenType == -1) {
                Check(IdCount < ID_MAX);
                TokenType = IdCount++;
                IdOffset[TokenType] = IdBufferIndex;
                IdBufferIndex += (int)(pc - start);
                Check(IdBufferIndex <= IDBUFFER_MAX);
                IdHashTab[Hash] = TokenType;
                break;
            }
            if (!memcmp(start, IdBuffer + IdOffset[TokenType], pc-start)) {
                break;
            }
            Hash += 1 + Slot++;
        }
        TokenType += TOK_BREAK;
        return;
    } else if (TokenType < 'a') {
        if (TokenType == '_') goto Identifier;
    } else if (TokenType <= 'z') {
        goto Identifier;
    }

    switch (TokenType) {
    case '#':
        SkipLine();
        goto Redo;
    case '\'':
        TokenNumVal = GetChar();
        if (TokenNumVal == '\\') {
            TokenNumVal = Unescape();
        }
        if (GetChar() != '\'') {
            Fatal("Invalid character literal");
        }
        TokenType = TOK_NUM;
        return;
    case '"':
        GetStringLiteral();
        TokenType = TOK_STRLIT;
        return;
    case 0:
    case '(':
    case ')':
    case '{':
    case '}':
    case '[':
    case ']':
    case ':':
    case ';':
    case '~':
        return;
    case ',':
        OperatorPrecedence = PRED_COMMA;
        return;
    case '?':
        OperatorPrecedence = 13;
        return;
    case '=':
        OperatorPrecedence = PRED_EQ;
        TryGetChar('=', TOK_EQEQ, 7);
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
        OperatorPrecedence = 3;
        TryGetChar('=', TOK_SLASHEQ, PRED_EQ);
        return;
    case '%':
        OperatorPrecedence = 3;
        TryGetChar('=', TOK_MODEQ, PRED_EQ);
        return;
    case '.':
        if (CurChar == '.') {
            GetChar();
            if (!TryGetChar('.', TOK_ELLIPSIS, PRED_STOP)) {
                break;
            }
        }
        return;
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
    MODRM_DISP16   = 0x06, // [disp16]
    MODRM_BX       = 0x07, // [BX]
    MODRM_BP_DISP8 = 0x46, // [BP+disp8]
    MODRM_REG      = 0xC0,
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
    I_STOSB          = 0xaa,
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
int EmitChecks(void);

void OutputBytes(int first, ...)
{
    if (EmitChecks()) return;
    char* o = Output - CODESTART;
    va_list vl;
    va_start(vl, first);
    do {
        o[CodeAddress++] = first;
    } while ((first = va_arg(vl, int)) != -1);
    va_end(vl);
    Check(CodeAddress <= OUTPUT_MAX+CODESTART);
}

void OutputWord(int w)
{
    OutputBytes(w&0xff, (w>>8)&0xff, -1);
}

int MakeLabel(void)
{
    Check(LocalLabelCounter < LABEL_MAX);
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
    Check(NamedLabelCount < NAMED_LABEL_MAX);
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
        f  = CodeAddress;
        if (relative) {
            o = r + 2;
            f -= o;
        }
        c = Output + r - CODESTART;
        r = (c[0]&0xff)|(c[1]&0xff)<<8;
        // Is this a uselss jump we can avoid? (Too cumbersome if we already resolved fixups at this address)
        if (CodeAddress == o && LastFixup != CodeAddress) {
            CodeAddress -= 3;
            continue;
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
    Check(l < LocalLabelCounter && !Labels[l].Addr);
    FlushSpAdj();
    IsDeadCode = 0;
    DoFixups(Labels[l].Ref, 1);
    Labels[l].Addr = CodeAddress;
    Labels[l].Ref = 0;
}

void EmitGlobalLabel(struct VarDecl* vd)
{
    Check(!vd->Offset);
    vd->Offset = CodeAddress;
    IsDeadCode = 0;
    DoFixups(vd->Ref, vd->Type & VT_FUN);


    char* Buf = &IdBuffer[IdBufferIndex];
    char* P = CvtHex(Buf, CodeAddress);
    *P++ = ' ';
    P = CopyStr(P, vd->Id == -2 ? "@static" : IdText(vd->Id));
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
    if (Loc == VT_LOCOFF) {
        OutputBytes(Inst, MODRM_BP_DISP8|R, Val & 0xff, -1);
    } else if (Loc == VT_LOCGLOB) {
        OutputBytes(Inst, MODRM_DISP16|R, -1);
        EmitGlobalRef(&VarDecls[Val]);
    } else {
        Check(!Loc);
        OutputBytes(Inst, MODRM_BX|R, -1);
    }
}

void EmitLoadAx(int Size, int Loc, int Val)
{
    EmitModrm(I_MOV_RM_R-1+Size, R_AX, Loc, Val);
    if (Size == 1)
        OutputBytes(I_CBW, -1);
}

void EmitStoreAx(int Size, int Loc, int Val)
{
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

void EmitLeaStackVar(int r, int off)
{
    OutputBytes(I_LEA, MODRM_BP_DISP8 | r<<3, off & 0xff, -1);
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

void EmitToBool(void)
{
    OutputBytes(I_AND|1, MODRM_REG, -1); // AND AX, AX
}

void EmitLocalJump(int l)
{
    Check(l >= 0 && l < LocalLabelCounter);
    int Addr = Labels[l].Addr;
    if (Addr) {
        Addr -= CodeAddress+2;
        if (Addr == (char)Addr) {
            OutputBytes(I_JMP_REL8, Addr & 0xff, -1);
        } else {
            OutputBytes(I_JMP_REL16, -1);
            OutputWord(Addr-1);
        }
    } else {
        OutputBytes(I_JMP_REL16, -1);
        AddFixup(&Labels[l].Ref);
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
    EmitLocalJump(l);
}

void EmitLoadAddr(int Reg, int Loc, int Val)
{
    if (Loc == VT_LOCOFF) {
        EmitLeaStackVar(Reg, Val);
    } else if (Loc == VT_LOCGLOB) {
        OutputBytes(I_MOV_R_IMM16 | Reg, -1);
        EmitGlobalRef(&VarDecls[Val]);
    } else {
        Check(0);
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

void EmitAdjSp(int Amm)
{
    PendingSpAdj += Amm;
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
    if (TokenType == type) {
        GetToken();
        return 1;
    }
    return 0;
}

void Expect(int type)
{
    if (!Accept(type)) {
        PrintTokenType(type);
        Printf("expected got ");
        Unexpected();
    }
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
        Check(AD->Bound);
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
    } else {
        Check(Type == VT_INT || (Type & VT_PTRMASK));
        return 2;
    }
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
            CurrentType &= ~VT_ARRAY;
            CurrentType += VT_PTR1;
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
        if (!loc)
            EmitMovRR(R_BX, R_AX);
        EmitLoadAx(sz, loc, CurrentVal);
    }
}

void DoIncDecOp(int Op, int Post)
{
    Check(CurrentType & VT_LVAL);
    const int WordOp = ((CurrentType&(VT_BASEMASK|VT_PTRMASK)) != VT_CHAR);
    const int Loc = CurrentType & VT_LOCMASK;
    if (!Loc) {
        EmitMovRR(R_BX, R_AX);
    }
    if (Post) {
        EmitLoadAx(1+WordOp, Loc, CurrentVal);
        if (WordOp)
            CurrentType &= ~(VT_LVAL|VT_LOCMASK);
        else
            CurrentType = VT_INT;
    }
    Op = Op == TOK_MINUSMINUS;
    if (CurrentType & VT_PTRMASK) {
        const int Size = SizeofType(CurrentType-VT_PTR1, CurrentTypeExtra);
        if (Size != 1) {
            EmitModrm(I_ALU_RM16_IMM8, Op*(I_SUB>>3), Loc, CurrentVal);
            OutputBytes(Size, -1);
            return;
        }
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
    Check(idx < VARDECL_MAX);

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
    Check(CurrentVal < Scopes[1]);
    return AddVarDeclScope(0, Id);
}

void HandlePrimaryId(int id)
{
    const int vd = VarLookup[id];
    if (vd < 0) {
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
    CurrentType      = VarDecls[vd].Type;
    CurrentTypeExtra = VarDecls[vd].TypeExtra;
    CurrentVal       = VarDecls[vd].Offset;
    if (CurrentType == (VT_LOCLIT | VT_INT)) {
        return;
    }
    const int Loc = CurrentType & VT_LOCMASK;
    if (Loc != VT_LOCOFF) {
        Check(Loc == VT_LOCGLOB);
        CurrentVal = vd;
        if (CurrentType & VT_FUN) {
            return;
        }
    }
    CurrentType |= VT_LVAL;
}

void ParseExpr(void);
void ParseAbstractDecl(void);

void ParsePrimaryExpression(void)
{
    if (Accept('(')) {
        ParseExpr();
        Expect(')');
    } else if (TokenType == TOK_NUM) {
        CurrentType = VT_LOCLIT | VT_INT;
        CurrentVal  = TokenNumVal;
        GetToken();
    } else if (TokenType == TOK_STRLIT) {
        Check(ArrayCount < ARRAY_MAX);
        struct ArrayDecl* AD = &ArrayDecls[ArrayCount];
        AD->Bound = TokenNumVal;
        AD->Extra = 0;
        CurrentType = VT_CHAR | VT_ARRAY | VT_LVAL;
        CurrentTypeExtra = ArrayCount++;
        if (!IsDeadCode) {
            const int EndLab = MakeLabel();
            FlushPushAx();
            EmitJmp(EndLab);
            const int Addr = CodeAddress;
            IsDeadCode = 0;
            while (TokenNumVal--)
                OutputBytes(*TokenStrLit++ & 0xff, -1);
            EmitLocalLabel(EndLab);
            OutputBytes(I_MOV_R_IMM16, -1);
            OutputWord(Addr);
        }
        GetToken();
    } else if (TokenType == TOK_VA_START || TokenType == TOK_VA_END || TokenType == TOK_VA_ARG) {
        // Handle var arg builtins
        const int func = TokenType;
        GetToken();
        Expect('(');
        int id = ExpectId();
        int vd = VarLookup[id];
        if (vd < 0 || VarDecls[vd].Type != (VT_LOCOFF|VT_CHAR|VT_PTR1)) {
            Fatal("Invalid va_list");
        }
        const int offset = VarDecls[vd].Offset;
        if (func == TOK_VA_START) {
            Expect(',');
            id = ExpectId();
            vd = VarLookup[id];
            if (vd < 0 || (VarDecls[vd].Type & VT_LOCMASK) != VT_LOCOFF) {
                Fatal("Invalid argument to va_start");
            }
            EmitLeaStackVar(R_AX, VarDecls[vd].Offset);
            EmitStoreAx(2, VT_LOCOFF, offset);
            CurrentType = VT_VOID;
        } else if (func == TOK_VA_END) {
            CurrentType = VT_VOID;
        } else if (func == TOK_VA_ARG) {
            Expect(',');
            EmitLoadAx(2, VT_LOCOFF, offset);
            EmitAddRegConst(R_AX, 2);
            EmitStoreAx(2, VT_LOCOFF, offset);
            ParseAbstractDecl();
            CurrentType |= VT_LVAL;
        }
        Expect(')');
    } else {
        HandlePrimaryId(ExpectId());
    }
}

void GetVal(void)
{
    if (CurrentType == VT_BOOL) {
        EmitMovRImm(R_AX, 0);
        OutputBytes(0x70 | (CurrentVal^1), 1, I_INC, -1);
        CurrentType = VT_INT;
        return;
    }

    if (CurrentType & VT_LOCMASK) {
        Check(CurrentType == (VT_INT|VT_LOCLIT));
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
    Check(CurrentTypeExtra >= 0 && CurrentTypeExtra < StructCount);
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
        if (Accept('(')) {
            // Function call
            if (!(CurrentType & VT_FUN)) {
                Fatal("Not a function");
            }
            Check((CurrentType & VT_LOCMASK) == VT_LOCGLOB);
            struct VarDecl* Func = &VarDecls[CurrentVal];
            const int RetType = CurrentType & ~(VT_FUN|VT_LOCMASK);
            int NumArgs = 0;
            int StackAdj = 0;
            while (TokenType != ')') {
                enum { ArgsPerChunk = 4 };
                if (NumArgs % ArgsPerChunk == 0) {
                    FlushPushAx();
                    StackAdj += ArgsPerChunk*2;
                    LocalOffset -= ArgsPerChunk*2;
                    EmitAdjSp(-ArgsPerChunk*2);
                    if (NumArgs) {
                        // Move arguments to new stack top
                        EmitPush(R_SI);
                        EmitPush(R_DI);
                        EmitLeaStackVar(R_SI, LocalOffset + ArgsPerChunk*2);
                        EmitMovRR(R_DI, R_SI);
                        EmitAddRegConst(R_DI, -ArgsPerChunk*2);
                        EmitMovRImm(R_CX, NumArgs*2);
                        OutputBytes(I_REP, I_MOVSB, -1);
                        EmitPop(R_DI);
                        EmitPop(R_SI);
                    }
                }
                ParseAssignmentExpression();
                LvalToRval();
                const int off = LocalOffset + NumArgs*2;
                if (CurrentType & VT_LOCMASK) {
                    Check(CurrentType == (VT_INT|VT_LOCLIT));
                    EmitStoreConst(2, VT_LOCOFF, off);
                } else {
                    GetVal();
                    Check(CurrentType == VT_INT || (CurrentType & VT_PTRMASK));
                    EmitStoreAx(2, VT_LOCOFF, off);
                }
                ++NumArgs;

                if (!Accept(',')) {
                    break;
                }
            }
            Expect(')');
            OutputBytes(I_CALL, -1);
            EmitGlobalRefRel(Func);
            if (StackAdj) {
                EmitAdjSp(StackAdj);
                LocalOffset += StackAdj;
            }
            CurrentType = RetType;
            if (CurrentType == VT_CHAR) {
                CurrentType = VT_INT;
            }
            CurrentTypeExtra = Func->TypeExtra;
        } else if (Accept('[')) {
            LvalToRval();
            if (!(CurrentType & VT_PTRMASK)) {
                Fatal("Expected pointer");
            }
            CurrentType -= VT_PTR1;
            const int Scale    = SizeofCurrentType();
            const int ResType  = CurrentType | VT_LVAL;
            const int ResExtra = CurrentTypeExtra;
            if (!IsDeadCode)
                PendingPushAx = 1;
            ParseExpr();
            Expect(']');
            if (CurrentType == (VT_INT|VT_LOCLIT)) {
                Check(PendingPushAx || IsDeadCode);
                PendingPushAx = 0;
                EmitAddRegConst(R_AX, CurrentVal * Scale);
            } else {
                LvalToRval();
                Check(CurrentType == VT_INT);
                EmitScaleAx(Scale);
                Check(!PendingPushAx);
                EmitPop(R_CX);
                LocalOffset += 2;
                OutputBytes(I_ADD|1, MODRM_REG|R_CX<<3, -1);
            }
            CurrentType      = ResType;
            CurrentTypeExtra = ResExtra;
            Check(!(CurrentType & VT_LOCMASK));
        } else if (Accept('.')) {
            Check((CurrentType & ~VT_LOCMASK) == (VT_STRUCT|VT_LVAL));
            HandleStructMember();
        } else if (Accept(TOK_ARROW)) {
            LvalToRval();
            Check((CurrentType & ~VT_LOCMASK) == (VT_STRUCT|VT_PTR1));
            CurrentType -= VT_PTR1;
            HandleStructMember();
        } else {
            const int Op = TokenType;
            if (Op != TOK_PLUSPLUS && Op != TOK_MINUSMINUS) {
                break;
            }
            GetToken();
            DoIncDecOp(Op, 1);
        }
    }
}

void ParseCastExpression(void);

void ParseUnaryExpression(void)
{
    const int Op = TokenType;
    int IsConst = 0;
    if (Op == TOK_PLUSPLUS || Op == TOK_MINUSMINUS) {
        GetToken();
        ParseUnaryExpression();
        DoIncDecOp(Op, 0);
    } else if (Op == '&' || Op == '*' || Op == '+' || Op == '-' || Op == '~' || Op == '!') {
        GetToken();
        ParseCastExpression();
        if (Op == '&') {
            if (!(CurrentType & VT_LVAL)) {
                Fatal("Lvalue required for address-of operator");
            }
            const int loc = CurrentType & VT_LOCMASK;
            if (loc) {
                if (loc == VT_LOCOFF) {
                    EmitLeaStackVar(R_AX, CurrentVal);
                } else if (loc == VT_LOCGLOB) {
                    OutputBytes(I_MOV_R_IMM16, -1);
                    EmitGlobalRef(&VarDecls[CurrentVal]);
                } else {
                    Check(0);
                }
                CurrentType &= ~VT_LOCMASK;
            }
            CurrentType = (CurrentType&~VT_LVAL) + VT_PTR1;
            return;
        }

        if ((CurrentType & VT_LOCMASK) == VT_LOCLIT) {
            Check(CurrentType == (VT_INT|VT_LOCLIT));
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
            Check(IsConst || CurrentType == VT_INT);
        } else if (Op == '-') {
            if (IsConst) {
                CurrentVal = -CurrentVal;
            } else {
                Check(CurrentType == VT_INT);
                OutputBytes(0xF7, 0xD8, -1); // NEG AX
            }
        } else if (Op == '~') {
            if (IsConst) {
                CurrentVal = ~CurrentVal;
            } else {
                Check(CurrentType == VT_INT);
                OutputBytes(0xF7, 0xD0, -1); // NOT AX
            }
        } else if (Op == '!') {
            if (IsConst) {
                CurrentVal = !CurrentVal;
            } else if (CurrentType == VT_BOOL) {
                CurrentVal ^= 1;
            } else {
                Check(CurrentType == VT_INT || (CurrentType & VT_PTRMASK));
                EmitToBool();
                CurrentType = VT_BOOL;
                CurrentVal  = JZ;
            }
        } else {
            Unexpected();
        }
    } else if (Op == TOK_SIZEOF) {
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
        Check(IsDeadCode);
        IsDeadCode = WasDead;
        CurrentVal = SizeofCurrentType();
        CurrentType = VT_LOCLIT | VT_INT;
    } else {
        ParsePrimaryExpression();
        ParsePostfixExpression();
    }
}

void ParseCastExpression(void)
{
    if (Accept('(')) {
        if (IsTypeStart()) {
            ParseAbstractDecl();
            Expect(')');
            const int T = CurrentType;
            const int E = CurrentTypeExtra;
            Check(!(T & VT_LOCMASK));
            ParseCastExpression();
            LvalToRval();
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

int RelOpToCC(int Op)
{
    switch (Op) {
    case '<':       return JL;
    case TOK_LTEQ:  return JNG;
    case '>':       return JG;
    case TOK_GTEQ:  return JNL;
    case TOK_EQEQ:  return JZ;
    case TOK_NOTEQ: return JNZ;
    }
    return -1;
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
    Check(0);
}

// Emit: AX <- AX 'OP' CX, Must preserve BX.
void DoBinOp(int Op)
{
    int RM = MODRM_REG|R_CX<<3;
    int CC = RelOpToCC(Op);
    if (CC >= 0) {
        CurrentType = VT_BOOL;
        CurrentVal  = CC;
        Op = I_CMP|1;
    } else if (Op == '+') {
        Op = I_ADD|1;
    } else if (Op == '-') {
        Op = I_SUB|1;
    } else if (Op == '*') {
        // IMUL : F7 /5
        Op = 0xF7;
        RM = MODRM_REG | (5<<3) | R_CX;
    } else if (Op == '/' || Op == '%') {
        OutputBytes(I_CWD, 0xF7, MODRM_REG | (7<<3) | R_CX, -1);
        if (Op == '%') {
            EmitMovRR(R_AX, R_DX);
        }
        return;
    } else if (Op == '&') {
        Op = I_AND|1;
    } else if (Op == '^') {
        Op = I_XOR|1;
    } else if (Op == '|') {
        Op = I_OR|1;
    } else if (Op == TOK_LSH) {
        Op = 0xd3;
        RM = MODRM_REG|SHROT_SHL<<3;
    } else if (Op == TOK_RSH) {
        Op = 0xd3;
        RM = MODRM_REG|SHROT_SAR<<3;
    } else {
        Check(0);
    }
    OutputBytes(Op, RM, -1);
}

void DoRhsConstBinOp(int Op)
{
    int CC = RelOpToCC(Op);

    if (CC >= 0) {
        CurrentType = VT_BOOL;
        Op = I_CMP|5;
    } else if (Op == '+')  {
Plus:
        EmitAddRegConst(R_AX, CurrentVal);
        return;
    } else if (Op == '-') {
        // Ease optimizations
        CurrentVal = -CurrentVal;
        goto Plus;
    } else if (Op == '&') {
        Op = I_AND|5;
    } else if (Op == '^') {
        Op = I_XOR|5;
    } else if (Op == '|') {
        Op = I_OR|5;
    } else if (Op == '*') {
        EmitScaleAx(CurrentVal);
        return;
    } else if (Op == '/') {
        EmitDivAxConst(CurrentVal);
        return;
    } else {
        EmitMovRImm(R_CX, CurrentVal);
        DoBinOp(Op);
        return;
    }
    OutputBytes(Op, -1);
    OutputWord(CurrentVal);
    if (CC >= 0)
        CurrentVal = CC;
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
    Check(0);
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
    LvalToRval();
    if (CurrentType == VT_BOOL) {
        EmitJcc(CurrentVal^Forward, Label);
    } else if (CurrentType == (VT_LOCLIT|VT_INT)) {
        // Constant condition
        if (!CurrentVal && Forward)
            EmitJmp(Label);
    } else {
        GetVal();
        Check(CurrentType == VT_INT || (CurrentType&VT_PTRMASK));
        EmitToBool();
        EmitJcc(JNZ^Forward, Label);
    }
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
        Check(IsDeadCode);
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
            EmitMovRR(R_BX, R_AX);
        } else {
            EmitPop(R_BX);
            LocalOffset += 2;
        }
    } else {
        Check(LhsLoc == VT_LOCOFF || LhsLoc == VT_LOCGLOB);
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
            if (CurrentType != VT_BOOL) {
                Check(CurrentType == VT_INT || (CurrentType & VT_PTRMASK));
                EmitToBool();
                CurrentVal = JNZ;
            }
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
            Check((CurrentType&~VT_LOCMASK) == (VT_STRUCT|VT_LVAL) && CurrentTypeExtra == LhsTypeExtra);
            Temp = CurrentType & VT_LOCMASK;

            HandleLhsLvalLoc(LhsLoc);
            EmitPush(R_SI);
            EmitPush(R_DI);
            if (LhsLoc)
                EmitLoadAddr(R_DI, LhsLoc, LhsVal);
            else
                EmitMovRR(R_DI, R_BX);
            if (Temp)
                EmitLoadAddr(R_SI, Temp, CurrentVal);
            else
                EmitMovRR(R_SI, R_AX);
            EmitMovRImm(R_CX, SizeofCurrentType());
            OutputBytes(I_REP, I_MOVSB, -1);
            EmitPop(R_DI);
            EmitPop(R_SI);
            continue;
        }

        LvalToRval();

        if (LhsLoc == VT_LOCLIT && CurrentType == (VT_LOCLIT|VT_INT)) {
            Check(LhsType == VT_INT);
            CurrentVal = DoConstBinOp(Op, LhsVal, CurrentVal);
            continue;
        }

        if (LEnd >= 0) {
            GetVal();
            EmitLocalLabel(LEnd);
        } else if (IsAssign) {
            int Size = 2;
            if (LhsType == VT_CHAR) {
                Size = 1;
            } else {
                Check(LhsType == VT_INT || (LhsType & VT_PTRMASK));
            }
            HandleLhsLvalLoc(LhsLoc);
            if (Op != '=') {
                Check(LhsType == VT_INT || (LhsType & (VT_PTR1|VT_CHAR))); // For pointer types only += and -= should be allowed, and only support char* beacause we're lazy
                Temp = CurrentType == (VT_INT|VT_LOCLIT);
                if (!Temp) {
                    Check(CurrentType == VT_INT);
                    EmitMovRR(R_CX, R_AX);
                }
                EmitLoadAx(2, LhsLoc, LhsVal);
                if (Temp) {
                    DoRhsConstBinOp(Op);
                    CurrentType &= ~VT_LOCMASK;
                } else {
                    DoBinOp(Op);
                }
            } else if (CurrentType == (VT_INT|VT_LOCLIT)) {
                // Constant assignment
                EmitStoreConst(Size, LhsLoc, LhsVal);
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
                Check(PendingPushAx || IsDeadCode);
                PendingPushAx = 0;
                CurrentType = VT_INT;
                if (LhsType & VT_PTRMASK) {
                    CurrentVal *= LhsPointeeSize;
                    CurrentType = LhsType;
                }
                DoRhsConstBinOp(Op);
            } else {
                GetVal();
                Check(CurrentType == VT_INT || (CurrentType & VT_PTRMASK));
                if (Op == '+' && (LhsType & VT_PTRMASK)) {
                    EmitScaleAx(LhsPointeeSize);
                    CurrentType = LhsType;
                }
                Temp = OpCommutes(Op);
                if (LhsLoc == VT_LOCLIT) {
                    Check(LhsType == VT_INT);
                    if (Temp) {
                        CurrentVal = LhsVal;
                        DoRhsConstBinOp(Op);
                        continue;
                    } else {
                        EmitMovRImm(R_CX, LhsVal);
                    }
                } else {
                    Check(LhsType == VT_INT || (LhsType & VT_PTRMASK));
                    EmitPop(R_CX);
                    Check(!PendingPushAx);
                    LocalOffset += 2;
                }
                if (!Temp)
                    OutputBytes(I_XCHG_AX | R_CX, -1);
                DoBinOp(Op);
            }
            if (Op == '-' && (LhsType & VT_PTRMASK)) {
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
        if (Accept(TOK_VOID))
            CurrentType = VT_VOID;
        else if (Accept(TOK_CHAR))
            CurrentType = VT_CHAR;
        else if (Accept(TOK_INT))
            CurrentType = VT_INT;
        else if (Accept(TOK_VA_LIST))
            CurrentType = VT_CHAR | VT_PTR1;
        else if (Accept(TOK_CONST))   // Ignore but accept const for now
            ;
        else if (Accept(TOK_STATIC))
            Storage |= VT_STATIC;
        else if (Accept(TOK_ENUM)) {
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
                        Check(CurrentType == (VT_INT|VT_LOCLIT));
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
        } else if (TokenType == TOK_STRUCT || TokenType == TOK_UNION) {
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
                Check(StructCount < STRUCT_MAX);
                const int SI = StructCount++;
                struct StructDecl* SD = &StructDecls[SI];
                SD->Id = id;
                struct StructMember** Last = &SD->Members;
                while (!Accept('}')) {
                    Check(StructMemCount < STRUCT_MEMBER_MAX);
                    ParseAbstractDecl();
                    int BaseType  = CurrentType;
                    int BaseExtra = CurrentTypeExtra;
                    while (TokenType != ';') {
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
        } else {
            break;
        }
    }
    CurrentType |= Storage;
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
        Check(ArrayCount < ARRAY_MAX);
        struct ArrayDecl* AD = &ArrayDecls[ArrayCount];
        AD->Extra = CurrentTypeExtra;
        int T  = CurrentType | VT_ARRAY;
        int TE = ArrayCount++;
        if (TokenType != ']') {
            ParseExpr();
            Check(CurrentType == (VT_INT|VT_LOCLIT)); // Need constant expression
            AD->Bound = CurrentVal;
            Check(AD->Bound > 0);
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

        if (Accept(TOK_CASE)) {
            EnsureSwitchStmt();
        DoCase:
            ParseExpr();
            Expect(':');
            Check(CurrentType == (VT_INT|VT_LOCLIT)); // Need constant expression
            EmitLocalLabel(NextSwitchCase);
            NextSwitchCase = MakeLabel();
            OutputBytes(I_ALU_RM16_IMM16, MODRM_REG|I_CMP|R_SI, -1);
            OutputWord(CurrentVal);
            if (Accept(TOK_CASE)) {
                EmitJcc(JZ, NextSwitchStmt);
                goto DoCase;
            } else {
                EmitJcc(JNZ, NextSwitchCase);
                goto Stmt;
            }
        } else if (Accept(TOK_DEFAULT)) {
            Expect(':');
            Check(SwitchDefault == -1);
            EnsureSwitchStmt();
            SwitchDefault = NextSwitchStmt;
            goto Redo;
        } else if (NextSwitchStmt >= 0) {
            EmitJmp(NextSwitchCase);
        Stmt:
            EmitLocalLabel(NextSwitchStmt);
            NextSwitchStmt = -1;
            goto Redo;
        }
    }

    if (Accept(';')) {
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
                    LvalToRval();
                    GetVal();
                    if (CurrentType == VT_CHAR) {
                        OutputBytes(I_CBW, -1);
                    }
                    EmitPush(R_AX);
                } else {
                    EmitAdjSp(-size);
                }
                LocalOffset -= size;
                vd->Offset = LocalOffset;
            }
            CurrentType      = BaseType;
            CurrentTypeExtra = BaseStruct;
            vd = NextDecl();
        }
        Expect(';');
    } else if (Accept(TOK_RETURN)) {
        if (TokenType != ';') {
            ParseExpr();
            LvalToRval();
            GetVal();
        }
        if (LocalOffset)
            ReturnUsed = 1;
        EmitJmp(ReturnLabel);
        Expect(';');
    } else if (Accept(TOK_BREAK)) {
        EmitAdjSp(BStackLevel - LocalOffset);
        EmitJmp(BreakLabel);
        Expect(';');
    } else if (Accept(TOK_CONTINUE)) {
        EmitAdjSp(CStackLevel - LocalOffset);
        EmitJmp(ContinueLabel);
        Expect(';');
    } else if (Accept(TOK_GOTO)) {
        EmitJmp(GetNamedLabel(ExpectId())->LabelId);
    } else if (Accept(TOK__EMIT)) {
        ParseExpr();
        Check(CurrentType == (VT_INT|VT_LOCLIT)); // Constant expression expected
        OutputBytes(CurrentVal & 0xff, -1);
    } else if (Accept(TOK_IF)) {
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
    } else if (Accept(TOK_FOR)) {
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
    } else if (Accept(TOK_WHILE)) {
        const int StartLabel = MakeLabel();
        const int EndLabel   = MakeLabel();
        EmitLocalLabel(StartLabel);
        Expect('(');
        DoCond(EndLabel, 1);
        Expect(')');
        DoLoopStatements(EndLabel, StartLabel);
        EmitJmp(StartLabel);
        EmitLocalLabel(EndLabel);
    } else if (Accept(TOK_DO)) {
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
        Expect(';');
        EmitLocalLabel(EndLabel);
    } else if (Accept(TOK_SWITCH)) {
        Expect('(');
        ParseExpr();
        LvalToRval();
        GetVal();
        Expect(')');

        const int OldBreakLevel  = BStackLevel;
        const int OldBreakLabel  = BreakLabel;
        const int LastSwitchCase = NextSwitchCase;
        const int LastSwitchStmt = NextSwitchStmt;
        const int LastSwitchDef  = SwitchDefault;
        EmitPush(R_SI);
        EmitMovRR(R_SI, R_AX);
        LocalOffset -= 2;
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
        EmitPop(R_SI);
        LocalOffset += 2;

        NextSwitchCase = LastSwitchCase;
        NextSwitchStmt = LastSwitchStmt;
        SwitchDefault  = LastSwitchDef;
        BStackLevel    = OldBreakLevel;
        BreakLabel     = OldBreakLabel;
    } else {
        // Expression statement / Labelled statement
        if (TokenType >= TOK_ID) {
            const int id = TokenType - TOK_BREAK;
            GetToken();
            if (Accept(':')) {
                EmitLocalLabel(GetNamedLabel(id)->LabelId);
                EmitLeaStackVar(R_SP, LocalOffset);
                goto Redo;
            } else {
                HandlePrimaryId(id);
                ParsePostfixExpression();
                ParseExpr1(PRED_COMMA);
            }
        } else {
            ParseExpr();
        }
        Expect(';');
    }
}

void PushScope(void)
{
    Check(ScopesCount < SCOPE_MAX);
    Scopes[ScopesCount] = Scopes[ScopesCount-1];
    ++ScopesCount;
    ++IgnoreRedef;
}

void PopScope(void)
{
    Check(ScopesCount > 1);
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
        EmitAdjSp(OldOffset - LocalOffset);
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

    // Check for definition of previous forward declaration
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
            Check(!LocalLabelCounter);
            LocalOffset = 0;
            ReturnUsed = 0;
            ReturnLabel = MakeLabel();
            BreakLabel = ContinueLabel = -1;

            EmitGlobalLabel(vd);
            EmitPush(R_BP);
            EmitMovRR(R_BP, R_SP);
            ParseCompoundStatement();
            EmitLocalLabel(ReturnLabel);
            if (ReturnUsed) {
                PendingSpAdj = 0;
                EmitMovRR(R_SP, R_BP);
            }
            EmitPop(R_BP);
            OutputBytes(I_RET, -1); // RET

            // When debugging:
            //int l;for (l = 0; l < LocalLabelCounter; ++l)Check(Labels[l].Ref == 0);
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
            ParseAssignmentExpression();
            Check(CurrentType == (VT_INT|VT_LOCLIT)); // Expecct constant expressions
            // TODO: Could save one byte per global char...
            //       Handle this if/when implementing complex initializers
            EmitGlobalLabel(vd);
            OutputWord(CurrentVal);
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
    Check(IdCount+TOK_BREAK-1 == TOK__EBSS);

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
    Check(ScopesCount == 1);

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
    Check(CodeAddress - SBSS->Offset == BssSize);
    EmitGlobalLabel(EBSS);

    close(MapFile);

    if (argv[2])
       CopyStr(IdBuffer, argv[2]);
    else
        MakeOutputFilename(argv[1], ".com");
    const int OutFile = OpenOutput();
    write(OutFile, Output, CodeAddress - BssSize - CODESTART);
    close(OutFile);

    return 0;
}

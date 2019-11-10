#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
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
#define O_BINARY 0
#include <unistd.h>
#endif

#if 0
// Only used when self-compiling
// Small "standard library"

enum { CREATE_FLAGS = 1 }; // Ugly hack

char* HeapStart;

char* _brk(void);
int main(int argc, char** argv);

char* malloc(int Size)
{
    char* ret;
    ret = HeapStart;
    HeapStart += Size;
    return ret;
}

void exit(int retval)
{
    retval = (retval & 0xff) | 0x4c00;
    _DosCall(&retval, 0, 0, 0);
}

void putchar(int ch)
{
    int ax;
    ax = 0x200;
    _DosCall(&ax, 0, 0, ch);
}

int open(const char* filename, int flags, ...)
{
    int ax;
    if (flags)
        ax = 0x3c00; // create or truncate file
    else
        ax = 0x3d00; // open existing file

    if (_DosCall(&ax, 0, 0, filename))
        return -1;
    return ax;
}

void close(int fd)
{
    int ax;
    ax = 0x3e00;
    _DosCall(&ax, fd, 0, 0);
}

int read(int fd, char* buf, int count)
{
    int ax;
    ax = 0x3f00;
    if (_DosCall(&ax, fd, count, buf))
        return 0;
    return ax;
}

int write(int fd, const char* buf, int count)
{
    int ax;
    ax = 0x4000;
    if (_DosCall(&ax, fd, count, buf))
        return 0;
    return ax;
}

void _start(int Len, char* CmdLine)
{
    char **Args;
    int NumArgs;
    CmdLine[Len] = 0;

    HeapStart = _brk();
    Args = malloc(sizeof(char*)*10);
    Args[0] = "scc";
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
#endif


enum {
    LINEBUF_MAX = 100,
    TMPBUF_MAX = 32,
    INBUF_MAX = 1024,
    SCOPE_MAX = 10,
    VARDECL_MAX = 300,
    ID_MAX = 512, // Must be power of 2
    IDBUFFER_MAX = 4096,
    LABEL_MAX = 300,
    OUTPUT_MAX = 0x6000,
};

enum {
    //DJB2
    HASHINIT = 5381,
    HASHMUL  = 33,
};

enum {
    VT_VOID,
    VT_BOOL,        // CurrentValue holds condition code for "true"
    VT_CHAR,
    VT_INT,

    VT_BASEMASK = 3,

    VT_LVAL    = 1<<2,
    VT_FUN     = 1<<3,

    VT_PTR1    = 1<<4,
    VT_PTRMASK = 3<<4, // 3 levels of indirection should be enough..

    VT_LOCLIT  = 1<<6,    // CurrentVal holds a literal value (or label)
    VT_LOCOFF  = 2<<6,    // CurrentVal holds BP offset
    VT_LOCGLOB = 3<<6,    // CurrentVal holds the VarDecl index of a global
    VT_LOCMASK = 3<<6,
};

enum {
    TOK_EOF,

    TOK_NUM,
    TOK_STRLIT,

    TOK_LPAREN,
    TOK_RPAREN,
    TOK_LBRACE,
    TOK_RBRACE,
    TOK_LBRACKET,
    TOK_RBRACKET,
    TOK_COMMA,
    TOK_SEMICOLON,
    TOK_EQ,
    TOK_EQEQ,
    TOK_NOT,
    TOK_NOTEQ,
    TOK_LT,
    TOK_LTEQ,
    TOK_GT,
    TOK_GTEQ,
    TOK_PLUS,
    TOK_PLUSPLUS,
    TOK_MINUS,
    TOK_MINUSMINUS,
    TOK_STAR,
    TOK_SLASH,
    TOK_MOD,
    TOK_AND,
    TOK_XOR,
    TOK_OR,
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
    TOK_TILDE,
    TOK_DOT,
    TOK_ELLIPSIS,

    // NOTE: Must match order of registration in main
    //       TOK_BREAK must be first
    TOK_BREAK,
    TOK_CHAR,
    TOK_CONST,
    TOK_CONTINUE,
    TOK_ELSE,
    TOK_ENUM,
    TOK_FOR,
    TOK_IF,
    TOK_INT,
    TOK_RETURN,
    TOK_SIZEOF,
    TOK_VOID,
    TOK_WHILE,

    TOK_VA_LIST,
    TOK_VA_START,
    TOK_VA_END,
    TOK_VA_ARG,

    TOK_ID
};

enum {
    CODESTART = 0x100,
};

char* Output;
int CodeAddress = CODESTART;

char* LineBuf;
char* TempBuf;

int InFile;
char* InBuf;
int InBufCnt;
int InBufSize;
char UngottenChar;

int Line = 1;

int TokenType;
int TokenNumVal;

char* IdBuffer;
int IdBufferIndex;

int* IdOffset;
int* IdHashTab;
int IdCount;

int* VarDeclId;
int* VarDeclType;
int* VarDeclOffset;
int* VarDeclRef; // Actually only needed for globals

int* Scopes; // VarDecl index
int ScopesCount;

int LocalLabelCounter;
int* LabelAddr;
int* LabelRef;

int LocalOffset;
int ReturnLabel;
int BCStackLevel; // Break/Continue stack level (== LocalOffset of block)
int BreakLabel;
int ContinueLabel;

int CurrentType;
int CurrentVal;

int ReturnUsed;
int PendingPushAx;
int IsDeadCode;

int IsDigit(int ch)
{
    return ch >= '0' && ch <= '9';
}

int IsAlpha(int ch)
{
    return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z');
}

void PutStr(const char* s)
{
    while (*s)
        putchar(*s++);
}

char* CopyStr(char* dst, const char* src)
{
    while (*src) *dst++ = *src++;
    return dst;
}

int StrLen(const char* s)
{
    int l = 0;
    while (*s++)
        ++l;
    return l;
}

char* VSPrintf(char* dest, const char* format, va_list vl)
{
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
            buf = TempBuf + TMPBUF_MAX;
            *--buf = 0;
            for (;;) {
                *--buf = '0' + n % 10;
                n/=10;
                if (!n) break;
            }
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
            PutStr("Invalid format string");
            exit(1);
        }
    }
    *dest = 0;
    return dest;
}

void Printf(const char* format, ...)
{
    va_list vl;
    va_start(vl, format);
    VSPrintf(LineBuf, format, vl);
    PutStr(LineBuf);
    va_end(vl);
}

int StrEqual(const char* a, const char* b)
{
    while (*a && *b && *a == *b) {
        ++a;
        ++b;
    }
    return !*a && !*b;
}

void Fatal(const char* Msg)
{
    Printf("In line %d: %s\n", Line, Msg);
    exit(1);
}

void Check(int ok)
{
    if (!ok) {
        Fatal("Check failed");
    }
}

#define Check(expr) do { if (!(expr)) { Printf("In %s:%d: ", __FILE__, __LINE__); Fatal(#expr " failed"); } } while (0)

const char* IdText(int id)
{
    Check(id >= 0 && id < IdCount);
    return IdBuffer + IdOffset[id];
}

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
    I_ADD           = 0x00,
    I_OR            = 0x08,
    I_ADC           = 0x10,
    I_SBB           = 0x18,
    I_AND           = 0x20,
    I_SUB           = 0x28,
    I_XOR           = 0x30,
    I_CMP           = 0x38,
    I_INC           = 0x40,
    I_DEC           = 0x48,
    I_PUSH          = 0x50,
    I_POP           = 0x58,
    I_MOV_R_RM      = 0x88,
    I_MOV_RM_R      = 0x8a,
    I_LEA           = 0x8d,
    I_XCHG_AX       = 0x90,
    I_CBW           = 0x98,
    I_CWD           = 0x99,
    I_MOV_R_IMM16   = 0xb8,
    I_RET           = 0xc3,
    I_INT           = 0xcd,
    I_SHROT16_CL    = 0xd3,
    I_CALL          = 0xe8,
    I_JMP           = 0xe9,
};

enum {
    SHROT_SHL = 4,
    SHROT_SAR = 7,
};

int EmitChecks(void);

void OutputBytes(int first, ...)
{
    if (EmitChecks()) return;
    char* o = Output - CODESTART;
    va_list vl;
    va_start(vl, first);
    o[CodeAddress++] = first;
    while ((first = va_arg(vl, int)) != -1) {
        o[CodeAddress++] = first;
    }
    va_end(vl);
}

void OutputWord(int w)
{
    OutputBytes(w&0xff, (w>>8)&0xff, -1);
}

int MakeLabel(void)
{
    Check(LocalLabelCounter < LABEL_MAX);
    const int l = LocalLabelCounter++;
    LabelAddr[l] = 0;
    LabelRef[l] = 0;
    return l;
}

void ResetLabels(void)
{
    int l;
    for (l = 0; l < LocalLabelCounter; ++l)
        Check(LabelRef[l] == 0);
    LocalLabelCounter = 0;
}

void DoFixups(int r)
{
    int f;
    char* c;
    while (r) {
        f  = CodeAddress - r - 2;
        c = Output + r - CODESTART;
        r = (c[0]&0xff)|(c[1]&0xff)<<8;
        c[0] = f;
        c[1] = f>>8;
    }
}

void AddFixup(int* f)
{
    OutputWord(*f);
    *f = CodeAddress - 2;
}

void EmitLocalLabel(int l)
{
    Check(l < LocalLabelCounter && !LabelAddr[l]);
    LabelAddr[l] = CodeAddress;
    IsDeadCode = 0;
    DoFixups(LabelRef[l]);
    LabelRef[l] = 0;
}

void EmitLocalRef(int l)
{
    int Addr = LabelAddr[l];
    Check(Addr);
    OutputWord(Addr);
}

void EmitLocalRefRel(int l)
{
    int Addr = LabelAddr[l];
    if (Addr) {
        OutputWord(Addr - CodeAddress - 2);
    } else {
        AddFixup(LabelRef + l);
    }
}

void EmitGlobalLabel(int VarDeclIndex)
{
    Check(!VarDeclOffset[VarDeclIndex]);
    VarDeclOffset[VarDeclIndex] = CodeAddress;
    IsDeadCode = 0;
    DoFixups(VarDeclRef[VarDeclIndex]);
}

void EmitGlobalRef(int vd)
{
    int Addr = VarDeclOffset[vd];
    Check(Addr);
    OutputWord(Addr);
}

void EmitGlobalRefRel(int vd)
{
    int Addr = VarDeclOffset[vd];
    if (Addr) {
        OutputWord(Addr - CodeAddress - 2);
    } else {
        AddFixup(VarDeclRef + vd);
    }
}

void EmitModrm(int Inst, int Loc, int Val)
{
    if (Loc == VT_LOCOFF) {
        OutputBytes(Inst, MODRM_BP_DISP8, Val & 0xff, -1);
    } else if (Loc == VT_LOCGLOB) {
        OutputBytes(Inst, MODRM_DISP16, -1);
        EmitGlobalRef(Val);
    } else {
        Check(!Loc);
        OutputBytes(Inst, 0x07, -1);
    }
}

void EmitLoadAx(int Size, int Loc, int Val)
{
    EmitModrm(I_MOV_RM_R-1+Size, Loc, Val);
    if (Size == 1)
        OutputBytes(I_CBW, -1);
}

void EmitStoreAx(int Size, int Loc, int Val)
{
    EmitModrm(I_MOV_R_RM-1+Size, Loc, Val);
}

void EmitStoreConst(int Size, int Loc, int Val)
{
    EmitModrm(0xc6-1+Size, Loc, Val);
    OutputBytes(CurrentVal & 0xff, - 1);
    if (Size == 2)
        OutputBytes((CurrentVal >> 8) & 0xff, - 1);
}

void EmitAdjSp(int Amm)
{
    OutputBytes(0x83, 0xC4, Amm & 0xff, -1);
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
    OutputBytes(I_MOV_R_IMM16 | r, val & 0xff, (val >> 8) & 0xff, -1);
}

void EmitLeaStackVar(int r, int off)
{
    OutputBytes(I_LEA, MODRM_BP_DISP8 | r<<3, off & 0xff, -1);
}

void EmitToBool(void)
{
    OutputBytes(I_AND|1, MODRM_REG, -1); // AND AX, AX
}

void EmitJmp(int l)
{
    PendingPushAx = 0;
    if (IsDeadCode) {
        return;
    }
    OutputBytes(I_JMP, -1);
    EmitLocalRefRel(l);
    IsDeadCode = 1;
}

void EmitJcc(int cc, int l)
{
    OutputBytes(0x70 | (cc^1), 3, -1); // Skip jump
    OutputBytes(I_JMP, -1);
    EmitLocalRefRel(l);
}

void EmitCall(int Func)
{
    OutputBytes(I_CALL, -1);
    EmitGlobalRefRel(Func);
}

int EmitChecks(void)
{
    if (IsDeadCode) {
        return 1;
    }
    if (PendingPushAx) {
        PendingPushAx = 0;
        EmitPush(R_AX);
    }
    return 0;
}

int AddVarDecl(int Type, int Id)
{
    int vd;
    Check(ScopesCount);
    Check(Scopes[ScopesCount-1] < VARDECL_MAX-1);
    vd = ++Scopes[ScopesCount-1];
    VarDeclType[vd]   = Type;
    VarDeclId[vd]     = Id;
    VarDeclOffset[vd] = 0;
    VarDeclRef[vd]    = 0;
    return vd;
}

void PushScope(void)
{
    int End;
    Check(ScopesCount < SCOPE_MAX);
    if (ScopesCount)
        End = Scopes[ScopesCount-1];
    else
        End = -1;
    Scopes[ScopesCount++] = End;
}

void PopScope(void)
{
    Check(ScopesCount);
    --ScopesCount;
}

int Lookup(int id)
{
    int vd;
    Check(ScopesCount);
    for (vd = Scopes[ScopesCount-1]; vd >= 0; vd--) {
        if (VarDeclId[vd] == id) {
            break;
        }
    }
    return vd;
}

char GetChar(void)
{
    if (UngottenChar) {
        char ch;
        ch = UngottenChar;
        UngottenChar = 0;
        return ch;
    }
    if (InBufCnt == InBufSize) {
        InBufCnt = 0;
        InBufSize = read(InFile, InBuf, INBUF_MAX);
        if (!InBufSize) {
            return 0;
        }
    }
    return InBuf[InBufCnt++];
}

void UnGetChar(char ch)
{
    UngottenChar = ch;
}

int TryGetChar(char ch)
{
    char ch2;
    if ((ch2 = GetChar()) != ch) {
        UnGetChar(ch2);
        return 0;
    }
    return 1;
}

void SkipLine(void)
{
    int ch;
    while ((ch = GetChar()) != '\n' && ch)
        ;
    ++Line;
}

void SkipWhitespace(void)
{
    char ch;
    while ((ch = GetChar()) <= ' ') {
        if (!ch) {
            return;
        }
        if (ch == '\n') {
            ++Line;
        }
    }
    UnGetChar(ch);
}

char Unescape(void)
{
    char ch;
    ch = GetChar();
    if (ch == 'n') {
        return '\n';
    } else if (ch == 'r') {
        return '\r';
    } else if (ch == 't') {
        return '\t';
    } else if (ch == '\'') {
        return '\'';
    } else if (ch == '"') {
        return '"';
    } else if (ch == '\\') {
        return '\\';
    } else {
        Printf("TODO: Escaped character literal \\%c\n", ch);
        Fatal("Unsupported character literal");
    }
}

void GetStringLiteral(void)
{
    TokenNumVal = MakeLabel();
    const int WasDeadCode = IsDeadCode;
    const int JL = MakeLabel();
    EmitJmp(JL);
    EmitLocalLabel(TokenNumVal);
    IsDeadCode = WasDeadCode;

    char ch;
    for (;;) {
        while ((ch = GetChar()) != '"') {
            if (ch == '\\') {
                ch = Unescape();
            }
            if (!ch) {
                Fatal("Unterminated string literal");
            }
            OutputBytes(ch, -1);
        }
        SkipWhitespace();
        ch = GetChar();
        if (ch != '"') {
            UnGetChar(ch);
            break;
        }
    }
    OutputBytes(0, -1);
    EmitLocalLabel(JL);
}

void AddIdHash(int Id, int Hash)
{
    for (;;) {
        Hash &= ID_MAX-1;
        if (IdHashTab[Hash] == -1) {
            break;
        }
        ++Hash;
    }
    IdHashTab[Hash] = Id;
}

int GetStrIdHash(const char* Str, int Hash)
{
    int Id;
    for (;; ++Hash) {
        Hash &= ID_MAX-1;
        Id = IdHashTab[Hash];
        if (Id == -1) {
            break;
        }
        if (StrEqual(Str, IdText(Id))) {
            break;
        }
    }
    return Id;
}

int HashStr(const char* Str)
{
    int Hash = HASHINIT;
    while (*Str) {
        Hash = Hash*HASHMUL + *Str++;
    }
    return Hash;
}

int GetStrId(const char* Str)
{
    return GetStrIdHash(Str, HashStr(Str));
}

void GetToken(void)
{
    char ch;

    SkipWhitespace();
    ch = GetChar();

    if (!ch) {
        TokenType = TOK_EOF;
        return;
    }

    if (ch == '#') {
        SkipLine();
        GetToken();
        return;
    } else if (IsDigit(ch)) {
        TokenNumVal = 0;
        int base = 10;
        if (ch == '0') {
            base = 8;
            ch = GetChar();
            if (ch == 'x' || ch == 'X') {
                base = 16;
                ch = GetChar();
            }
        }
        for (;;) {
            if (IsDigit(ch)) {
                ch -= '0';
            } else if (ch >= 'A' && ch <= 'F') {
                ch -= 'A'-10;
            } else if (ch >= 'a' && ch <= 'f') {
                ch -= 'a'-10;
            }
            if (ch < 0 || ch >= base)
                break;
            TokenNumVal = TokenNumVal*base + ch;
            ch = GetChar();
        }
        UnGetChar(ch);
        TokenType = TOK_NUM;
    } else if (IsAlpha(ch) || ch == '_') {
        char* pc;
        char* start;
        start = pc = &IdBuffer[IdBufferIndex];
        *pc++ = ch;
        int Hash = HASHINIT*HASHMUL+ch;
        for (;;) {
            ch = GetChar();
            if (ch != '_' && !IsDigit(ch) && !IsAlpha(ch)) {
                break;
            }
            *pc++ = ch;
            Hash = Hash*HASHMUL+ch;
        }
        *pc++ = 0;
        UnGetChar(ch);
        TokenType = GetStrIdHash(start, Hash);
        if (TokenType < 0) {
            Check(IdCount < ID_MAX);
            TokenType = IdCount++;
            IdOffset[TokenType] = IdBufferIndex;
            IdBufferIndex += pc - start;
            AddIdHash(TokenType, Hash);
        }
        TokenType += TOK_BREAK;
    } else if (ch == '\'') {
        ch = GetChar();
        if (ch == '\\') {
            TokenNumVal = Unescape();
        } else {
            TokenNumVal = ch;
        }
        if (GetChar() != '\'') {
            Fatal("Invalid character literal");
        }
        TokenType = TOK_NUM;
    } else if (ch == '"') {
        GetStringLiteral();
        TokenType = TOK_STRLIT;
    } else if (ch == '(') { TokenType = TOK_LPAREN;    }
    else if (ch == ')') { TokenType = TOK_RPAREN;    }
    else if (ch == '{') { TokenType = TOK_LBRACE;    }
    else if (ch == '}') { TokenType = TOK_RBRACE;    }
    else if (ch == '[') { TokenType = TOK_LBRACKET;  }
    else if (ch == ']') { TokenType = TOK_RBRACKET;  }
    else if (ch == ',') { TokenType = TOK_COMMA;     }
    else if (ch == ';') { TokenType = TOK_SEMICOLON; }
    else if (ch == '~') { TokenType = TOK_TILDE;     }
    else if (ch == '=') {
        TokenType = TOK_EQ;
        if (TryGetChar('=')) {
            TokenType = TOK_EQEQ;
        }
    } else if (ch == '!') {
        TokenType = TOK_NOT;
        if (TryGetChar('=')) {
            TokenType = TOK_NOTEQ;
        }
    } else if (ch == '<') {
        TokenType = TOK_LT;
        if (TryGetChar('<')) {
            TokenType = TOK_LSH;
            if (TryGetChar('=')) {
                TokenType = TOK_LSHEQ;
            }
        } else if (TryGetChar('=')) {
            TokenType = TOK_LTEQ;
        }
    } else if (ch ==  '>') {
        TokenType = TOK_GT;
        if (TryGetChar('>')) {
            TokenType = TOK_RSH;
            if (TryGetChar('=')) {
                TokenType = TOK_RSHEQ;
            }
        } else if (TryGetChar('=')) {
            TokenType = TOK_GTEQ;
        }
    } else if (ch == '&') {
        TokenType = TOK_AND;
        if (TryGetChar('&')) {
            TokenType = TOK_ANDAND;
        } else if (TryGetChar('=')) {
            TokenType = TOK_ANDEQ;
        }
    } else if (ch == '|') {
        TokenType = TOK_OR;
        if (TryGetChar('|')) {
            TokenType = TOK_OROR;
        } else if (TryGetChar('=')) {
            TokenType = TOK_OREQ;
        }
    } else if (ch == '^') {
        TokenType = TOK_XOR;
        if (TryGetChar('=')) {
            TokenType = TOK_XOREQ;
        }
    } else if (ch == '+') {
        TokenType = TOK_PLUS;
        if (TryGetChar('+')) {
            TokenType = TOK_PLUSPLUS;
        } else if (TryGetChar('=')) {
            TokenType = TOK_PLUSEQ;
        }
    } else if (ch == '-') {
        TokenType = TOK_MINUS;
        if (TryGetChar('-')) {
            TokenType = TOK_MINUSMINUS;
        } else if (TryGetChar('=')) {
            TokenType = TOK_MINUSEQ;
        }
    } else if (ch == '*') {
        TokenType = TOK_STAR;
        if (TryGetChar('=')) {
            TokenType = TOK_STAREQ;
        }
    } else if (ch == '/') {
        if (TryGetChar('/')) {
            SkipLine();
            GetToken();
            return;
        }
        TokenType = TOK_SLASH;
        if (TryGetChar('=')) {
            TokenType = TOK_SLASHEQ;
        }
    } else if (ch == '%') {
        TokenType = TOK_MOD;
        if (TryGetChar('=')) {
            TokenType = TOK_MODEQ;
        }
    } else if (ch == '.') {
        TokenType = TOK_DOT;
        if (TryGetChar('.')) {
            if (!TryGetChar('.')) {
                Fatal("Invalid token ..");
            }
            TokenType = TOK_ELLIPSIS;
        }
    } else {
        Printf("Unknown token start '%c' (%d)\n", ch, ch);
        Fatal("Unknown token encountered");
    }
}

enum {
    PRED_EQ = 14,
    PRED_COMMA,
};

int OperatorPrecedence(int tok)
{
    if (tok == TOK_STAR || tok == TOK_SLASH || tok == TOK_MOD) {
        return 3;
    } else if (tok == TOK_PLUS || tok == TOK_MINUS) {
        return 4;
    } else if (tok == TOK_LSH || tok == TOK_RSH) {
        return 5;
    } else if (tok == TOK_LT || tok == TOK_LTEQ || tok == TOK_GT || tok == TOK_GTEQ) {
        return 6;
    } else if (tok == TOK_EQEQ || tok == TOK_NOTEQ) {
        return 7;
    } else if (tok == TOK_AND) {
        return 8;
    } else if (tok == TOK_XOR) {
        return 9;
    } else if (tok == TOK_OR) {
        return 10;
    } else if (tok == TOK_ANDAND) {
        return 11;
    } else if (tok == TOK_OROR) {
        return 12;
    } else if (tok == TOK_EQ || tok == TOK_PLUSEQ || tok == TOK_MINUSEQ || tok == TOK_STAREQ || tok == TOK_SLASHEQ || tok == TOK_MODEQ || tok == TOK_LSHEQ || tok == TOK_RSHEQ || tok == TOK_ANDEQ || tok == TOK_XOREQ || tok == TOK_OREQ) {
        return PRED_EQ;
    } else if (tok == TOK_COMMA) {
        return PRED_COMMA;
    } else {
        return 100;
    }
}

void Unexpected(void)
{
    Printf("token type %d\n", TokenType);
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
        Printf("Token type %d expected got ", type);
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
    return TokenType == TOK_CONST
        || TokenType == TOK_VOID
        || TokenType == TOK_CHAR
        || TokenType == TOK_INT
        || TokenType == TOK_VA_LIST;
}

void LvalToRval(void)
{
    if (CurrentType & VT_LVAL) {
        const int loc = CurrentType & VT_LOCMASK;
        CurrentType &= ~(VT_LVAL | VT_LOCMASK);
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

void DoIncDec(int Op)
{
    if (Op == TOK_PLUSPLUS)
        Op = I_INC;
    else
        Op = I_DEC;

    int A2;
    if (CurrentType == VT_CHAR || CurrentType == VT_INT || CurrentType == (VT_CHAR|VT_PTR1)) {
        A2 = -1;
    } else {
        A2 = Op; // Repeat Op
    }
    OutputBytes(Op, A2, -1);
}

void DoIncDecOp(int Op, int Post)
{
    Check(CurrentType & VT_LVAL);
    CurrentType &= ~VT_LVAL;
    const int loc = CurrentType & VT_LOCMASK;
    if (loc) {
        CurrentType &= ~VT_LOCMASK;
    } else {
        EmitMovRR(R_BX, R_AX);
    }
    char Sz = 2;
    if (CurrentType == VT_CHAR) {
        Sz = 1;
    }
    EmitLoadAx(Sz, loc, CurrentVal);
    if (Post) EmitPush(R_AX);
    DoIncDec(Op);
    EmitStoreAx(Sz, loc, CurrentVal);
    if (Sz == 1) {
        CurrentType = VT_INT;
    }
    if (Post) EmitPop(R_AX);
}

void ParseExpr(void);
void ParseCastExpression(void);
void ParseAssignmentExpression(void);
int ParseDeclSpecs(void);

void ParsePrimaryExpression(void)
{
    int id;
    int vd;
    if (TokenType == TOK_LPAREN) {
        GetToken();
        ParseExpr();
        Expect(TOK_RPAREN);
    } else if (TokenType == TOK_NUM) {
        CurrentType = VT_LOCLIT | VT_INT;
        CurrentVal  = TokenNumVal;
        GetToken();
    } else if (TokenType == TOK_STRLIT) {
        OutputBytes(I_MOV_R_IMM16, -1);
        EmitLocalRef(TokenNumVal);
        CurrentType = VT_PTR1 | VT_CHAR;
        GetToken();
    } else if (TokenType == TOK_VA_START || TokenType == TOK_VA_END || TokenType == TOK_VA_ARG) {
        // Handle var arg builtins
        const int func = TokenType;
        GetToken();
        Expect(TOK_LPAREN);
        id = ExpectId();
        vd = Lookup(id);
        if (vd < 0 || VarDeclType[vd] != (VT_LOCOFF|VT_CHAR|VT_PTR1)) {
            Fatal("Invalid va_list");
        }
        const int offset = VarDeclOffset[vd];
        if (func == TOK_VA_START) {
            Expect(TOK_COMMA);
            id = ExpectId();
            vd = Lookup(id);
            if (vd < 0 || (VarDeclType[vd] & VT_LOCMASK) != VT_LOCOFF) {
                Fatal("Invalid argument to va_start");
            }
            EmitLeaStackVar(R_AX, VarDeclOffset[vd]);
            EmitStoreAx(2, VT_LOCOFF, offset);
            CurrentType = VT_VOID;
        } else if (func == TOK_VA_END) {
            CurrentType = VT_VOID;
        } else if (func == TOK_VA_ARG) {
            Expect(TOK_COMMA);
            EmitLoadAx(2, VT_LOCOFF, offset);
            OutputBytes(I_INC, I_INC, -1); // INC AX \ INC AX
            EmitStoreAx(2, VT_LOCOFF, offset);
            CurrentType = VT_LVAL | ParseDeclSpecs();
        }
        Expect(TOK_RPAREN);
    } else {
        id = ExpectId();
        vd = Lookup(id);
        if (vd < 0) {
            // Lookup failed. TODO: Assume function returning int. (Needs to be global...)
            if (IsDeadCode) {
                // Hack: Allow undefined functions in dead code
                CurrentType = VT_LOCGLOB|VT_FUN|VT_INT;
                CurrentVal  = 0;
                return;
            }
            Printf("Undefined identifier: \"%s\"\n", IdText(id));
            Fatal("TODO");
        }
        CurrentType = VarDeclType[vd];
        CurrentVal = VarDeclOffset[vd];
        const int Loc = CurrentType & VT_LOCMASK;
        if (Loc == VT_LOCLIT) {
            Check(CurrentType == (VT_LOCLIT | VT_INT));
            return;
        }
        CurrentType |= VT_LVAL;
        if (Loc != VT_LOCOFF) {
            Check(Loc == VT_LOCGLOB);
            CurrentVal = vd;
            if (CurrentType & VT_FUN) {
                CurrentType &= ~VT_LVAL;
            }
        }
    }
}

void GetVal(void)
{
    if (CurrentType == VT_BOOL) {
        const int Lab = MakeLabel();
        EmitMovRImm(R_AX, 1);
        EmitJcc(CurrentVal, Lab);
        OutputBytes(I_DEC, -1);
        EmitLocalLabel(Lab);
        CurrentType = VT_INT;
        return;
    }

    const int loc = CurrentType & VT_LOCMASK;
    if (loc) {
        Check(loc == VT_LOCLIT);
        CurrentType &= ~VT_LOCMASK;
        EmitMovRImm(R_AX, CurrentVal);
    }
}

enum { MaxArgs = 4 }; // TODO: Handle this better...
void ParsePostfixExpression(void)
{
    for (;;) {
        if (Accept(TOK_LPAREN)) {
            // Function call
            if (!(CurrentType & VT_FUN)) {
                Fatal("Not a function");
            }
            Check((CurrentType & VT_LOCMASK) == VT_LOCGLOB);
            const int Func    = CurrentVal;
            const int RetType = CurrentType & ~(VT_FUN|VT_LOCMASK);
            int NumArgs = 0;
            while (TokenType != TOK_RPAREN) {
                if (!NumArgs) {
                    EmitAdjSp(-MaxArgs*2);
                    LocalOffset -= MaxArgs*2;
                }
                Check(NumArgs < MaxArgs);
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

                if (!Accept(TOK_COMMA)) {
                    break;
                }
            }
            Expect(TOK_RPAREN);
            EmitCall(Func);
            if (NumArgs) {
                EmitAdjSp(MaxArgs*2);
                LocalOffset += MaxArgs*2;
            }
            CurrentType = RetType;
            if (CurrentType == VT_CHAR) {
                CurrentType = VT_INT;
            }
        } else if (Accept(TOK_LBRACKET)) {
            LvalToRval();
            if (!(CurrentType & VT_PTRMASK)) {
                Fatal("Expected pointer");
            }
            const int AType = CurrentType - VT_PTR1;
            int Double = 0;
            if (AType != VT_CHAR) {
                Check(AType == VT_INT || (AType & VT_PTRMASK));
                Double = 1;
            }
            EmitPush(R_AX);
            ParseExpr();
            Expect(TOK_RBRACKET);
            if (CurrentType == (VT_INT|VT_LOCLIT)) {
                if (Double) CurrentVal <<= 1;
                EmitPop(R_AX);
                OutputBytes(I_ADD|5, CurrentVal&0xff, (CurrentVal>>8)&0xff, -1);
            } else {
                LvalToRval();
                Check(CurrentType == VT_INT);
                if (Double) {
                    OutputBytes(I_ADD|1, MODRM_REG, -1);
                }
                EmitPop(R_CX);
                OutputBytes(I_ADD|1, MODRM_REG|R_CX<<3, -1);
            }
            CurrentType = AType | VT_LVAL;
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

void ParseUnaryExpression(void)
{
    const int Op = TokenType;
    int IsConst = 0;
    if (Op == TOK_PLUSPLUS || Op == TOK_MINUSMINUS) {
        GetToken();
        ParseUnaryExpression();
        DoIncDecOp(Op, 0);
    } else if (Op == TOK_AND || Op == TOK_STAR || Op == TOK_PLUS || Op == TOK_MINUS || Op == TOK_TILDE || Op == TOK_NOT) {
        GetToken();
        ParseCastExpression();
        if ((CurrentType & VT_LOCMASK) == VT_LOCLIT) {
            Check(CurrentType == (VT_INT|VT_LOCLIT));
            IsConst = 1;
        } else if (Op != TOK_AND) {
            LvalToRval();
        }
        if (Op == TOK_AND) {
            if (!(CurrentType & VT_LVAL)) {
                Fatal("Lvalue required for address-of operator");
            }
            const int loc = CurrentType & VT_LOCMASK;
            if (loc) {
                if (loc == VT_LOCOFF)
                    EmitLeaStackVar(R_AX, CurrentVal);
                else
                    Check(0);
                CurrentType &= ~VT_LOCMASK;
            }
            CurrentType = (CurrentType&~VT_LVAL) + VT_PTR1;
        } else if (Op == TOK_STAR) {
            if (!(CurrentType & VT_PTRMASK)) {
                Fatal("Pointer required for dereference");
            }
            CurrentType = (CurrentType-VT_PTR1) | VT_LVAL;
        } else if (Op == TOK_PLUS) {
            Check(IsConst || CurrentType == VT_INT);
        } else if (Op == TOK_MINUS) {
            if (IsConst) {
                CurrentVal = -CurrentVal;
            } else {
                Check(CurrentType == VT_INT);
                OutputBytes(0xF7, 0xD8, -1); // NEG AX
            }
        } else if (Op == TOK_TILDE) {
            if (IsConst) {
                CurrentVal = ~CurrentVal;
            } else {
                Check(CurrentType == VT_INT);
                OutputBytes(0xF7, 0xD0, -1); // NOT AX
            }
        } else if (Op == TOK_NOT) {
            if (IsConst) {
                CurrentVal = !CurrentVal;
            } else {
                int Lab;
                Lab = MakeLabel();
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
        Expect(TOK_LPAREN);
        if (Accept(TOK_CHAR)) {
            CurrentVal = 1;
        } else if (Accept(TOK_INT)) {
            CurrentVal = 2;
        } else {
            Fatal("sizeof not implemented for this type");
        }
        while (Accept(TOK_STAR)) {
            CurrentVal = 2;
        }
        Expect(TOK_RPAREN);
        CurrentType = VT_LOCLIT | VT_INT;
    } else {
        ParsePrimaryExpression();
        ParsePostfixExpression();
    }
}

void ParseCastExpression(void)
{
    ParseUnaryExpression();
}

int RelOpToCC(int Op)
{
    if (Op == TOK_LT)         return JL;
    else if (Op == TOK_LTEQ)  return JNG;
    else if (Op == TOK_GT)    return JG;
    else if (Op == TOK_GTEQ)  return JNL;
    else if (Op == TOK_EQEQ)  return JZ;
    else if (Op == TOK_NOTEQ) return JNZ;
    Fatal("Not implemented");
}

int IsRelOp(int Op)
{
    return Op == TOK_LT || Op == TOK_LTEQ || Op == TOK_GT || Op == TOK_GTEQ || Op == TOK_EQEQ || Op == TOK_NOTEQ;
}

int RemoveAssign(int Op)
{
    if (Op == TOK_PLUSEQ)  return TOK_PLUS;
    if (Op == TOK_MINUSEQ) return TOK_MINUS;
    if (Op == TOK_STAREQ)  return TOK_STAR;
    if (Op == TOK_SLASHEQ) return TOK_SLASH;
    if (Op == TOK_MODEQ)   return TOK_MOD;
    if (Op == TOK_LSHEQ)   return TOK_LSH;
    if (Op == TOK_RSHEQ)   return TOK_RSH;
    if (Op == TOK_ANDEQ)   return TOK_AND;
    if (Op == TOK_XOREQ)   return TOK_XOR;
    if (Op == TOK_OREQ)    return TOK_OR;
    Check(0);
}

// Emit: AX <- AX 'OP' CX, Must preserve BX.
void DoBinOp(int Op)
{
    int RM = MODRM_REG|R_CX<<3;
    if (IsRelOp(Op)) {
        CurrentType = VT_BOOL;
        CurrentVal  = RelOpToCC(Op);
        Op = I_CMP;
    } else if (Op == TOK_PLUS) {
        Op = I_ADD;
    } else if (Op == TOK_MINUS) {
        Op = I_SUB;
    } else if (Op == TOK_STAR) {
        // IMUL : F7 /5
        Op = 0xF7;
        RM = MODRM_REG | (5<<3) | R_CX;
    } else if (Op == TOK_SLASH || Op == TOK_MOD) {
        OutputBytes(I_CWD, 0xF7, MODRM_REG | (7<<3) | R_CX, -1);
        if (Op == TOK_MOD) {
            EmitMovRR(R_AX, R_DX);
        }
        return;
    } else if (Op == TOK_AND) {
        Op = I_AND;
    } else if (Op == TOK_XOR) {
        Op = I_XOR;
    } else if (Op == TOK_OR) {
        Op = I_OR;
    } else if (Op == TOK_LSH) {
        Op = 0xd3; // Don't care about |1 below
        RM = MODRM_REG|SHROT_SHL<<3;
    } else if (Op == TOK_RSH) {
        Op = 0xd3;
        RM = MODRM_REG|SHROT_SAR<<3;
    } else {
        Check(0);
    }
    OutputBytes(Op|1, RM, -1);
}

void DoRhsConstBinOp(int Op)
{
    int NextVal = CurrentVal;
    if (IsRelOp(Op)) {
        CurrentType = VT_BOOL;
        NextVal = RelOpToCC(Op);
        Op = I_CMP;
    }
    else if (Op == TOK_PLUS)  Op = I_ADD;
    else if (Op == TOK_MINUS) Op = I_SUB;
    else if (Op == TOK_AND)   Op = I_AND;
    else if (Op == TOK_XOR)   Op = I_XOR;
    else if (Op == TOK_OR)    Op = I_OR;
    else {
        EmitMovRImm(R_CX, CurrentVal);
        DoBinOp(Op);
        return;
    }
    OutputBytes(Op|5, CurrentVal & 0xff, (CurrentVal>>8) & 0xff, -1);
    CurrentVal = NextVal;
}

int DoConstBinOp(int Op, int L, int R)
{
    if (Op == TOK_PLUS)  return L + R;
    if (Op == TOK_MINUS) return L - R;
    if (Op == TOK_STAR)  return L * R;
    if (Op == TOK_SLASH) return L / R;
    if (Op == TOK_MOD)   return L % R;
    if (Op == TOK_AND)   return L & R;
    if (Op == TOK_XOR)   return L ^ R;
    if (Op == TOK_OR)    return L | R;
    if (Op == TOK_LSH)   return L << R;
    if (Op == TOK_RSH)   return L >> R;
    Check(0);
}

int OpCommutes(int Op)
{
    return Op == TOK_PLUS || Op == TOK_STAR || Op == TOK_EQ || Op == TOK_NOTEQ
        || Op == TOK_AND || Op == TOK_XOR || Op == TOK_OR;
}

void ParseExpr1(int OuterPrecedence)
{
    int LEnd;
    int Temp;
    int Op;
    int Prec;
    int IsAssign;
    int LhsType;
    int LhsVal;
    int LhsLoc;
    for (;;) {
        Op   = TokenType;
        Prec = OperatorPrecedence(Op);
        if (Prec > OuterPrecedence) {
            break;
        }
        GetToken();

        IsAssign = Prec == PRED_EQ;

        if (IsAssign) {
            if (!(CurrentType & VT_LVAL)) {
                Fatal("L-value required");
            }
            if (Op != TOK_EQ)
                Op = RemoveAssign(Op);
            CurrentType &= ~VT_LVAL;
        } else {
            LvalToRval();
        }
        LhsType = CurrentType;
        LhsVal = CurrentVal;

        if (Op == TOK_ANDAND || Op == TOK_OROR) {
            LEnd = MakeLabel();
            if (CurrentType == VT_BOOL) {
                Temp = MakeLabel();
                if (Op != TOK_ANDAND)
                    CurrentVal ^= 1;
                EmitJcc(CurrentVal, Temp);
                EmitMovRImm(R_AX, Op != TOK_ANDAND);
                EmitJmp(LEnd);
                EmitLocalLabel(Temp);
            } else {
                Check(CurrentType == VT_INT);
                EmitToBool();
                if (Op == TOK_ANDAND)
                    EmitJcc(JZ, LEnd);
                else
                    EmitJcc(JNZ, LEnd);
            }
        } else {
            LEnd = -1;
            if (!(LhsType & VT_LOCMASK)) {
                Check(!PendingPushAx);
                PendingPushAx = 1;
            }
        }

        // TODO: Question, ?:
        ParseCastExpression(); // RHS
        for (;;) {
            const int LookAheadOp         = TokenType;
            const int LookAheadPrecedence = OperatorPrecedence(LookAheadOp);
            if (LookAheadPrecedence > Prec || (LookAheadPrecedence == Prec && LookAheadPrecedence < PRED_EQ)) // LookAheadOp < PRED_EQ => !IsRightAssociative
                break;
            ParseExpr1(LookAheadPrecedence);
        }
        LvalToRval();
        LhsLoc = LhsType & VT_LOCMASK;
        LhsType &= ~VT_LOCMASK;

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
            if (!LhsLoc) {
                if (PendingPushAx) {
                    PendingPushAx = 0;
                    EmitMovRR(R_BX, R_AX);
                } else {
                    EmitPop(R_BX);
                }
            } else {
                Check(LhsLoc == VT_LOCOFF || LhsLoc == VT_LOCGLOB);
            }
            if (Op != TOK_EQ) {
                Check(LhsType == VT_INT || (LhsType & (VT_PTR1|VT_CHAR))); // For pointer types only += and -= should be allowed, and only support char* beacause we're lazy
                Temp = CurrentType == (VT_INT|VT_LOCLIT);
                if (!Temp) {
                    Check(CurrentType == VT_INT);
                    EmitMovRR(R_CX, R_AX);
                }
                EmitLoadAx(2, LhsLoc, LhsVal);
                if (Temp)
                    DoRhsConstBinOp(Op);
                else
                    DoBinOp(Op);
            } else if (CurrentType == (VT_INT|VT_LOCLIT)) {
                // Constant assignment
                EmitStoreConst(Size, LhsLoc, LhsVal);
                continue;
            } else {
                GetVal();
            }
            EmitStoreAx(Size, LhsLoc, LhsVal);
        } else {
            if (CurrentType == (VT_LOCLIT|VT_INT)) {
                Check(PendingPushAx);
                PendingPushAx = 0;
                CurrentType = VT_INT;
                DoRhsConstBinOp(Op);
            } else {
                Check(CurrentType == VT_INT || (Op == TOK_MINUS && (CurrentType & VT_PTRMASK)));
                if (Op == TOK_PLUS && (LhsType & VT_PTRMASK) && LhsType != (VT_CHAR|VT_PTR1)) {
                    OutputBytes(I_ADD|1, MODRM_REG, -1);
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
                }
                if (!Temp)
                    OutputBytes(I_XCHG_AX | R_CX, -1);
                DoBinOp(Op);
            }
            if (Op == TOK_MINUS && (LhsType & VT_PTRMASK)) {
                if (LhsType != (VT_CHAR|VT_PTR1))
                    OutputBytes(0xD1, MODRM_REG | SHROT_SAR<<3, -1); // SAR AX, 1
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

int ParseDeclSpecs(void)
{
    Check(IsTypeStart());
    if (TokenType == TOK_CONST) {
        // Ignore
        GetToken();
        Check(IsTypeStart());
    }
    int t;
    if (TokenType == TOK_VOID) {
        t = VT_VOID;
    } else if (TokenType == TOK_CHAR) {
        t = VT_CHAR;
    } else if (TokenType == TOK_INT) {
        t = VT_INT;
    } else if (TokenType == TOK_VA_LIST) {
        GetToken();
        return VT_CHAR | VT_PTR1;
    } else {
        Unexpected();
    }
    GetToken();
    while (TokenType == TOK_STAR) {
        t += VT_PTR1;
        GetToken();
    }
    return t;
}

int ParseDecl(void)
{
    const int Type = ParseDeclSpecs();
    const int Id   = ExpectId();
    return AddVarDecl(Type, Id);
}

void DoJumpFalse(int FalseLabel)
{
    ParseExpr();
    LvalToRval();
    if (CurrentType == VT_BOOL) {
        EmitJcc(CurrentVal^1, FalseLabel);
    } else if (CurrentType == (VT_LOCLIT|VT_INT)) {
        // Constant condition
        if (!CurrentVal)
            EmitJmp(FalseLabel);
    } else {
        GetVal();
        Check(CurrentType == VT_INT);
        EmitToBool();
        EmitJcc(JZ, FalseLabel);
    }
}

void ParseCompoundStatement(void);

void ParseStatement(void)
{
    const int OldBreak    = BreakLabel;
    const int OldContinue = ContinueLabel;

    if (Accept(TOK_SEMICOLON)) {
    } else if (TokenType == TOK_LBRACE) {
        ParseCompoundStatement();
    } else if (IsTypeStart()) {
        int vd;
        vd = ParseDecl();
        VarDeclType[vd] |= VT_LOCOFF;
        if (Accept(TOK_EQ)) {
            ParseAssignmentExpression();
            LvalToRval();
            GetVal();
            if (CurrentType == VT_CHAR) {
                OutputBytes(I_CBW, -1);
            }
        }
        LocalOffset -= 2;
        VarDeclOffset[vd] = LocalOffset;
        // Note: AX contains "random" garbage at first if the variable isn't initialized
        EmitPush(R_AX);
        Expect(TOK_SEMICOLON);
    } else if (Accept(TOK_FOR)) {
        const int CondLabel = MakeLabel();
        const int BodyLabel = MakeLabel();
        const int EndLabel  = MakeLabel();
        int IterLabel       = CondLabel;
        
        Expect(TOK_LPAREN);

        // Init
        if (TokenType != TOK_SEMICOLON) {
            ParseExpr();
        }
        Expect(TOK_SEMICOLON);

        // Cond
        EmitLocalLabel(CondLabel);
        if (TokenType != TOK_SEMICOLON) {
            DoJumpFalse(EndLabel);
        }
        Expect(TOK_SEMICOLON);

        // Iter
        if (TokenType != TOK_RPAREN) {
            EmitJmp(BodyLabel);
            IterLabel  = MakeLabel();
            EmitLocalLabel(IterLabel);
            ParseExpr();
            EmitJmp(CondLabel);
        }
        Expect(TOK_RPAREN);

        EmitLocalLabel(BodyLabel);
        BreakLabel    = EndLabel;
        ContinueLabel = IterLabel;
        const int OldBCStack = BCStackLevel;
        BCStackLevel  = LocalOffset;
        ParseStatement();
        BCStackLevel  = OldBCStack;
        EmitJmp(IterLabel);
        EmitLocalLabel(EndLabel);
    } else if (Accept(TOK_IF)) {
        const int ElseLabel = MakeLabel();

        Accept(TOK_LPAREN);
        DoJumpFalse(ElseLabel);
        Accept(TOK_RPAREN);
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
    } else if (Accept(TOK_RETURN)) {
        if (TokenType != TOK_SEMICOLON) {
            ParseExpr();
            LvalToRval();
            GetVal();
        }
        if (LocalOffset)
            ReturnUsed = 1;
        EmitJmp(ReturnLabel);
        Expect(TOK_SEMICOLON);
    } else if (Accept(TOK_WHILE)) {
        const int StartLabel = MakeLabel();
        const int EndLabel   = MakeLabel();
        EmitLocalLabel(StartLabel);
        Expect(TOK_LPAREN);
        DoJumpFalse(EndLabel);
        Expect(TOK_RPAREN);
        BreakLabel = EndLabel;
        ContinueLabel = StartLabel;
        const int OldBCStack = BCStackLevel;
        BCStackLevel  = LocalOffset;
        ParseStatement();
        BCStackLevel  = OldBCStack;
        EmitJmp(StartLabel);
        EmitLocalLabel(EndLabel);
    } else if (Accept(TOK_BREAK)) {
        Check(BreakLabel >= 0);
        if (LocalOffset != BCStackLevel)
            EmitAdjSp(BCStackLevel - LocalOffset);
        EmitJmp(BreakLabel);
        Expect(TOK_SEMICOLON);
    } else if (Accept(TOK_CONTINUE)) {
        Check(ContinueLabel >= 0);
        if (LocalOffset != BCStackLevel)
            EmitAdjSp(BCStackLevel - LocalOffset);
        EmitJmp(ContinueLabel);
        Expect(TOK_SEMICOLON);
    } else {
        // Expression statement
        ParseExpr();
        Expect(TOK_SEMICOLON);
    }

    BreakLabel    = OldBreak;
    ContinueLabel = OldContinue;
}

void ParseCompoundStatement(void)
{
    const int InitialOffset = LocalOffset;
    PushScope();
    Expect(TOK_LBRACE);
    while (!Accept(TOK_RBRACE)) {
        ParseStatement();
    }
    PopScope();
    if (InitialOffset != LocalOffset) {
        EmitAdjSp(InitialOffset - LocalOffset);
        LocalOffset = InitialOffset;
    }
}

// Arg 2              BP + 6
// Arg 1              BP + 4
// Return Address     BP + 2
// Old BP             BP      <-- BP
// Local 1            BP - 2
// Local 2            BP - 4  <-- SP

void EmitGlobal(int VarDeclIndex, int InitVal)
{
    EmitGlobalLabel(VarDeclIndex);
    OutputWord(InitVal);
}

void ParseExternalDefition(void)
{
    if (Accept(TOK_ENUM)) {
        int id;
        int vd;
        Expect(TOK_LBRACE);
        int EnumVal = 0;
        while (TokenType != TOK_RBRACE) {
            id = ExpectId();
            if (Accept(TOK_EQ)) {
                ParseAssignmentExpression();
                Check(CurrentType == (VT_INT|VT_LOCLIT));
                EnumVal = CurrentVal;
            }
            vd = AddVarDecl(VT_INT|VT_LOCLIT, id);
            VarDeclOffset[vd] = EnumVal;
            if (!Accept(TOK_COMMA)) {
                break;
            }
            ++EnumVal;
        }
        Expect(TOK_RBRACE);
        Expect(TOK_SEMICOLON);
        return;
    }

    const int Type = ParseDeclSpecs();
    const int Id   = ExpectId();

    int fd = Lookup(Id);
    if (fd < 0) {
        fd = AddVarDecl(Type | VT_LOCGLOB, Id);
    } else {
        Check(VarDeclOffset[fd] == 0);
        Check((VarDeclType[fd] & VT_LOCMASK) == VT_LOCGLOB);
    }

    VarDeclType[fd] |= VT_LOCGLOB;

    // Variable?
    if (Accept(TOK_EQ)) {
        ParseAssignmentExpression();
        Check(CurrentType == (VT_INT|VT_LOCLIT)); // Expecct constant expressions
        EmitGlobal(fd, CurrentVal);
        Expect(TOK_SEMICOLON);
        return;
    } else if (Accept(TOK_SEMICOLON)) {
        EmitGlobal(fd, 0);
        return;
    }

    Expect(TOK_LPAREN);
    VarDeclType[fd] |= VT_FUN;
    PushScope();
    int ArgOffset;
    int vd;
    ArgOffset = 4;
    while (TokenType != TOK_RPAREN) {
        if (Accept(TOK_VOID) || Accept(TOK_ELLIPSIS)) {
            break;
        }
        vd = ParseDecl();
        VarDeclType[vd] |= VT_LOCOFF;
        VarDeclOffset[vd] = ArgOffset;
        ArgOffset += 2;
        if (!Accept(TOK_COMMA)) {
            break;
        }
    }
    Expect(TOK_RPAREN);
    if (!Accept(TOK_SEMICOLON)) {
        Check(!LocalLabelCounter);
        LocalOffset = 0;
        ReturnUsed = 0;
        ReturnLabel = MakeLabel();
        BreakLabel = ContinueLabel = -1;
        EmitGlobalLabel(fd);
        EmitPush(R_BP);
        EmitMovRR(R_BP, R_SP);
        ParseCompoundStatement();
        EmitLocalLabel(ReturnLabel);
        if (ReturnUsed)
            EmitMovRR(R_SP, R_BP);
        EmitPop(R_BP);
        OutputBytes(I_RET, -1); // RET
        ResetLabels();
    }
    PopScope();
}

#define CREATE_FLAGS O_WRONLY | O_CREAT | O_TRUNC | O_BINARY

void MakeOutputFilename(char* dest, const char* n)
{
    char* LastDot;
    LastDot = 0;
    while (*n) {
        if (*n == '.')
            LastDot = dest;
        *dest++ = *n++;
    }
    if (!LastDot) LastDot = dest;
    *CopyStr(LastDot, ".com") = 0;
}

int AddId(const char* s)
{
    const int Id = IdCount++;
    int Hash = HASHINIT;
    char ch;
    IdOffset[Id] = IdBufferIndex;
    while (1) {
        if ((ch = *s++) <= ' ')
            break;
        IdBuffer[IdBufferIndex++] = ch;
        Hash = Hash*HASHMUL+ch;
    }
    IdBuffer[IdBufferIndex++] = 0;
    AddIdHash(Id, Hash);
    return Id;
}

void AddBuiltins(const char* s)
{
    for (;;) {
        AddId(s);
        while (*s > ' ')
            ++s;
        if (!*s++) break;
    }
    Check(IdCount+TOK_BREAK-1 == TOK_VA_ARG);
}

int main(int argc, char** argv)
{
    int i;

    Output        = malloc(OUTPUT_MAX);
    LineBuf       = malloc(LINEBUF_MAX);
    TempBuf       = malloc(TMPBUF_MAX);
    InBuf         = malloc(INBUF_MAX);
    IdBuffer      = malloc(IDBUFFER_MAX);
    IdOffset      = malloc(sizeof(int)*ID_MAX);
    IdHashTab     = malloc(sizeof(int)*ID_MAX);
    VarDeclId     = malloc(sizeof(int)*VARDECL_MAX);
    VarDeclType   = malloc(sizeof(int)*VARDECL_MAX);
    VarDeclOffset = malloc(sizeof(int)*VARDECL_MAX);
    VarDeclRef    = malloc(sizeof(int)*VARDECL_MAX);
    Scopes        = malloc(sizeof(int)*SCOPE_MAX);
    LabelAddr     = malloc(sizeof(int)*LABEL_MAX);
    LabelRef      = malloc(sizeof(int)*LABEL_MAX);

    for (i = 0; i < ID_MAX; ++i)
        IdHashTab[i] = -1;

    if (argc < 2) {
        Printf("Usage: %s input-file\n", argv[0]);
        return 1;
    }

    InFile = open(argv[1], 0); // O_RDONLY
    if (InFile < 0) {
        Fatal("Error opening input file");
    }

    AddBuiltins("break char const continue else enum for if int return sizeof void while va_list va_start va_end va_arg");

    PushScope();

    // Prelude
    OutputBytes(I_XOR|1, MODRM_REG|R_BP<<3|R_BP, -1);
    EmitMovRImm(R_AX, 0x81);
    EmitPush(R_AX);
    OutputBytes(0xA0, 0x80, 0x00, -1); // MOV AL, [0x80]
    EmitPush(R_AX);
    EmitCall(AddVarDecl(VT_FUN|VT_VOID|VT_LOCGLOB, AddId("_start")));
    EmitGlobalLabel(AddVarDecl(VT_FUN|VT_INT|VT_LOCGLOB, AddId("_DosCall")));
    EmitPush(R_BP);
    EmitMovRR(R_BP, R_SP);
    OutputBytes(I_MOV_RM_R|1, MODRM_BP_DISP8|R_BX<<3, 4, -1);
    OutputBytes(I_MOV_RM_R|1, MODRM_BX, -1);
    OutputBytes(I_MOV_RM_R|1, MODRM_BP_DISP8|R_BX<<3, 6, -1);
    OutputBytes(I_MOV_RM_R|1, MODRM_BP_DISP8|R_CX<<3, 8, -1);
    OutputBytes(I_MOV_RM_R|1, MODRM_BP_DISP8|R_DX<<3, 10, -1);
    OutputBytes(I_INT, 0x21, -1);
    OutputBytes(I_MOV_RM_R|1, MODRM_BP_DISP8|R_BX<<3, 4, -1);
    OutputBytes(I_MOV_R_RM|1, MODRM_BX, -1);
    EmitMovRImm(R_AX, 0);
    OutputBytes(I_SBB|1, MODRM_REG, -1);
    EmitPop(R_BP);
    OutputBytes(I_RET, -1);

    GetToken();
    while (TokenType != TOK_EOF) {
        ParseExternalDefition();
    }

    // Postlude
    int id = GetStrId("_brk");
    if (id >= 0) {
        int vd = Lookup(id);
        if (vd >= 0) {
            Check(VarDeclType[vd] == (VT_FUN|VT_CHAR|VT_PTR1|VT_LOCGLOB));
            EmitGlobalLabel(vd);
            EmitMovRImm(R_AX, CodeAddress+4);
            OutputBytes(I_RET, -1);
        }
    }

    PopScope();
    close(InFile);

    for (i = 0 ; i < Scopes[0]; ++i) {
        const int Loc = VarDeclType[i] & VT_LOCMASK;
        if (Loc == VT_LOCGLOB && !VarDeclOffset[i]) {
            Printf("%s is undefined (Type: %X)\n", IdText(VarDeclId[i]), VarDeclType[i]);
            Fatal("Undefined function");
        }
    }

    MakeOutputFilename(IdBuffer, argv[1]);
    const int OutFile = open(IdBuffer, CREATE_FLAGS, 0600);
    if (OutFile < 0) {
        Fatal("Error creating output file");
    }
    write(OutFile, Output, CodeAddress - CODESTART);
    close(OutFile);

    return 0;
}

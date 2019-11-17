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
#ifndef O_BINARY
#define O_BINARY 0
#endif
#include <unistd.h>
#endif

#define CREATE_FLAGS O_WRONLY | O_CREAT | O_TRUNC | O_BINARY

#if 0
// Only used when self-compiling
// Small "standard library"

enum { CREATE_FLAGS = 1 }; // Ugly hack

int main(int argc, char** argv);
int DosCall(int* ax, int bx, int cx, int dx);

char* HeapStart;

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
    DosCall(&retval, 0, 0, 0);
}

void putchar(int ch)
{
    int ax;
    ax = 0x200;
    DosCall(&ax, 0, 0, ch);
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

void _start(void)
{
    // Clear BSS
    char* bss = &_SBSS;
    while (bss != &_EBSS)
        *bss++ = 0;

    char* CmdLine = (char*)0x80;
    int Len = *CmdLine++ & 0xff;
    CmdLine[Len] = 0;

    char **Args;
    int NumArgs;

    HeapStart = &_EBSS;
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
    VARDECL_MAX = 350,
    ID_MAX = 550,
    ID_HASHMAX = 1024, // Must be power of 2 and (some what) greater than ID_MAX
    IDBUFFER_MAX = 5000,
    LABEL_MAX = 300,
    NAMED_LABEL_MAX = 10,
    OUTPUT_MAX = 0x6400, // Decrease this if increasing/adding other buffers..
    STRUCT_MAX = 8,
    STRUCT_MEMBER_MAX = 32,
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

    VT_PTR1    = 1<<5,
    VT_PTRMASK = 3<<5, // 3 levels of indirection should be enough..

    VT_LOCLIT  = 1<<7,    // CurrentVal holds a literal value (or label)
    VT_LOCOFF  = 2<<7,    // CurrentVal holds BP offset
    VT_LOCGLOB = 3<<7,    // CurrentVal holds the VarDecl index of a global
    VT_LOCMASK = 3<<7,
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
    TOK_COLON,
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
    TOK_STRUCT,
    TOK_SWITCH,
    TOK_VOID,
    TOK_WHILE,

    TOK_VA_LIST,
    TOK_VA_START,
    TOK_VA_END,
    TOK_VA_ARG,

    TOK__EMIT,

    TOK_ID
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

struct StructDecl {
    int Id;
    struct StructMember* Members;
};

struct VarDecl {
    int Id;
    int Type;
    int TypeExtra;
    int Offset;
    int Ref; // Actually only needed for globals
};

char* Output;
int CodeAddress = CODESTART;
int BssSize;

char* LineBuf;
char* TempBuf;

int InFile;
char* InBuf;
int InBufCnt;
int InBufSize;
char CurChar;
char StoredSlash;

int Line = 1;

int TokenType;
int TokenNumVal;

char* IdBuffer;
int IdBufferIndex;

int* IdOffset;
int* IdHashTab;
int IdCount;

struct StructMember* StructMembers;
int StructMemCount;

struct StructDecl* StructDecls;
int StructCount;

struct VarDecl* VarDecls;

struct VarDecl* MemcpyDecl;
int MemcpyUsed;

int* Scopes; // VarDecl index
int ScopesCount;

struct Label* Labels;
int LocalLabelCounter;

struct NamedLabel* NamedLabels;
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
int CurrentStruct;

int ReturnUsed;
int PendingPushAx; // Remember to adjsust LocalOffset!
int PendingSpAdj;
int PendingJump = -1;
int IsDeadCode;

int NextSwitchCase = -1; // Where to go if we haven't matched
int NextSwitchStmt;      // Where to go if we've matched (-1 if already emitted)
int SwitchDefault;       // Optional switch default label

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
    *dst = 0;
    return dst;
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
            buf = TempBuf + TMPBUF_MAX;
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

#if 0
int* GetBP(void)
{
    _emit 0x8B _emit 0x46 _emit 0x00 // MOV AX, [BP]
}
#endif

void Fatal(const char* Msg)
{
#if 0
    int* BP = GetBP();
    Printf("BP   Return address\n");
    while (*BP) {
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
    I_RET            = 0xc3,
    I_INT            = 0xcd,
    I_SHROT16_CL     = 0xd3,
    I_CALL           = 0xe8,
    I_JMP_REL16      = 0xe9,
    I_JMP_REL8       = 0xeb,
    I_INCDEC_RM      = 0xfe,
};

enum {
    SHROT_SHL = 4,
    SHROT_SAR = 7,
};

void FlushSpAdj(void);
void FlushJump(void);
int EmitChecks(void);

void OutputBytes(int first, ...)
{
    if (EmitChecks()) return;
    char* o = Output - CODESTART;
    va_list vl;
    va_start(vl, first);
    do {
        Check(CodeAddress < OUTPUT_MAX+CODESTART);
        o[CodeAddress++] = (char)first;
    } while ((first = va_arg(vl, int)) != -1);
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
    Labels[l].Addr = 0;
    Labels[l].Ref  = 0;
    return l;
}

struct NamedLabel* GetNamedLabel(int Id)
{
    struct NamedLabel* NL = NamedLabels;
    int l = NamedLabelCount;
    while (l--) {
        if (NL->Id == Id) {
            goto Ret;
        }
    }
    Check(NamedLabelCount < NAMED_LABEL_MAX);
    ++NamedLabelCount;
    NL->Id = Id;
    NL->LabelId = MakeLabel();
Ret:
    return NL;
}

void ResetLabels(void)
{
    int l;
    for (l = 0; l < LocalLabelCounter; ++l)
        Check(Labels[l].Ref == 0);
    LocalLabelCounter = 0;
    NamedLabelCount = 0;
}

void DoFixups(int r, int relative)
{
    int f;
    char* c;
    while (r) {
        f  = CodeAddress;
        if (relative)
            f -= r + 2;
        c = Output + r - CODESTART;
        r = (c[0]&0xff)|(c[1]&0xff)<<8;
        c[0] = (char)(f);
        c[1] = (char)(f>>8);
    }
}

void AddFixup(int* f)
{
    OutputWord(*f);
    *f = CodeAddress - 2;
}

void EmitLocalLabel(int l)
{
    Check(l < LocalLabelCounter && !Labels[l].Addr);
    if (PendingJump == l) {
        PendingJump = -1;
    }
    FlushJump();
    FlushSpAdj();
    Labels[l].Addr = CodeAddress;
    IsDeadCode = 0;
    DoFixups(Labels[l].Ref, 1);
    Labels[l].Ref = 0;
}

void EmitLocalRef(int l)
{
    int Addr = Labels[l].Addr;
    Check(Addr);
    OutputWord(Addr);
}

void EmitGlobalLabel(struct VarDecl* vd)
{
    Check(!vd->Offset);
    vd->Offset = CodeAddress;
    IsDeadCode = 0;
    DoFixups(vd->Ref, vd->Type & VT_FUN);
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
    } else {
        Check(Amm < 128);
        OutputBytes(I_ALU_RM16_IMM8, MODRM_REG|Op|r, Amm & 0xff, -1);
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
        PendingJump = l;
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
    FlushJump();
}

void EmitCall(struct VarDecl* Func)
{
    Check(Func->Type & VT_FUN);
    OutputBytes(I_CALL, -1);
    EmitGlobalRefRel(Func);
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
    PendingSpAdj = 0;
    EmitAddRegConst(R_SP, Amm);
}

void FlushPushAx(void)
{
    if (PendingPushAx) {
        PendingPushAx = 0;
        EmitPush(R_AX);
        LocalOffset -= 2;
    }
}

void FlushJump(void)
{
    if (PendingJump != -1) {
        int* f = &Labels[PendingJump].Ref;
        PendingJump = -1;
        IsDeadCode = 0;
        OutputBytes(I_JMP_REL16, -1);
        AddFixup(f);
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
    FlushJump();
    return 0;
}

void SetPendingPushAx(void)
{
    Check(!PendingPushAx && !PendingSpAdj);
    PendingPushAx = 1;
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

int TryGetChar(char ch)
{
    if (CurChar == ch) {
        NextChar();
        return 1;
    }
    return 0;
}

void SkipLine(void)
{
    while (CurChar && CurChar != '\n')
        NextChar();
    if (CurChar) NextChar();
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
    if (CurChar == '/') {
        NextChar();
        if (CurChar == '/') {
            SkipLine();
        } else if (CurChar == '*') {
            NextChar();
            int star = 0;
            while (!star || CurChar != '/') {
                star = CurChar == '*';
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
}

char Unescape(void)
{
    char ch = GetChar();
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
        if (StoredSlash || CurChar != '"')
            break;
        NextChar();
    }
    OutputBytes(0, -1);
    EmitLocalLabel(JL);
}

void AddIdHash(int Id, int Hash)
{
    int i = 0;
    for (;;) {
        Hash &= ID_HASHMAX-1;
        if (IdHashTab[Hash] == -1) {
            break;
        }
        Hash += 1 + i++;
    }
    IdHashTab[Hash] = Id;
}

int GetStrIdHash(const char* Str, int Hash)
{
    int Id;
    int i = 0;
    for (;;) {
        Hash &= ID_HASHMAX-1;
        Id = IdHashTab[Hash];
        if (Id == -1) {
            break;
        }
        if (StrEqual(Str, IdText(Id))) {
            break;
        }
        Hash += 1 + i++;
    }
    return Id;
}

void GetToken(void)
{
Redo:
    SkipWhitespace();
    char ch = GetChar();
    if (!ch) {
        TokenType = TOK_EOF;
        return;
    }
    if (ch == '#') {
        SkipLine();
        goto Redo;
    } else if (IsDigit(ch)) {
        TokenNumVal = ch - '0';
        int base = 10;
        if (ch == '0') {
            base = 8;
            if (CurChar == 'x' || CurChar == 'X') {
                base = 16;
                NextChar();
            }
        }
        for (;;) {
            if (IsDigit(CurChar)) {
                CurChar -= '0';
            } else if (IsAlpha(CurChar)) {
                CurChar = (CurChar & 0xdf) - ('A'-10);
            } else {
                break;
            }
            if (CurChar < 0 || CurChar >= base)
                break;
            TokenNumVal = TokenNumVal*base + CurChar;
            NextChar();
        }
        TokenType = TOK_NUM;
    } else if (IsAlpha(ch) || ch == '_') {
        char* pc;
        char* start;
        start = pc = &IdBuffer[IdBufferIndex];
        int Hash = HASHINIT*HASHMUL+ch;
        *pc++ = ch;
        while (CurChar == '_' || IsDigit(CurChar) || IsAlpha(CurChar)) {
            *pc++ = CurChar;
            Hash = Hash*HASHMUL+CurChar;
            NextChar();
        }
        *pc++ = 0;
        TokenType = GetStrIdHash(start, Hash);
        if (TokenType < 0) {
            Check(IdCount < ID_MAX);
            TokenType = IdCount++;
            IdOffset[TokenType] = IdBufferIndex;
            IdBufferIndex += (int)(pc - start);
            Check(IdBufferIndex <= IDBUFFER_MAX);
            AddIdHash(TokenType, Hash);
        }
        TokenType += TOK_BREAK;
    } else if (ch == '\'') {
        TokenNumVal = GetChar();
        if (TokenNumVal == '\\') {
            TokenNumVal = Unescape();
        }
        if (GetChar() != '\'') {
            Fatal("Invalid character literal");
        }
        TokenType = TOK_NUM;
    } else if (ch == '"') {
        GetStringLiteral();
        TokenType = TOK_STRLIT;
    }
    else if (ch == '(') { TokenType = TOK_LPAREN;    }
    else if (ch == ')') { TokenType = TOK_RPAREN;    }
    else if (ch == '{') { TokenType = TOK_LBRACE;    }
    else if (ch == '}') { TokenType = TOK_RBRACE;    }
    else if (ch == '[') { TokenType = TOK_LBRACKET;  }
    else if (ch == ']') { TokenType = TOK_RBRACKET;  }
    else if (ch == ',') { TokenType = TOK_COMMA;     }
    else if (ch == ':') { TokenType = TOK_COLON;     }
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
        } else if (TryGetChar('>')) {
            TokenType = TOK_ARROW;
        }
    } else if (ch == '*') {
        TokenType = TOK_STAR;
        if (TryGetChar('=')) {
            TokenType = TOK_STAREQ;
        }
    } else if (ch == '/') {
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

int SizeofType(int Type, int Extra)
{
    Type &= VT_BASEMASK|VT_PTRMASK;
    if (Type == VT_CHAR) {
        return 1;
    } else if (Type == VT_STRUCT) {
        int Size = 0;
        struct StructMember* SM = StructDecls[Extra].Members;
        for (; SM; SM = SM->Next) {
            Size += SizeofType(SM->Type, SM->TypeExtra);
        }
        return Size;
    } else {
        Check(Type == VT_INT || (Type & VT_PTRMASK));
        return 2;
    }
}

int SizeofCurrentType(void)
{
    return SizeofType(CurrentType, CurrentStruct);
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
        || TokenType == TOK_ENUM
        || TokenType == TOK_STRUCT
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
        const int Size = SizeofType(CurrentType-VT_PTR1, CurrentStruct);
        if (Size != 1) {
            EmitModrm(I_ALU_RM16_IMM8, Op*(I_SUB>>3), Loc, CurrentVal);
            OutputBytes(Size, -1);
            return;
        }
    }

    EmitModrm(I_INCDEC_RM|WordOp, Op, Loc, CurrentVal);
}

void ParseExpr(void);
void ParseCastExpression(void);
void ParseAssignmentExpression(void);
void ParseDeclSpecs(void);

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
        if (VarDecls[vd].Id == id) {
            break;
        }
    }
    return vd;
}

void HandlePrimaryId(int id)
{
    const int vd = Lookup(id);
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
    CurrentType   = VarDecls[vd].Type;
    CurrentStruct = VarDecls[vd].TypeExtra;
    CurrentVal    = VarDecls[vd].Offset;
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

void ParsePrimaryExpression(void)
{
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
        int id = ExpectId();
        int vd = Lookup(id);
        if (vd < 0 || VarDecls[vd].Type != (VT_LOCOFF|VT_CHAR|VT_PTR1)) {
            Fatal("Invalid va_list");
        }
        const int offset = VarDecls[vd].Offset;
        if (func == TOK_VA_START) {
            Expect(TOK_COMMA);
            id = ExpectId();
            vd = Lookup(id);
            if (vd < 0 || (VarDecls[vd].Type & VT_LOCMASK) != VT_LOCOFF) {
                Fatal("Invalid argument to va_start");
            }
            EmitLeaStackVar(R_AX, VarDecls[vd].Offset);
            EmitStoreAx(2, VT_LOCOFF, offset);
            CurrentType = VT_VOID;
        } else if (func == TOK_VA_END) {
            CurrentType = VT_VOID;
        } else if (func == TOK_VA_ARG) {
            Expect(TOK_COMMA);
            EmitLoadAx(2, VT_LOCOFF, offset);
            EmitAddRegConst(R_AX, 2);
            EmitStoreAx(2, VT_LOCOFF, offset);
            ParseDeclSpecs();
            CurrentType |= VT_LVAL;
        }
        Expect(TOK_RPAREN);
    } else {
        HandlePrimaryId(ExpectId());
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

    if (CurrentType & VT_LOCMASK) {
        Check(CurrentType == (VT_INT|VT_LOCLIT));
        CurrentType = VT_INT;
        EmitMovRImm(R_AX, CurrentVal);
    }
}

void HandleStructMember(void)
{
    const int MemId = ExpectId();
    int Off = 0;
    Check(CurrentStruct >= 0 && CurrentStruct < StructCount);
    struct StructMember* SM = StructDecls[CurrentStruct].Members;
    for (; SM && SM->Id != MemId; SM = SM->Next) {
        Off += SizeofType(SM->Type, SM->TypeExtra);
    }
    if (!SM) {
        Fatal("Invalid struct member");
    }
    int Loc = CurrentType & VT_LOCMASK;
    if (Loc == VT_LOCGLOB) {
        EmitLoadAddr(R_AX, Loc, CurrentVal);
        Loc = 0;
    }
    if (!Loc) {
        EmitAddRegConst(R_AX, Off);
    } else if (Loc == VT_LOCOFF) {
        CurrentVal += Off;
    } else {
        Check(0);
    }
    CurrentType   = SM->Type | VT_LVAL | Loc;
    CurrentStruct = SM->TypeExtra;
}

void EmitCallMemcpy(void)
{
    EmitCall(MemcpyDecl);
    EmitAdjSp(6);
    MemcpyUsed = 1;
}

void ParsePostfixExpression(void)
{
    for (;;) {
        if (Accept(TOK_LPAREN)) {
            // Function call
            if (!(CurrentType & VT_FUN)) {
                Fatal("Not a function");
            }
            Check((CurrentType & VT_LOCMASK) == VT_LOCGLOB);
            struct VarDecl* Func = &VarDecls[CurrentVal];
            const int RetType = CurrentType & ~(VT_FUN|VT_LOCMASK);
            int NumArgs = 0;
            int StackAdj = 0;
            while (TokenType != TOK_RPAREN) {
                enum { ArgsPerChunk = 4 };
                if (NumArgs % ArgsPerChunk == 0) {
                    StackAdj += ArgsPerChunk*2;
                    LocalOffset -= ArgsPerChunk*2;
                    EmitAdjSp(-ArgsPerChunk*2);
                    if (NumArgs) {
                        // Move arguments to new stack top
                        EmitMovRImm(R_AX, NumArgs*2);
                        EmitPush(R_AX);
                        EmitLeaStackVar(R_AX, LocalOffset + ArgsPerChunk*2);
                        EmitPush(R_AX);
                        EmitAddRegConst(R_AX, -ArgsPerChunk*2);
                        EmitPush(R_AX);
                        EmitCallMemcpy();
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

                if (!Accept(TOK_COMMA)) {
                    break;
                }
            }
            Expect(TOK_RPAREN);
            EmitCall(Func);
            if (StackAdj) {
                EmitAdjSp(StackAdj);
                LocalOffset += StackAdj;
            }
            CurrentType = RetType;
            if (CurrentType == VT_CHAR) {
                CurrentType = VT_INT;
            }
            CurrentStruct = Func->TypeExtra;
        } else if (Accept(TOK_LBRACKET)) {
            LvalToRval();
            if (!(CurrentType & VT_PTRMASK)) {
                Fatal("Expected pointer");
            }
            CurrentType -= VT_PTR1;
            const int Scale    = SizeofCurrentType();
            const int ResType  = CurrentType | VT_LVAL;
            const int ResExtra = CurrentStruct;
            SetPendingPushAx();
            ParseExpr();
            Expect(TOK_RBRACKET);
            if (CurrentType == (VT_INT|VT_LOCLIT)) {
                Check(PendingPushAx);
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
            CurrentType   = ResType;
            CurrentStruct = ResExtra;
            Check(!(CurrentType & VT_LOCMASK));
        } else if (Accept(TOK_DOT)) {
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
        if (Op == TOK_AND) {
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
        if (Op == TOK_STAR) {
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
        ParseDeclSpecs();
        CurrentVal = SizeofCurrentType();
        Expect(TOK_RPAREN);
        CurrentType = VT_LOCLIT | VT_INT;
    } else {
        ParsePrimaryExpression();
        ParsePostfixExpression();
    }
}

void ParseCastExpression(void)
{
    if (Accept(TOK_LPAREN)) {
        if (IsTypeStart()) {
            ParseDeclSpecs();
            Expect(TOK_RPAREN);
            const int T = CurrentType;
            const int S = CurrentStruct;
            Check(!(T & VT_LOCMASK));
            ParseCastExpression();
            LvalToRval();
            GetVal(); // TODO: could optimize some constant expressions here
            CurrentType = T;
            CurrentStruct = S;
            if (CurrentType == VT_CHAR) {
                OutputBytes(I_CBW, -1);
                CurrentType = VT_INT;
            }
        } else {
            ParseExpr();
            Expect(TOK_RPAREN);
        }
    } else {
        ParseUnaryExpression();
    }
}

int RelOpToCC(int Op)
{
    if (Op == TOK_LT)    return JL;
    if (Op == TOK_LTEQ)  return JNG;
    if (Op == TOK_GT)    return JG;
    if (Op == TOK_GTEQ)  return JNL;
    if (Op == TOK_EQEQ)  return JZ;
    if (Op == TOK_NOTEQ) return JNZ;
    Check(0);
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
    } else if (Op == TOK_PLUS)  {
Plus:
        if (CurrentVal == (char)CurrentVal) {
            EmitAddRegConst(R_AX, CurrentVal);
            return;
        }
        Op = I_ADD;
    } else if (Op == TOK_MINUS) {
        // Ease optimizations
        CurrentVal = -CurrentVal;
        goto Plus;
    } else if (Op == TOK_AND) {
        Op = I_AND;
    } else if (Op == TOK_XOR) {
        Op = I_XOR;
    } else if (Op == TOK_OR) {
        Op = I_OR;
    } else if (Op == TOK_STAR) {
        EmitScaleAx(CurrentVal);
        return;
    } else if (Op == TOK_SLASH) {
        EmitDivAxConst(CurrentVal);
        return;
    } else {
        EmitMovRImm(R_CX, CurrentVal);
        DoBinOp(Op);
        return;
    }
    OutputBytes(Op|5, -1);
    OutputWord(CurrentVal);
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
    int LhsPointeeSize;
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
        if (LhsType & VT_PTRMASK) {
            LhsPointeeSize = SizeofType(CurrentType-VT_PTR1, CurrentStruct);
        }

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
                Check(CurrentType == VT_INT || (CurrentType & VT_PTRMASK));
                EmitToBool();
                if (Op == TOK_ANDAND)
                    EmitJcc(JZ, LEnd);
                else
                    EmitJcc(JNZ, LEnd);
            }
        } else {
            LEnd = -1;
            if (!(LhsType & VT_LOCMASK) && Op != TOK_COMMA) {
                SetPendingPushAx();
            }
        }

        // TODO: Question, ?:
        ParseCastExpression(); // RHS
        for (;;) {
            const int LookAheadOp         = TokenType;
            const int LookAheadPrecedence = OperatorPrecedence(LookAheadOp);
            if (LookAheadPrecedence > Prec || (LookAheadPrecedence == Prec && LookAheadPrecedence != PRED_EQ)) // LookAheadOp != PRED_EQ => !IsRightAssociative
                break;
            ParseExpr1(LookAheadPrecedence);
        }

        LhsLoc = LhsType & VT_LOCMASK;
        LhsType &= ~VT_LOCMASK;

        if (Op == TOK_COMMA) {
            continue;
        } else if (Op == TOK_EQ && LhsType == VT_STRUCT) {
            // Struct assignment
            // TODO: Verify CurrentStruct matches Lhs struct
            Check((CurrentType&~VT_LOCMASK) == (VT_STRUCT|VT_LVAL));
            EmitMovRImm(R_CX, SizeofCurrentType());
            EmitPush(R_CX);
            EmitLoadAddr(R_CX, CurrentType&VT_LOCMASK, CurrentVal);
            EmitPush(R_CX);
            EmitLoadAddr(R_CX, LhsLoc, LhsVal);
            EmitPush(R_CX);
            EmitCallMemcpy();
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
            if (Op != TOK_EQ) {
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
            if (CurrentType == (VT_LOCLIT|VT_INT)) {
                Check(PendingPushAx);
                PendingPushAx = 0;
                CurrentType = VT_INT;
                if (LhsType & VT_PTRMASK) {
                    CurrentVal *= LhsPointeeSize;
                }
                DoRhsConstBinOp(Op);
            } else {
                GetVal();
                Check(CurrentType == VT_INT || (CurrentType & VT_PTRMASK));
                if (Op == TOK_PLUS && (LhsType & VT_PTRMASK)) {
                    EmitScaleAx(LhsPointeeSize);
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
            if (Op == TOK_MINUS && (LhsType & VT_PTRMASK)) {
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

struct VarDecl* AddVarDecl(int Id)
{
    Check(ScopesCount);
    Check(Scopes[ScopesCount-1] < VARDECL_MAX-1);
    struct VarDecl* vd = &VarDecls[++Scopes[ScopesCount-1]];
    vd->Type      = CurrentType;
    vd->TypeExtra = CurrentStruct;
    vd->Id        = Id;
    vd->Offset    = 0;
    vd->Ref       = 0;
    return vd;
}

void ParseDeclSpecs(void)
{
    Check(IsTypeStart());
    if (TokenType == TOK_CONST) {
        // Ignore
        GetToken();
        Check(IsTypeStart());
    }
    int t = VT_INT;
    if (Accept(TOK_ENUM)) {
        if (TokenType >= TOK_ID) {
            // TODO: Store and use the enum identifier
            GetToken();
        }
        if (Accept(TOK_LBRACE)) {
            int EnumVal = 0;
            while (TokenType != TOK_RBRACE) {
                const int id = ExpectId();
                if (Accept(TOK_EQ)) {
                    ParseAssignmentExpression();
                    Check(CurrentType == (VT_INT|VT_LOCLIT));
                    EnumVal = CurrentVal;
                }
                CurrentType = VT_INT|VT_LOCLIT;
                struct VarDecl* vd = AddVarDecl(id);
                vd->Offset = EnumVal;
                if (!Accept(TOK_COMMA)) {
                    break;
                }
                ++EnumVal;
            }
            Expect(TOK_RBRACE);
        }
    } else if (TokenType == TOK_STRUCT) {
        t = VT_STRUCT;
        GetToken();
        const int id = ExpectId();
        CurrentStruct = -1;
        int i;
        for (i = 0; i < StructCount; ++i) {
            if (StructDecls[i].Id == id) {
                CurrentStruct = i;
                break;
            }
        }
        if (CurrentStruct < 0) {
            Check(StructCount < STRUCT_MAX);
            const int SI = StructCount++;
            struct StructDecl* SD = &StructDecls[SI];
            SD->Id = id;
            struct StructMember** Last = &SD->Members;
            Expect(TOK_LBRACE);
            while (!Accept(TOK_RBRACE)) {
                Check(StructMemCount < STRUCT_MEMBER_MAX);
                struct StructMember* SM = &StructMembers[StructMemCount++];
                ParseDeclSpecs();
                SM->Type      = CurrentType;
                SM->TypeExtra = CurrentStruct;
                SM->Id        = ExpectId();
                *Last = SM;
                Last = &SM->Next;
                Expect(TOK_SEMICOLON);
            }
            *Last = 0;
            CurrentStruct = SI;
        }
    } else {
        if (TokenType == TOK_VOID) {
            t = VT_VOID;
        } else if (TokenType == TOK_CHAR) {
            t = VT_CHAR;
        } else if (TokenType == TOK_INT) {
            t = VT_INT;
        } else if (TokenType == TOK_VA_LIST) {
            t = VT_CHAR | VT_PTR1;
        } else {
            Unexpected();
        }
        GetToken();
    }
    while (TokenType == TOK_STAR) {
        t += VT_PTR1;
        GetToken();
    }
    CurrentType = t;
}

void DoCond(int Label, int Forward) // forward => jump if label is false
{
    ParseExpr();
    LvalToRval();
    if (CurrentType == VT_BOOL) {
        EmitJcc(CurrentVal^Forward, Label);
    } else if (CurrentType == (VT_LOCLIT|VT_INT)) {
        // Constant condition
        if (CurrentVal != Forward)
            EmitJmp(Label);
    } else {
        GetVal();
        Check(CurrentType == VT_INT || (CurrentType&VT_PTRMASK));
        EmitToBool();
        EmitJcc(JNZ^Forward, Label);
    }
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

void ParseStatement(void)
{
Redo:

    // Get compund statements out of the way to simplify switch handling
    if (TokenType == TOK_LBRACE) {
        ParseCompoundStatement();
        return;
    }

    if (NextSwitchCase >= 0) {
        // switch is active

        if ((TokenType == TOK_CASE || TokenType == TOK_DEFAULT)
            && NextSwitchStmt < 0) {
            // Continue execution after the case
            NextSwitchStmt = MakeLabel();
            EmitJmp(NextSwitchStmt);
        }

        if (Accept(TOK_CASE)) {
            EmitLocalLabel(NextSwitchCase);
            NextSwitchCase = MakeLabel();
            ParseExpr();
            Expect(TOK_COLON);
            Check(CurrentType == (VT_INT|VT_LOCLIT)); // Need constant expression
            OutputBytes(I_ALU_RM16_IMM16, MODRM_REG|I_CMP|R_SI, -1);
            OutputWord(CurrentVal);
            EmitJcc(JZ, NextSwitchStmt);
            goto Redo;
        } else if (Accept(TOK_DEFAULT)) {
            Expect(TOK_COLON);
            Check(SwitchDefault == -1);
            SwitchDefault = NextSwitchStmt;
            goto Redo;
        } else if (NextSwitchStmt >= 0) {
            EmitJmp(NextSwitchCase);
            EmitLocalLabel(NextSwitchStmt);
            NextSwitchStmt = -1;
            goto Redo;
        }
    }

    if (Accept(TOK_SEMICOLON)) {
    } else if (IsTypeStart()) {
        ParseDeclSpecs();
        if (TokenType != TOK_SEMICOLON) {
            struct VarDecl* vd = AddVarDecl(ExpectId());
            vd->Type |= VT_LOCOFF;
            int size = 2;
            if (CurrentType == VT_STRUCT) {
                size = SizeofCurrentType();
            } else if (Accept(TOK_EQ)) {
                ParseAssignmentExpression();
                LvalToRval();
                GetVal();
                if (CurrentType == VT_CHAR) {
                    OutputBytes(I_CBW, -1);
                }
            }
            LocalOffset -= size;
            vd->Offset = LocalOffset;
            if (size == 2) {
                // Note: AX contains "random" garbage at first if the variable isn't initialized
                EmitPush(R_AX);
            } else {
                EmitAdjSp(-size);
            }
        }
        Expect(TOK_SEMICOLON);
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
    } else if (Accept(TOK_BREAK)) {
        EmitAdjSp(BStackLevel - LocalOffset);
        EmitJmp(BreakLabel);
        Expect(TOK_SEMICOLON);
    } else if (Accept(TOK_CONTINUE)) {
        EmitAdjSp(CStackLevel - LocalOffset);
        EmitJmp(ContinueLabel);
        Expect(TOK_SEMICOLON);
    } else if (Accept(TOK_GOTO)) {
        EmitJmp(GetNamedLabel(ExpectId())->LabelId);
    } else if (Accept(TOK__EMIT)) {
        ParseExpr();
        Check(CurrentType == (VT_INT|VT_LOCLIT)); // Constant expression expected
        OutputBytes(CurrentVal & 0xff, -1);
    } else if (Accept(TOK_IF)) {
        const int ElseLabel = MakeLabel();

        Accept(TOK_LPAREN);
        DoCond(ElseLabel, 1);
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
            DoCond(EndLabel, 1);
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
        DoLoopStatements(EndLabel, IterLabel);
        EmitJmp(IterLabel);
        EmitLocalLabel(EndLabel);
    } else if (Accept(TOK_WHILE)) {
        const int StartLabel = MakeLabel();
        const int EndLabel   = MakeLabel();
        EmitLocalLabel(StartLabel);
        Expect(TOK_LPAREN);
        DoCond(EndLabel, 1);
        Expect(TOK_RPAREN);
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
        Expect(TOK_LPAREN);
        DoCond(StartLabel, 0);
        Expect(TOK_RPAREN);
        Expect(TOK_SEMICOLON);
        EmitLocalLabel(EndLabel);
    } else if (Accept(TOK_SWITCH)) {
        Expect(TOK_LPAREN);
        ParseExpr();
        LvalToRval();
        GetVal();
        Expect(TOK_RPAREN);

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
            if (Accept(TOK_COLON)) {
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
        Expect(TOK_SEMICOLON);
    }
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

void ParseExternalDefition(void)
{
    ParseDeclSpecs();

    if (Accept(TOK_SEMICOLON)) {
        return;
    }

    const int Id = ExpectId();

    CurrentType |= VT_LOCGLOB;

    struct VarDecl* vd;
    const int VarIndex = Lookup(Id);
    if (VarIndex < 0) {
        vd = AddVarDecl(Id);
    } else {
        vd = &VarDecls[VarIndex];
        Check(vd->Offset == 0);
        Check((vd->Type & VT_LOCMASK) == VT_LOCGLOB);
    }

    // Variable?
    if (Accept(TOK_EQ)) {
        ParseAssignmentExpression();
        Expect(TOK_SEMICOLON);
        Check(CurrentType == (VT_INT|VT_LOCLIT)); // Expecct constant expressions
        // TODO: Could save one byte per global char...
        //       Handle this if/when implementing complex initializers
        EmitGlobalLabel(vd);
        OutputWord(CurrentVal);
        return;
    } else if (Accept(TOK_SEMICOLON)) {
        BssSize += SizeofCurrentType();
        return;
    }

    Expect(TOK_LPAREN);
    vd->Type |= VT_FUN;
    PushScope();
    int ArgOffset;
    ArgOffset = 4;
    while (TokenType != TOK_RPAREN) {
        if (Accept(TOK_ELLIPSIS))
            break;
        ParseDeclSpecs();
        if (CurrentType == VT_VOID)
            break;
        struct VarDecl* arg = AddVarDecl(ExpectId());
        arg->Type |= VT_LOCOFF;
        arg->Offset = ArgOffset;
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
        ResetLabels();
    }
    PopScope();
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
    if (!LastDot) LastDot = dest;
    CopyStr(LastDot, ".com");
}

int AddId(const char* s)
{
    const int Id = IdCount++;
    int Hash = HASHINIT;
    char ch;
    IdOffset[Id] = IdBufferIndex;
    while ((ch = *s++) > ' ') {
        IdBuffer[IdBufferIndex++] = ch;
        Hash = Hash*HASHMUL+ch;
    }
    IdBuffer[IdBufferIndex++] = 0;
    AddIdHash(Id, Hash);
    return Id;
}

void AddBuiltins(const char* s)
{
    do {
        AddId(s);
        while (*s > ' ')
            ++s;
    } while(*s++);
}

#if 0
int DosCall(int* ax, int bx, int cx, int dx)
{
    _emit I_MOV_RM_R|1          _emit MODRM_BP_DISP8|R_BX<<3 _emit 4 // 8B5E04            MOV BX,[BP+0x4]
    _emit I_MOV_RM_R|1          _emit MODRM_BX                       // 8B07              MOV AX,[BX]
    _emit I_MOV_RM_R|1          _emit MODRM_BP_DISP8|R_BX<<3 _emit 6 // 8B5E06            MOV BX,[BP+0x6]
    _emit I_MOV_RM_R|1          _emit MODRM_BP_DISP8|R_CX<<3 _emit 8 // 8B4E08            MOV CX,[BP+0x8]
    _emit I_MOV_RM_R|1          _emit MODRM_BP_DISP8|R_DX<<3 _emit 10// 8B560A            MOV DX,[BP+0xA]
    _emit I_INT                 _emit 0x21                           // CD21              INT 0x21
    _emit I_MOV_RM_R|1          _emit MODRM_BP_DISP8|R_BX<<3 _emit 4 // 8B5E04            MOV BX,[BP+0x4]
    _emit I_MOV_R_RM|1          _emit MODRM_BX                       // 8907              MOV [BX],AX
    _emit I_MOV_R_IMM16|R_AX    _emit 0x00                   _emit 0 // B80000            MOV AX,0x0
    _emit I_SBB|1               _emit MODRM_REG                      // 19C0              SBB AX,AX
}

void memcpy(char* dst, const char* src, int size)
{
    _emit I_PUSH|R_SI                                       // PUSH SI
    _emit I_PUSH|R_DI                                       // PUSH DI
    _emit I_MOV_RM_R|1 _emit MODRM_BP_DISP8|R_DI<<3 _emit 4 // MOV DI,[BP+0x4]
    _emit I_MOV_RM_R|1 _emit MODRM_BP_DISP8|R_SI<<3 _emit 6 // MOV SI,[BP+0x6]
    _emit I_MOV_RM_R|1 _emit MODRM_BP_DISP8|R_CX<<3 _emit 8 // MOV CX,[BP+0x8]
    _emit 0xF3         _emit 0xA4                           // REP MOVSB
    _emit I_POP|R_DI                                        // POP DI
    _emit I_POP|R_SI                                        // POP SI
}
#endif

int main(int argc, char** argv)
{
    int i;

    Output           = malloc(OUTPUT_MAX);
    LineBuf          = malloc(LINEBUF_MAX);
    TempBuf          = malloc(TMPBUF_MAX);
    InBuf            = malloc(INBUF_MAX);
    IdBuffer         = malloc(IDBUFFER_MAX);
    IdOffset         = malloc(sizeof(int)*ID_MAX);
    IdHashTab        = malloc(sizeof(int)*ID_HASHMAX);
    VarDecls         = malloc(sizeof(struct VarDecl)*VARDECL_MAX);
    Scopes           = malloc(sizeof(int)*SCOPE_MAX);
    Labels           = malloc(sizeof(struct Label)*LABEL_MAX);
    NamedLabels      = malloc(sizeof(struct NamedLabel)*NAMED_LABEL_MAX);
    StructMembers    = malloc(sizeof(struct StructMember)*STRUCT_MEMBER_MAX);
    StructDecls      = malloc(sizeof(struct StructDecl)*STRUCT_MAX);

    if (argc < 2) {
        Printf("Usage: %s input-file [output-file]\n", argv[0]);
        return 1;
    }

    InFile = open(argv[1], 0); // O_RDONLY
    if (InFile < 0) {
        Fatal("Error opening input file");
    }

    for (i = 0; i < ID_HASHMAX; ++i)
        IdHashTab[i] = -1;

    AddBuiltins("break case char const continue default do else enum for goto if"
        " int return sizeof struct switch void while"
        " va_list va_start va_end va_arg _emit");
    Check(IdCount+TOK_BREAK-1 == TOK__EMIT);

    PushScope();

    // Prelude
    OutputBytes(I_XOR|1, MODRM_REG|R_BP<<3|R_BP, -1);
    CurrentType = VT_FUN|VT_VOID|VT_LOCGLOB;
    OutputBytes(I_JMP_REL16, -1);
    EmitGlobalRefRel(AddVarDecl(AddId("_start")));

    CurrentType = VT_CHAR|VT_LOCGLOB;
    struct VarDecl* SBSS = AddVarDecl(AddId("_SBSS"));
    struct VarDecl* EBSS = AddVarDecl(AddId("_EBSS"));

    CurrentType = VT_FUN|VT_VOID|VT_LOCGLOB;
    MemcpyDecl = AddVarDecl(AddId("memcpy"));

    NextChar();
    GetToken();
    while (TokenType != TOK_EOF) {
        ParseExternalDefition();
    }
    close(InFile);

    EmitGlobalLabel(SBSS);
    for (i = 0 ; i <= Scopes[0]; ++i) {
        struct VarDecl* vd = &VarDecls[i];
        CurrentType = vd->Type;
        CurrentStruct = vd->TypeExtra;
        const int Loc = CurrentType & VT_LOCMASK;
        if (Loc == VT_LOCGLOB && !vd->Offset) {
            if (vd == EBSS || (vd == MemcpyDecl && !MemcpyUsed))
                continue;
            CurrentType &= ~VT_LOCMASK;
            if (CurrentType & VT_FUN) {
                Printf("%s is undefined\n", IdText(vd->Id));
                Fatal("Undefined function");
            }
            EmitGlobalLabel(vd);
            CodeAddress += SizeofCurrentType();
        }
        //if (Loc == VT_LOCGLOB && (CurrentType&VT_FUN)) Printf("%X %s\n", vd->Offset, IdText(vd->Id));
    }
    Check(CodeAddress - SBSS->Offset == BssSize);
    EmitGlobalLabel(EBSS);

    PopScope();

    if (argv[2])
       CopyStr(IdBuffer, argv[2]);
    else
        MakeOutputFilename(IdBuffer, argv[1]);
    const int OutFile = open(IdBuffer, CREATE_FLAGS, 0600);
    if (OutFile < 0) {
        Fatal("Error creating output file");
    }
    write(OutFile, Output, SBSS->Offset - CODESTART);
    close(OutFile);

    return 0;
}
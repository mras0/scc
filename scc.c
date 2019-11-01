#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <assert.h>

#define TOKEN_MAX 64

#define VT_VOID     0
#define VT_CHAR     1
#define VT_INT      2
#define VT_BASEMASK 0xff
#define VT_LVAL     0x100
#define VT_PTR      0x200
#define VT_FUN      0x400

#define BUILTIN_TOKS(X)   \
X(TOK_VOID   , "void")    \
X(TOK_CHAR   , "char")    \
X(TOK_INT    , "int")     \
X(TOK_RETURN , "return")  \
X(TOK_WHILE  , "while")   \


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
    TOK_PLUS,
    TOK_MINUS,
    TOK_STAR,
    TOK_SLASH,
    TOK_MOD,

    TOK_LAST = TOK_MOD,
#define DEF_BULTIN(N, _) N,
    BUILTIN_TOKS(DEF_BULTIN)
#undef DEF_BUILTIN
    TOK_ID
};

struct VarDecl {
    int             Id;
    int             Type;
    int             Offset;
    struct VarDecl* Next;
};

struct Scope {
    struct VarDecl* Decls;
    struct Scope*   Next;
};

static const char* InBuf;
static char TokenText[TOKEN_MAX];
static int TokenType;
static uintptr_t TokenNumVal;
static char** Ids;
static int IdsCount, IdsCapacity;
static struct Scope* CurrentScope;
static int LocalOffset;
static int CurrentType;
static int LocalLabelCounter;

static int IsDigit(char ch) { return ch >= '0' && ch <= '9'; }
static int IsAlpha(char ch) { return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z'); }

static __declspec(noreturn) void Fatal(const char* Msg)
{
    fputs(Msg, stderr);
    abort();
}

static void* Malloc(size_t Bytes)
{
    void* Buf = malloc(Bytes);
    if (!Buf) {
        Fatal("malloc failed");
    }
    return Buf;
}

static char* DupString(const char* Name)
{
    const size_t cnt = strlen(Name) + 1;
    char* Buf = Malloc(cnt);
    memcpy(Buf, Name, cnt);
    return Buf;
}

static int AddId(const char* Name)
{
    if (IdsCount == IdsCapacity) {
        IdsCapacity = IdsCapacity ? IdsCapacity*2 : 32;
        char** NewIds = realloc(Ids, sizeof(char*)*IdsCapacity);
        if (!NewIds) {
            Fatal("realloc failed");
        }
        Ids = NewIds;
    }
    Ids[IdsCount++] = DupString(Name);
    return IdsCount - 1;
}

static char GetChar(void)
{
    return *InBuf ? *InBuf++ : 0;
}

static void UnGetChar(void)
{
    --InBuf;
}

static char* TokenTypeString(int type)
{
    switch (type) {
    case TOK_EOF:       return "eof";
    case TOK_NUM:       return "number";
    case TOK_STRLIT:    return "strlit";
    case TOK_LPAREN:    return "("; 
    case TOK_RPAREN:    return ")";
    case TOK_LBRACE:    return "{";
    case TOK_RBRACE:    return "}";
    case TOK_LBRACKET:  return "[";
    case TOK_RBRACKET:  return "]";
    case TOK_COMMA:     return ",";
    case TOK_SEMICOLON: return ";";
    case TOK_EQ:        return "=";
    case TOK_PLUS:      return "+";
    case TOK_MINUS:     return "-";
    case TOK_STAR:      return "*";
    case TOK_SLASH:     return "/";
    case TOK_MOD:       return "%";
#define CASE_TOKEN(V, N) case V: return N;
    BUILTIN_TOKS(CASE_TOKEN)
#undef CASE_TOKEN
    }
    static char buf[64];
    snprintf(buf, sizeof(buf), "TOK(%d)", type);
    return buf;
}

static char* TypeString(int Type)
{
    switch (Type & VT_BASEMASK) {
    case VT_VOID: return Type & VT_PTR ? "void*" : "void";
    case VT_CHAR: return Type & VT_PTR ? "char*" : "char";
    case VT_INT:  return Type & VT_PTR ? "int*"  : "int";
    default:
        printf("Unknown type 0x%X\n", Type);
        Fatal("Invalid type");
    }
}

static void PrintToken(void)
{
    if (TokenType == TOK_NUM) {
        printf("%"PRIuMAX, TokenNumVal);
    } else if (TokenType > TOK_LAST) {
        printf("%s", Ids[TokenType-TOK_LAST-1]);
    } else {
        printf("%s", TokenText);
    }
}

static void GetToken(void)
{
    char ch;
    int TokenLen = 0;

    ch = GetChar();
    while (ch <= ' ') {
        if (!ch) {
            TokenType = TOK_EOF;
            goto Out;
        }
        ch = GetChar();
    }
    TokenText[TokenLen++] = ch;

    if (ch >= '0' && ch <= '9') {
        TokenNumVal = 0;
        for (;;) {
            TokenNumVal = TokenNumVal*10 + ch - '0';
            ch = GetChar();
            if (!IsDigit(ch)) {
                break;
            }
            TokenText[TokenLen++] = ch;
        }
        UnGetChar();
        TokenType = TOK_NUM;
    } else if (IsAlpha(ch) || ch == '_') {
        for (;;) {
            ch = GetChar();
            if (ch != '_' && !IsDigit(ch) && !IsAlpha(ch)) {
                break;
            }
            TokenText[TokenLen++] = ch;
        }
        TokenText[TokenLen] = '\0';
        UnGetChar();
        int id;
        for (id = 0; id < IdsCount; ++id) {
            if (!strcmp(Ids[id], TokenText)) {
                break;
            }
        }
        TokenType = TOK_LAST + 1 + (id < IdsCount ? id : AddId(TokenText));
    } else if (ch == '"') {
        --TokenLen;
        while ((ch = GetChar()) != '"') {
            if (!ch) Fatal("Unterminated string literal");
            assert(TokenLen < TOKEN_MAX-1);
            TokenText[TokenLen++] = ch;
        }
        TokenType = TOK_STRLIT;
    } else {
        switch (ch) {
        case '(': TokenType = TOK_LPAREN;    break;
        case ')': TokenType = TOK_RPAREN;    break;
        case '{': TokenType = TOK_LBRACE;    break;
        case '}': TokenType = TOK_RBRACE;    break;
        case '[': TokenType = TOK_LBRACKET;  break;
        case ']': TokenType = TOK_RBRACKET;  break;
        case ',': TokenType = TOK_COMMA;     break;
        case ';': TokenType = TOK_SEMICOLON; break;
        case '=': TokenType = TOK_EQ;        break;
        case '+': TokenType = TOK_PLUS;      break;
        case '-': TokenType = TOK_MINUS;     break;
        case '*': TokenType = TOK_STAR;      break;
        case '/': TokenType = TOK_SLASH;     break;
        case '%': TokenType = TOK_MOD;       break;
        default:
            printf("Unknown token start '%c' (0x%02X)\n", ch, ch);
            TokenType = TOK_EOF;
        }
    }
Out:
    TokenText[TokenLen] = '\0';

#if 0
    printf("%s \"%s\" ", TokenTypeString(TokenType), TokenText);
    PrintToken();
    puts("");
#endif
}

int OperatorPrecedence(int tok)
{
    switch (tok) {
    case TOK_STAR:
    case TOK_SLASH:
    case TOK_MOD:
        return 5;
    case TOK_PLUS:
    case TOK_MINUS:
        return 6;
    case TOK_EQ:
        return 16;
    case TOK_COMMA:
        return 17;
    default:
        return 100;
    }
}

void __declspec(noreturn) Unexpected(void)
{
    PrintToken();
    printf(" (%s)\n", TokenTypeString(TokenType));
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
        printf("%s expected got ", TokenTypeString(type));
        Unexpected();
    }
}

int ExpectId(void)
{
    const int id = TokenType;
    if (id >= TOK_ID) {
        GetToken();
        return id - TOK_LAST - 1;
    }
    printf("Expected identifier got ");
    Unexpected();
}

int IsTypeStart(void)
{
    return TokenType == TOK_VOID || TokenType == TOK_CHAR || TokenType == TOK_INT;
}

struct VarDecl* Lookup(int id)
{
    for (struct Scope* s = CurrentScope; s; s = s->Next) {
        for (struct VarDecl* d = s->Decls; d; d = d->Next) {
            if (d->Id == id) {
                return d;
            }
        }
    }
    printf("%s not found\n", Ids[id]);
    Fatal("Lookup failed");
}

static void LvalToRval(void)
{
    if (CurrentType & VT_LVAL) {
        CurrentType &= ~VT_LVAL;
        printf("\tMOV\tBX, AX\n");
        if (CurrentType == VT_CHAR) {
            printf("\tMOV\tAL, [BX]\n");
            printf("\tCBW\n");
            CurrentType = VT_INT;
        } else {
            assert(CurrentType == VT_INT || (CurrentType & VT_PTR));
            printf("\tMOV\tAX, [BX]\n");
        }
    }
}

void ParseExpression(void);
void ParseAssignmentExpression(void);

void ParsePrimaryExpression(void)
{
    if (TokenType == TOK_NUM) {
        printf("\tMOV\tAX, 0x%04X\n", (unsigned short)TokenNumVal);
        CurrentType = VT_INT;
        GetToken();
    } else if (TokenType == TOK_STRLIT) {
        const int SL = LocalLabelCounter++;
        const int JL = LocalLabelCounter++;
        printf("\tJMP\t.L%d\n", JL);
        printf(".L%d:\tDB '%s', 0\n", SL, TokenText);
        printf(".L%d:\n", JL);
        printf("\tMOV\tAX, .L%d\n", SL);
        CurrentType = VT_PTR | VT_CHAR;
        GetToken();
    } else {
        const int id = ExpectId();
        const struct VarDecl* d = Lookup(id);
        CurrentType = d->Type | VT_LVAL;
        if (d->Offset) {
            printf("\tLEA\tAX, [BP%+d]\t; %s\n", d->Offset, Ids[d->Id]);
        } else {
            printf("\tMOV\tAX, _%s\n", Ids[d->Id]);
            if (CurrentType & VT_FUN) {
                CurrentType &= ~VT_LVAL;
            }
        }
    }
}

void ParseUnaryExpression(void)
{
    ParsePrimaryExpression();
    // Post-fix expression
    for (;;) {
        if (Accept(TOK_LPAREN)) {
            // Function call
            if (!(CurrentType & VT_FUN)) {
                Fatal("Not a function");
            }
            printf("\tPUSH\tAX\n");
            const int RetType = CurrentType & ~VT_FUN;
            int NumArgs = 0;
            while (TokenType != TOK_RPAREN) {
                ++NumArgs;
                ParseAssignmentExpression();
                // TODO: Check if we need to reorder stack...
                //       Possible work-around: Reserve fixed amount of space for arguments
                LvalToRval();
                assert(CurrentType == VT_INT || (CurrentType & VT_PTR));
                printf("\tPUSH\tAX\n");

                if (!Accept(TOK_COMMA)) {
                    break;
                }
            }
            assert(NumArgs <= 1); // TODO: Swap args to match expected stack layoutu
            Expect(TOK_RPAREN);
            printf("\tMOV\tBX, SP\n");
            printf("\tMOV\tBX, [BX%+d]\n", NumArgs*2);
            printf("\tCALL\tBX\n");
            printf("\tADD\tSP, %d\n", NumArgs*2+2); // +2 for the function pointer
            CurrentType = RetType;
        } else if (Accept(TOK_LBRACKET)) {
            LvalToRval();
            if (!(CurrentType & VT_PTR)) {
                Fatal("Expected pointer");
            }
            const int AType = CurrentType & ~VT_PTR;
            printf("\tPUSH\tAX\n");
            ParseExpression();
            Expect(TOK_RBRACKET);
            LvalToRval();
            assert(CurrentType == VT_INT);
            if (AType != VT_CHAR) {
                assert(AType == VT_INT);
                printf("\tADD\tAX, AX\n");
            }
            printf("\tPOP\tCX\n");
            printf("\tADD\tAX, CX\n");
            CurrentType = AType | VT_LVAL;
        } else {
            break;
        }
    }
}

void ParseCastExpression(void)
{
    ParseUnaryExpression();
}

void DoBinOp(int Op)
{
    switch (Op) {
    case TOK_PLUS:  printf("\tADD\tAX, CX\n"); break;
    case TOK_MINUS: printf("\tSUB\tAX, CX\n"); break;
    case TOK_STAR:  printf("\tMUL\tCX\n"); break;
    case TOK_SLASH:
    case TOK_MOD:
        printf("\tXOR\tDX, DX\n");
        printf("\tIDIV\tCX\n");
        if (Op == TOK_MOD) printf("\tMOV\tAX, DX\n");
        break;
    default:
        printf("TODO: BinOp %s\n", TokenTypeString(Op));
        Fatal("Not implemented");
    }
}

void ParseExpression1(int OuterPrecedence)
{
    for (;;) {
        const int Op   = TokenType;
        const int Prec = OperatorPrecedence(Op);
        if (Prec > OuterPrecedence) {
            break;
        }
        GetToken();
        printf("\tPUSH\tAX\n");
        int LhsType = CurrentType;
        // TODO: Question, ?:
        /*rhs = */ ParseCastExpression();
        for (;;) {
            const int LookAheadOp         = TokenType;
            const int LookAheadPrecedence = OperatorPrecedence(LookAheadOp);
            if (LookAheadPrecedence > Prec /* || (LookAheadPrecedence == Prec && !IsRightAssociative(LookAheadOp)) */) {
                break;
            }
            ParseExpression1(/*rhs, */LookAheadPrecedence);
        }
        const int IsAssign = Op == TOK_EQ;
        LvalToRval();
        if (LhsType & VT_LVAL) {
            LhsType &= ~VT_LVAL;
            printf("\tPOP\tBX\n");
            if (IsAssign) {
                if (LhsType == VT_CHAR) {
                    assert(CurrentType == VT_INT);
                    printf("\tMOV\t[BX], AL\n");
                } else {
                    assert(LhsType == VT_INT || (LhsType & VT_PTR));
                    assert(CurrentType == VT_INT || (CurrentType & VT_PTR));
                    printf("\tMOV\t[BX], AX\n");
                }
            } else {
                assert(LhsType == VT_INT);
                assert(CurrentType == VT_INT);
                printf("\tMOV\tCX, AX\n");
                printf("\tMOV\tAX, [BX]\n");
                DoBinOp(Op);
            }
        } else {
            assert(!IsAssign && "Lvalue expected");
            assert(LhsType == VT_INT);
            assert(CurrentType == VT_INT);
            printf("\tPOP\tCX\n");
            printf("\tXCHG\tAX, CX\n");
            DoBinOp(Op);
        }
    }
}

void ParseExpression0(int OuterPrecedence)
{
    ParseCastExpression();
    ParseExpression1(OuterPrecedence);
}

void ParseExpression(void)
{
    ParseExpression0(OperatorPrecedence(TOK_COMMA));
}

void ParseAssignmentExpression(void)
{
    ParseExpression0(OperatorPrecedence(TOK_EQ));
}

int ParseDeclSpecs(void)
{
    assert(IsTypeStart());
    int t;
    switch (TokenType) {
    case TOK_VOID: t = VT_VOID; break;
    case TOK_CHAR: t = VT_CHAR; break;
    case TOK_INT:  t = VT_INT;  break;
    default: Unexpected();
    }
    GetToken();
    if (TokenType == TOK_STAR) {
        t |= VT_PTR;
        GetToken();
    }
    return t;
}

static struct VarDecl* AddVarDecl(int Type, int Id)
{
    assert(CurrentScope);
    struct VarDecl* d = Malloc(sizeof(*d));
    d->Next   = CurrentScope->Decls;
    d->Type   = Type;
    d->Offset = 0;
    d->Id     = Id;
    CurrentScope->Decls = d;
    return d;
}

struct VarDecl* ParseDecl(void)
{
    const int Type = ParseDeclSpecs();
    const int Id   = ExpectId();
    return AddVarDecl(Type, Id);
}

void FreeVarDecls(struct VarDecl* d)
{
    struct VarDecl* temp;
    while (d) {
        temp = d->Next;
        free(d);
        d = temp;
    }
}

static void PushScope(void)
{
    struct Scope* s = Malloc(sizeof(*s));
    s->Decls = NULL;
    s->Next  = CurrentScope;
    CurrentScope = s;
}

static void PopScope(void)
{
    assert(CurrentScope);
    struct Scope* s = CurrentScope;
    CurrentScope = CurrentScope->Next;
    FreeVarDecls(s->Decls);
    free(s);
}

void ParseCompoundStatement(void);

void ParseStatement(void)
{
    if (TokenType == TOK_LBRACE) {
        ParseCompoundStatement();
    } else if (IsTypeStart()) {
        struct VarDecl* d = ParseDecl();
        LocalOffset -= 2;
        d->Offset = LocalOffset;
        printf("\tSUB\tSP, 2\t; [BP%+d] %s %s\n", d->Offset, TypeString(d->Type), Ids[d->Id]);
        Expect(TOK_SEMICOLON);
    } else if (Accept(TOK_RETURN)) {
        if (TokenType != TOK_SEMICOLON) {
            ParseExpression();
        }
        printf("\tJMP\t.RET\n");
    } else if (Accept(TOK_WHILE)) {
        Expect(TOK_LPAREN);
        const int StartLabel = LocalLabelCounter++;
        const int BodyLabel  = LocalLabelCounter++;
        const int EndLabel   = LocalLabelCounter++;
        printf(".L%d:\t; Start of while\n", StartLabel);
        ParseExpression();

        LvalToRval();
        assert(CurrentType == VT_INT);
        printf("\tAND\tAX, AX\n");
        printf("\tJNZ\t.L%d\n", BodyLabel);
        printf("\tJMP\t.L%d\n", EndLabel);
        Expect(TOK_RPAREN);
        printf(".L%d:\t; Body of while\n", BodyLabel);
        ParseStatement();
        printf("\tJMP\t.L%d\n", StartLabel);
        printf(".L%d:\t; End of while\n", EndLabel);
    } else {
        // Expression statement
        ParseExpression();
        Expect(TOK_SEMICOLON);
    }
}

void ParseCompoundStatement(void)
{
    PushScope();
    Expect(TOK_LBRACE);
    while (!Accept(TOK_RBRACE)) {
        ParseStatement();
    }
    PopScope();
}

// Arg 2              BP + 6
// Arg 1              BP + 4
// Return Address     BP + 2
// Old BP             BP      <-- BP
// Local 1            BP - 2
// Local 2            BP - 4  <-- SP

void ParseExternalDefition(void)
{
    struct VarDecl* fd = ParseDecl();
    int ArgOffset = 4;
    printf("\t; Starting function definition of %s, return value %s\n", Ids[fd->Id], TypeString(fd->Type));
    printf("_%s:\n", Ids[fd->Id]);
    printf("\tPUSH\tBP\n");
    printf("\tMOV\tBP, SP\n");
    fd->Type |= VT_FUN;
    LocalOffset = 0;
    LocalLabelCounter = 0;
    Expect(TOK_LPAREN);
    while (TokenType != TOK_RPAREN) {
        if (Accept(TOK_VOID)) {
            break;
        }
        struct VarDecl* d = ParseDecl();
        d->Offset = ArgOffset;
        ArgOffset += 2;
        printf("\t; Arg %s %s [BP+%d]\n", TypeString(d->Type), Ids[d->Id], d->Offset);
        if (!Accept(TOK_COMMA)) {
            break;
        }
    }
    Expect(TOK_RPAREN);
    ParseCompoundStatement();
    printf(".RET:\n");
    printf("\tMOV\tSP, BP\n");
    printf("\tPOP\tBP\n");
    printf("\tRET\n");
}

static const char* const Prelude =
"\tcpu 8086\n"
"\torg 0x100\n"
"Start:\n"
"\tXOR\tBP, BP\n"
"\tCALL\t_main\n"
"\tMOV\tAH, 0x4C\n"
"\tINT\t0x21\n"
"\n"
"_putchar:\n"
"\tPUSH\tBP\n"
"\tMOV\tBP, SP\n"
"\tMOV\tDL, [BP+4]\n"
"\tMOV\tAH, 0x02\n"
"\tINT\t0x21\n"
"\tMOV\tSP, BP\n"
"\tPOP\tBP\n"
"\tRET\n"
"\n"
"_malloc:\n"
"\tPUSH\tBP\n"
"\tMOV\tBP, SP\n"
"\tMOV\tAX, [HeapPtr]\n"
"\tMOV\tCX, [BP+4]\n"
"\tINC\tCX\n"
"\tAND\tCL, 0xFE\n"
"\tADD\tCX, AX\n"
"\tMOV\t[HeapPtr], CX\n"
"\tMOV\tSP, BP\n"
"\tPOP\tBP\n"
"\tRET\n"
;

static const char* const Postlude =
"\n"
"HeapPtr:\tdw HeapStart\n"
"HeapStart:\n"
;

int main(void)
{
    //InBuf = "void puts(char* s) { int i; i = 0; while (s[i]) { putchar(s[i]); i = i + 1; } putchar(13); putchar(10); } void main() { char* s; s = malloc(3); s[0] = 65; s[1] = 48+6; s[2] = 0; puts(s); }";
    InBuf = "void puts(char* s) { int i; i = 0; while (s[i]) { putchar(s[i]); i = i + 1; } putchar(13); putchar(10); } void main() { puts(\"Hello world!\"); }";
#define DEF_TOKEN(V, N) do { int val = AddId(N); assert(TOK_LAST+1+val == V); } while (0);
    BUILTIN_TOKS(DEF_TOKEN)
#undef DEF_TOKEN

    puts(Prelude);

    PushScope();
    GetToken();

    /*struct VarDecl* putchar_decl = */AddVarDecl(VT_FUN|VT_VOID, AddId("putchar"));
    AddVarDecl(VT_FUN|VT_VOID|VT_PTR, AddId("malloc"));

    while (TokenType != TOK_EOF) {
        ParseExternalDefition();
    }
    PopScope();

    puts(Postlude);

    return 0;
}

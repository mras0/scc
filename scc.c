#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <assert.h>

#define INVALID_IDX -1
#define TOKEN_MAX 64
#define SCOPE_MAX 10
#define VARDECL_MAX 100

#define VT_VOID     0
#define VT_CHAR     1
#define VT_INT      2
#define VT_BASEMASK 0xff
#define VT_LVAL     0x100
#define VT_PTR      0x200
#define VT_FUN      0x400

#define BUILTIN_TOKS(X)      \
X(TOK_BREAK    , "break")    \
X(TOK_CHAR     , "char")     \
X(TOK_CONTINUE , "continue") \
X(TOK_ELSE     , "else")     \
X(TOK_FOR      , "for")      \
X(TOK_IF       , "if")       \
X(TOK_INT      , "int")      \
X(TOK_RETURN   , "return")   \
X(TOK_VOID     , "void")     \
X(TOK_WHILE    , "while")    \

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

    TOK_LAST = TOK_TILDE,
#define DEF_BULTIN(N, _) N,
    BUILTIN_TOKS(DEF_BULTIN)
#undef DEF_BUILTIN
    TOK_ID
};

static const char* InBuf;
static char TokenText[TOKEN_MAX];
static int TokenLen;
static int TokenType;
static uintptr_t TokenNumVal;
static char** Ids;
static int IdsCount, IdsCapacity;

static int VarDeclId[VARDECL_MAX];
static int VarDeclType[VARDECL_MAX];
static int VarDeclOffset[VARDECL_MAX];

static int Scopes[SCOPE_MAX]; // VarDecl index
static int ScopesCount;

static int LocalOffset;
static int CurrentType;
static int LocalLabelCounter;
static int ReturnLabel;
static int BreakLabel;
static int ContinueLabel;

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

static int TryGetChar(char ch)
{
    if (*InBuf != ch) {
        return 0;
    }
    TokenText[TokenLen++] = ch;
    ++InBuf;
    return 1;
}

static char* TokenTypeString(int type)
{
    switch (type) {
    case TOK_EOF:        return "eof";
    case TOK_NUM:        return "number";
    case TOK_STRLIT:     return "strlit";
    case TOK_LPAREN:     return "("; 
    case TOK_RPAREN:     return ")";
    case TOK_LBRACE:     return "{";
    case TOK_RBRACE:     return "}";
    case TOK_LBRACKET:   return "[";
    case TOK_RBRACKET:   return "]";
    case TOK_COMMA:      return ",";
    case TOK_SEMICOLON:  return ";";
    case TOK_EQ:         return "=";
    case TOK_EQEQ:       return "==";
    case TOK_NOT:        return "!";
    case TOK_NOTEQ:      return "!=";
    case TOK_LT:         return "<";
    case TOK_LTEQ:       return "<=";
    case TOK_GT:         return ">";
    case TOK_GTEQ:       return ">=";
    case TOK_PLUS:       return "+";
    case TOK_PLUSPLUS:   return "++";
    case TOK_MINUS:      return "-";
    case TOK_MINUSMINUS: return "--";
    case TOK_STAR:       return "*";
    case TOK_SLASH:      return "/";
    case TOK_MOD:        return "%";
    case TOK_AND:        return "&";
    case TOK_XOR:        return "^";
    case TOK_OR:         return "|";
    case TOK_ANDAND:     return "&&";
    case TOK_OROR:       return "||";
    case TOK_LSH:        return "<<";
    case TOK_RSH:        return ">>";
    case TOK_PLUSEQ:     return "+=";
    case TOK_MINUSEQ:    return "-=";
    case TOK_STAREQ:     return "*=";
    case TOK_SLASHEQ:    return "/=";
    case TOK_MODEQ:      return "%=";
    case TOK_LSHEQ:      return "<<=";
    case TOK_RSHEQ:      return ">>=";
    case TOK_ANDEQ:      return "&=";
    case TOK_XOREQ:      return "^=";
    case TOK_OREQ:       return "|=";
    case TOK_TILDE:      return "~";
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
    TokenLen = 0;

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
    } else if (ch == '\'') {
        --TokenLen;
        TokenText[TokenLen++] = GetChar();
        if (GetChar() != '\'') {
            Fatal("Invalid character literal");
        }
        TokenType = TOK_NUM;
        TokenNumVal = TokenText[0];
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
        case '~': TokenType = TOK_TILDE;     break;
        case '=':
            TokenType = TOK_EQ;
            if (TryGetChar('=')) {
                TokenType = TOK_EQEQ;
            }
            break;
        case '!':
            TokenType = TOK_NOT;
            if (TryGetChar('=')) {
                TokenType = TOK_NOTEQ;
            }
            break;
        case '<':
            TokenType = TOK_LT;
            if (TryGetChar('<')) {
                TokenType = TOK_LSH;
                if (TryGetChar('=')) {
                    TokenType = TOK_LSHEQ;
                }
            } else if (TryGetChar('=')) {
                TokenType = TOK_LTEQ;
            }
            break;
        case '>':
            TokenType = TOK_GT;
            if (TryGetChar('>')) {
                TokenType = TOK_RSH;
                if (TryGetChar('=')) {
                    TokenType = TOK_RSHEQ;
                }
            } else if (TryGetChar('=')) {
                TokenType = TOK_GTEQ;
            }
            break;
        case '&':
            TokenType = TOK_AND;
            if (TryGetChar('&')) {
                TokenType = TOK_ANDAND;
            } else if (TryGetChar('=')) {
                TokenType = TOK_ANDEQ;
            }
            break;
        case '|':
            TokenType = TOK_OR;
            if (TryGetChar('|')) {
                TokenType = TOK_OROR;
            } else if (TryGetChar('=')) {
                TokenType = TOK_OREQ;
            }
            break;
        case '^':
            TokenType = TOK_XOR;
            if (TryGetChar('=')) {
                TokenType = TOK_XOREQ;
            }
            break;
        case '+':
            TokenType = TOK_PLUS;
            if (TryGetChar('+')) {
                TokenType = TOK_PLUSPLUS;
            } else if (TryGetChar('=')) {
                TokenType = TOK_PLUSEQ;
            }
            break;
        case '-':
            TokenType = TOK_MINUS;
            if (TryGetChar('-')) {
                TokenType = TOK_MINUSMINUS;
            } else if (TryGetChar('=')) {
                TokenType = TOK_MINUSEQ;
            }
            break;
        case '*':
            TokenType = TOK_STAR;
            if (TryGetChar('=')) {
                TokenType = TOK_STAREQ;
            }
            break;
        case '/':
            TokenType = TOK_SLASH;
            if (TryGetChar('=')) {
                TokenType = TOK_SLASHEQ;
            }
            break;
        case '%':
            TokenType = TOK_MOD;
            if (TryGetChar('=')) {
                TokenType = TOK_MODEQ;
            }
            break;
        default:
            printf("Unknown token start '%c' (0x%02X)\n", ch, ch);
            TokenType = TOK_EOF;
        }
    }
Out:
    TokenText[TokenLen] = '\0';
}

int OperatorPrecedence(int tok)
{
    switch (tok) {
    case TOK_STAR:
    case TOK_SLASH:
    case TOK_MOD:
        return 3;
    case TOK_PLUS:
    case TOK_MINUS:
        return 4;
    case TOK_LSH:
    case TOK_RSH:
        return 5;
    case TOK_LT:
    case TOK_LTEQ:
    case TOK_GT:
    case TOK_GTEQ:
        return 6;
    case TOK_EQEQ:
    case TOK_NOTEQ:
        return 7;
    case TOK_AND:
        return 8;
    case TOK_XOR:
        return 9;
    case TOK_OR:
        return 10;
    case TOK_ANDAND:
        return 11;
    case TOK_OROR:
        return 12;
    case TOK_EQ:
    case TOK_PLUSEQ:
    case TOK_MINUSEQ:
    case TOK_STAREQ:
    case TOK_SLASHEQ:
    case TOK_MODEQ:
    case TOK_LSHEQ:
    case TOK_RSHEQ:
    case TOK_ANDEQ:
    case TOK_XOREQ:
    case TOK_OREQ:
        return 14;
    case TOK_COMMA:
        return 15;
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

int Lookup(int id)
{
    assert(ScopesCount);
    for (int i = Scopes[ScopesCount-1]; i >= 0; i--) {
        if (VarDeclId[i] == id) {
            return i;
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

static void DoIncDec(int Op)
{
    int Amm;
    if (CurrentType == VT_CHAR || CurrentType == VT_INT || CurrentType == (VT_CHAR|VT_PTR)) {
        Amm = 1;
    } else {
        Amm = 2;
    }
    if (Op == TOK_PLUSPLUS) {
        if (Amm == 1) {
            printf("\tINC\tAX\n");
        } else {
            printf("\tADD\tAX, %d\n", Amm);
        }
    } else {
        assert(Op == TOK_MINUSMINUS);
        if (Amm == 1) {
            printf("\tDEC\tAX\n");
        } else {
            printf("\tSUB\tAX, %d\n", Amm);
        }
    }
}

static void DoIncDecOp(int Op, int Post)
{
    assert(CurrentType & VT_LVAL);
    CurrentType &= ~VT_LVAL;
    printf("\tMOV\tBX, AX\n");
    if (CurrentType == VT_CHAR) {
        printf("\tMOV\tAL, [BX]\n");
        printf("\tCBW\n");
        if (Post) printf("\tPUSH\tAX\n");
        DoIncDec(Op);
        printf("\tMOV\t[BX], AL\n");
        CurrentType = VT_INT;
    } else {
        assert(CurrentType == VT_INT || (CurrentType & VT_PTR));
        printf("\tMOV\tAX, [BX]\n");
        if (Post) printf("\tPUSH\tAX\n");
        DoIncDec(Op);
        printf("\tMOV\t[BX], AX\n");
    }
    if (Post) printf("\tPOP\tAX\n");
}

void ParseExpression(void);
void ParseCastExpression(void);
void ParseAssignmentExpression(void);

void ParsePrimaryExpression(void)
{
    if (TokenType == TOK_LPAREN) {
        GetToken();
        ParseExpression();
        Expect(TOK_RPAREN);
    } else if (TokenType == TOK_NUM) {
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
        const int vd = Lookup(id);
        CurrentType = VarDeclType[vd] | VT_LVAL;
        if (VarDeclOffset[vd]) {
            printf("\tLEA\tAX, [BP%+d]\t; %s\n", VarDeclOffset[vd], Ids[id]);
        } else {
            printf("\tMOV\tAX, _%s\n", Ids[id]);
            if (CurrentType & VT_FUN) {
                CurrentType &= ~VT_LVAL;
            }
        }
    }
}

void ParsePostfixExpression(void)
{
    for (;;) {
        if (Accept(TOK_LPAREN)) {
            // Function call
            if (!(CurrentType & VT_FUN)) {
                Fatal("Not a function");
            }
            printf("\tPUSH\tSI\n");
            printf("\tPUSH\tDI\n");
            printf("\tMOV\tSI, AX\n");
            const int RetType = CurrentType & ~VT_FUN;
            const int MaxArgs = 4; // TODO: Handle this better...
            printf("\tSUB\tSP, %d\n", MaxArgs*2);
            printf("\tMOV\tDI, SP\n");
            int NumArgs = 0;
            while (TokenType != TOK_RPAREN) {
                assert(NumArgs < MaxArgs);
                ParseAssignmentExpression();
                LvalToRval();
                assert(CurrentType == VT_INT || (CurrentType & VT_PTR));
                printf("\tMOV\t[DI%+d], AX\n", NumArgs*2);
                ++NumArgs;

                if (!Accept(TOK_COMMA)) {
                    break;
                }
            }
            Expect(TOK_RPAREN);
            printf("\tCALL\tSI\n");
            printf("\tADD\tSP, %d\n", MaxArgs*2);
            printf("\tPOP\tDI\n");
            printf("\tPOP\tSI\n");
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
    if (Op == TOK_PLUSPLUS || Op == TOK_MINUSMINUS) {
        GetToken();
        ParseUnaryExpression();
        DoIncDecOp(Op, 0);
    } else if (Op == TOK_AND || Op == TOK_STAR || Op == TOK_PLUS || Op == TOK_MINUS || Op == TOK_TILDE || Op == TOK_NOT) {
        GetToken();
        ParseCastExpression();
        if (Op != TOK_AND) {
            LvalToRval();
        }
        switch (Op) {
        case TOK_AND:
            if (!(CurrentType & VT_LVAL)) {
                Fatal("Lvalue required for address-of operator");
            }
            CurrentType = (CurrentType&~VT_LVAL) | VT_PTR;
            break;
        case TOK_STAR:
            if (!(CurrentType & VT_PTR)) {
                Fatal("Pointer required for dereference");
            }
            CurrentType = (CurrentType&~VT_PTR) | VT_LVAL;
            break;
        case TOK_PLUS:
            assert(CurrentType == VT_INT);
            break;
        case TOK_MINUS:
            assert(CurrentType == VT_INT);
            printf("\tNEG\tAX\n");
            break;
        case TOK_TILDE:
            assert(CurrentType == VT_INT);
            printf("\tNOT\tAX\n");
            break;
        case TOK_NOT:
            {
                const int Lab = LocalLabelCounter++;
                assert(CurrentType == VT_INT);
                printf("\tAND\tAX, AX\n");
                printf("\tMOV\tAX, 0\n");
                printf("\tJNZ\t.L%d\n", Lab);
                printf("\tINC\tAL\n");
                printf(".L%d:\n", Lab);
                break;
            }
        default:
            printf("TODO: Unary op %s. CurrentType = %s%s\n", TokenTypeString(Op), TypeString(CurrentType), CurrentType&VT_LVAL?"|VT_LVAL":"");
            Fatal("Not implemented");
        }
    } else {
        ParsePrimaryExpression();
        ParsePostfixExpression();
    }
}

void ParseCastExpression(void)
{
    ParseUnaryExpression();
}

const char* RelOp(int Op)
{
    if (Op == TOK_LT)         return "L";
    else if (Op == TOK_LTEQ)  return "NG";
    else if (Op == TOK_GT)    return "G";
    else if (Op == TOK_GTEQ)  return "NL";
    else if (Op == TOK_EQEQ)  return "E";
    else if (Op == TOK_NOTEQ) return "NE";
    printf("TODO: RelOp %s\n", TokenTypeString(Op));
    Fatal("Not implemented");
}

// Emit: AX <- AX 'OP' CX, Must preserve BX.
void DoBinOp(int Op)
{
    if (Op == TOK_LT || Op == TOK_LTEQ || Op == TOK_GT || Op == TOK_GTEQ || Op == TOK_EQEQ || Op == TOK_NOTEQ) {
        const int Lab = LocalLabelCounter++;
        printf("\tCMP\tAX, CX\n");
        printf("\tMOV\tAX, 1\n");
        RelOp(Op);
        printf("\tJ%s\t.L%d\n", RelOp(Op), Lab);
        printf("\tDEC\tAL\n");
        printf(".L%d:\n", Lab);
        return;
    }

    switch (Op) {
    case TOK_PLUSEQ:
    case TOK_PLUS:  printf("\tADD\tAX, CX\n"); break;
    case TOK_MINUSEQ:
    case TOK_MINUS: printf("\tSUB\tAX, CX\n"); break;
    case TOK_STAREQ:
    case TOK_STAR:  printf("\tMUL\tCX\n"); break;
    case TOK_SLASHEQ:
    case TOK_SLASH:
    case TOK_MODEQ:
    case TOK_MOD:
        printf("\tXOR\tDX, DX\n");
        printf("\tIDIV\tCX\n");
        if (Op == TOK_MOD) printf("\tMOV\tAX, DX\n");
        break;
    case TOK_ANDEQ:
    case TOK_AND:   printf("\tAND\tAX, CX\n"); break;
    case TOK_XOREQ:
    case TOK_XOR:   printf("\tXOR\tAX, CX\n"); break;
    case TOK_OREQ:
    case TOK_OR:    printf("\tOR\tAX, CX\n"); break;
    case TOK_LSHEQ:
    case TOK_LSH:   printf("\tSHL\tAX, CL\n"); break;
    case TOK_RSHEQ:
    case TOK_RSH:   printf("\tSAR\tAX, CL\n"); break;
    default:
        printf("TODO: DoBinOp %s\n", TokenTypeString(Op));
        Fatal("Not implemented");
    }
}

void ParseExpression1(int OuterPrecedence)
{
    int LEnd;
    for (;;) {
        const int Op   = TokenType;
        const int Prec = OperatorPrecedence(Op);
        if (Prec > OuterPrecedence) {
            break;
        }
        GetToken();

        const int IsAssign = Prec == OperatorPrecedence(TOK_EQ);

        if (IsAssign) {
            if (!(CurrentType & VT_LVAL)) {
                Fatal("L-value required");
            }
        } else {
            LvalToRval();
        }
        int LhsType = CurrentType;

        if (Op == TOK_ANDAND || Op == TOK_OROR) {
            LEnd = LocalLabelCounter++;
            assert(CurrentType == VT_INT);
            printf("\tAND\tAX, AX\n");
            printf("\tJ%s\t.L%d\n", Op == TOK_ANDAND ? "Z" : "NZ", LEnd);
        } else {
            LEnd = -1;
            printf("\tPUSH\tAX\n");
        }

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
        LvalToRval();
        if (LEnd >= 0) {
            printf(".L%d:\n", LEnd);
        } else if (IsAssign) {
            assert(LhsType & VT_LVAL);
            LhsType &= ~VT_LVAL;
            printf("\tPOP\tBX\n");
            if (Op != TOK_EQ) {
                assert(LhsType == VT_INT);
                assert(CurrentType == VT_INT);
                printf("\tMOV\tCX, AX\n");
                printf("\tMOV\tAX, [BX]\n");
                DoBinOp(Op);
            }
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

static int AddVarDecl(int Type, int Id)
{
    assert(ScopesCount);
    assert(Scopes[ScopesCount-1] < VARDECL_MAX);
    const int vd = Scopes[ScopesCount-1]++;
    VarDeclType[vd]   = Type;
    VarDeclOffset[vd] = 0;
    VarDeclId[vd]     = Id;
    return vd;
}

int ParseDecl(void)
{
    const int Type = ParseDeclSpecs();
    const int Id   = ExpectId();
    return AddVarDecl(Type, Id);
}

static void PushScope(void)
{
    assert(ScopesCount < SCOPE_MAX);
    const int End = ScopesCount ? Scopes[ScopesCount-1] : 0;
    Scopes[ScopesCount++] = End;
}

static void PopScope(void)
{
    assert(ScopesCount);
    --ScopesCount;
}

void ParseCompoundStatement(void);

void ParseStatement(void)
{
    const int OldBreak    = BreakLabel;
    const int OldContinue = ContinueLabel;

    if (TokenType == TOK_LBRACE) {
        ParseCompoundStatement();
    } else if (IsTypeStart()) {
        const int vd = ParseDecl();
        LocalOffset -= 2;
        VarDeclOffset[vd] = LocalOffset;
        printf("\tSUB\tSP, 2\t; [BP%+d] %s %s\n", LocalOffset, TypeString(VarDeclType[vd]), Ids[VarDeclId[vd]]);
        Expect(TOK_SEMICOLON);
    } else if (Accept(TOK_FOR)) {
        const int CondLabel  = LocalLabelCounter++;
        const int BodyLabel  = LocalLabelCounter++;
        const int EndLabel   = LocalLabelCounter++;
        int IterLabel = CondLabel;
        Expect(TOK_LPAREN);

        // Init
        if (TokenType != TOK_SEMICOLON) {
            ParseExpression();
        }
        Expect(TOK_SEMICOLON);

        // Cond
        printf(".L%d:\t; Cond for\n", CondLabel);
        if (TokenType != TOK_SEMICOLON) {
            ParseExpression();
            LvalToRval();
            assert(CurrentType == VT_INT);
            printf("\tAND\tAX, AX\n");
            printf("\tJNZ\t.L%d\n", BodyLabel); // TODO: Need far jump?
            printf("\tJMP\t.L%d\n", EndLabel);
        } else {
            printf("\tJMP\t.L%d\n", BodyLabel);
        }
        Expect(TOK_SEMICOLON);

        // Iter
        if (TokenType != TOK_RPAREN) {
            IterLabel  = LocalLabelCounter++;
            printf(".L%d:\t; Iter for\n", IterLabel);
            ParseExpression();
            printf("\tJMP\t.L%d\n", CondLabel);
        }
        Expect(TOK_RPAREN);

        printf(".L%d:\t; Body for\n", BodyLabel);
        BreakLabel    = EndLabel;
        ContinueLabel = IterLabel;
        ParseStatement();
        printf("\tJMP\t.L%d\n", IterLabel);
        printf(".L%d:\t; End for\n", EndLabel);
    } else if (Accept(TOK_IF)) {
        const int IfLabel    = LocalLabelCounter++;
        const int ElseLabel  = LocalLabelCounter++;
        const int EndLabel   = LocalLabelCounter++;
        printf("\t; Start of if\n");
        Accept(TOK_LPAREN);
        ParseExpression();
        LvalToRval();
        assert(CurrentType == VT_INT);
        printf("\tAND\tAX, AX\n");
        printf("\tJNZ\t.L%d\n", IfLabel);
        printf("\tJMP\t.L%d\n", ElseLabel);
        Accept(TOK_RPAREN);
        printf(".L%d:\t; If\n", IfLabel);
        ParseStatement();
        printf("\tJMP\t.L%d\n", EndLabel);
        printf(".L%d:\t; Else\n", ElseLabel);
        if (Accept(TOK_ELSE)) {
            ParseStatement();
        }
        printf(".L%d:\t; End if\n", EndLabel);
    } else if (Accept(TOK_RETURN)) {
        if (TokenType != TOK_SEMICOLON) {
            ParseExpression();
        }
        printf("\tJMP\t.L%d\n", ReturnLabel);
        Expect(TOK_SEMICOLON);
    } else if (Accept(TOK_WHILE)) {
        const int StartLabel = LocalLabelCounter++;
        const int BodyLabel  = LocalLabelCounter++;
        const int EndLabel   = LocalLabelCounter++;
        printf(".L%d:\t; Start of while\n", StartLabel);
        Expect(TOK_LPAREN);
        ParseExpression();
        LvalToRval();
        assert(CurrentType == VT_INT);
        printf("\tAND\tAX, AX\n");
        printf("\tJNZ\t.L%d\n", BodyLabel);
        printf("\tJMP\t.L%d\n", EndLabel);
        Expect(TOK_RPAREN);
        printf(".L%d:\t; Body of while\n", BodyLabel);
        BreakLabel = EndLabel;
        ContinueLabel = StartLabel;
        ParseStatement();
        printf("\tJMP\t.L%d\n", StartLabel);
        printf(".L%d:\t; End of while\n", EndLabel);
    } else if (Accept(TOK_BREAK)) {
        assert(BreakLabel >= 0);
        printf("\tJMP\t.L%d\n", BreakLabel);
        Expect(TOK_SEMICOLON);
    } else if (Accept(TOK_CONTINUE)) {
        assert(ContinueLabel >= 0);
        printf("\tJMP\t.L%d\n", ContinueLabel);
        Expect(TOK_SEMICOLON);
    } else {
        // Expression statement
        ParseExpression();
        Expect(TOK_SEMICOLON);
    }

    BreakLabel    = OldBreak;
    ContinueLabel = OldContinue;
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
    const int fd = ParseDecl();
    int ArgOffset = 4;
    printf("\t; Starting function definition of %s, return value %s\n", Ids[VarDeclId[fd]], TypeString(VarDeclType[fd]));
    printf("_%s:\n", Ids[VarDeclId[fd]]);
    printf("\tPUSH\tBP\n");
    printf("\tMOV\tBP, SP\n");
    VarDeclType[fd] |= VT_FUN;
    LocalOffset = 0;
    LocalLabelCounter = 0;
    ReturnLabel = LocalLabelCounter++;
    BreakLabel = ContinueLabel = -1;
    Expect(TOK_LPAREN);
    while (TokenType != TOK_RPAREN) {
        if (Accept(TOK_VOID)) {
            break;
        }
        const int vd = ParseDecl();
        VarDeclOffset[vd] = ArgOffset;
        ArgOffset += 2;
        printf("\t; Arg %s %s [BP+%d]\n", TypeString(VarDeclType[vd]), Ids[VarDeclId[vd]], VarDeclOffset[vd]);
        if (!Accept(TOK_COMMA)) {
            break;
        }
    }
    Expect(TOK_RPAREN);
    ParseCompoundStatement();
    printf(".L%d:\n", ReturnLabel);
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
    //InBuf = "void puts(char* s) { int i; i = 0; while (s[i]) { putchar(s[i]); i = i + 1; } putchar(13); putchar(10); } void main() { puts(\"Hello world!\"); }";
    //InBuf = "int IsAlpha(char ch) { return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z'); } void main() { int c; c = 0; while (c<256) { if (IsAlpha(c)) putchar(c); ++c; } }";
    //InBuf = "void main() { int i; for (i = 0; ;++i) { if (!(i&1)) continue; if(i>=10)break; putchar('0'+i); } }";
    InBuf = "void main() { int i; i = 0; while (1) { ++i; if (i==10) break; if (i&1) continue; putchar('0'+i); } }";
#define DEF_TOKEN(V, N) do { int val = AddId(N); assert(TOK_LAST+1+val == V); } while (0);
    BUILTIN_TOKS(DEF_TOKEN)
#undef DEF_TOKEN

    puts(Prelude);

    PushScope();
    GetToken();

    AddVarDecl(VT_FUN|VT_VOID, AddId("putchar"));
    AddVarDecl(VT_FUN|VT_VOID|VT_PTR, AddId("malloc"));

    while (TokenType != TOK_EOF) {
        ParseExternalDefition();
    }
    PopScope();

    puts(Postlude);

    return 0;
}

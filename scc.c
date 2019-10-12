#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <assert.h>

#define TOKEN_MAX 64

#define BUILTIN_TOKS(X)   \
X(TOK_VOID   , "void")    \
X(TOK_INT    , "int")     \
X(TOK_RETURN , "return")  \


enum {
    TOK_EOF,

    TOK_NUM,

    TOK_LPAREN,
    TOK_RPAREN,
    TOK_LBRACE,
    TOK_RBRACE,
    TOK_COMMA,
    TOK_SEMICOLON,
    TOK_EQ,
    TOK_PLUS,

    TOK_LAST = TOK_PLUS,
#define DEF_BULTIN(N, _) N,
    BUILTIN_TOKS(DEF_BULTIN)
#undef DEF_BUILTIN
    TOK_ID
};

static const char* InBuf;
static char TokenText[TOKEN_MAX];
static int TokenType;
static uintptr_t TokenNumVal;
static char** Ids;
static int IdsCount, IdsCapacity;

static int IsDigit(char ch) { return ch >= '0' && ch <= '9'; }
static int IsAlpha(char ch) { return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z'); }

static __declspec(noreturn) void Fatal(const char* Msg)
{
    fputs(Msg, stderr);
    abort();
}

static char* DupString(const char* Name)
{
    const size_t cnt = strlen(Name) + 1;
    char* Buf = malloc(cnt);
    if (!Buf) {
        Fatal("malloc failed");
    }
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
    case TOK_LPAREN:    return "("; 
    case TOK_RPAREN:    return ")";
    case TOK_LBRACE:    return "{";
    case TOK_RBRACE:    return "}";
    case TOK_COMMA:     return ",";
    case TOK_SEMICOLON: return ";";
    case TOK_EQ:        return "=";
    case TOK_PLUS:      return "+";
#define CASE_TOKEN(V, N) case V: return N;
    BUILTIN_TOKS(CASE_TOKEN)
#undef CASE_TOKEN
    }
    static char buf[64];
    snprintf(buf, sizeof(buf), "TOK(%d)", type);
    return buf;
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
    } else {
        switch (ch) {
        case '(': TokenType = TOK_LPAREN;    break;
        case ')': TokenType = TOK_RPAREN;    break;
        case '{': TokenType = TOK_LBRACE;    break;
        case '}': TokenType = TOK_RBRACE;    break;
        case ',': TokenType = TOK_COMMA;     break;
        case ';': TokenType = TOK_SEMICOLON; break;
        case '=': TokenType = TOK_EQ;        break;
        case '+': TokenType = TOK_PLUS;      break;
        default:
            printf("Unknown token start '%c' (0x%02X)\n", ch, ch);
            TokenType = TOK_EOF;
        }
    }
Out:
    TokenText[TokenLen] = '\0';

    printf("%s \"%s\" ", TokenTypeString(TokenType), TokenText);
    PrintToken();
    puts("");
}

void __declspec(noreturn) Unexpected(void)
{
    PrintToken();
    Fatal("\nUnexpected token");
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
    return TokenType == TOK_VOID || TokenType == TOK_INT;
}

void ParseExpression(void);

void ParseUnaryExpression(void)
{
    if (TokenType == TOK_NUM) {
        printf("TODO: Number %" PRIuMAX "\n", TokenNumVal);
        GetToken();
        return;
    }
    int id = ExpectId();
    printf("TODO: Id %s\n", Ids[id]);
    if (!Accept(TOK_LPAREN)) {
        return;
    }
    // Function call
    while (TokenType != TOK_RPAREN) {
        ParseExpression();
        if (!Accept(TOK_COMMA)) {
            break;
        }
    }
    Expect(TOK_RPAREN);
}

void ParseExpression(void)
{
    ParseUnaryExpression();
    for (;;) {
        if (TokenType != TOK_PLUS) {
            break;
        }
        GetToken();
        ParseUnaryExpression();
    }
}

void ParseDeclSpecs(void)
{
    assert(IsTypeStart());
    if (TokenType == TOK_VOID || TokenType == TOK_INT) {
        printf("Ignoring "); PrintToken(); printf(" in %s\n", __func__);
        GetToken();
        return;

    }
    Unexpected();
}

void ParseCompoundStatement(void)
{
    Expect(TOK_LBRACE);
    while (!Accept(TOK_RBRACE)) {
        if (IsTypeStart()) {
            ParseDeclSpecs();
            ExpectId();
            if (Accept(TOK_EQ)) {
                ParseExpression();
            }
        } else if (Accept(TOK_RETURN)) {
            if (TokenType != TOK_SEMICOLON) {
                ParseExpression();
            }
        } else {
            Unexpected();
        }
        Expect(TOK_SEMICOLON);
    }
}

void ParseExternalDefition(void)
{
    ParseDeclSpecs();
    const int NameId = ExpectId();
    printf("Name: %s\n", Ids[NameId]);
    Expect(TOK_LPAREN);
    while (TokenType != TOK_RPAREN) {
        if (Accept(TOK_VOID)) {
            break;
        }
        ParseDeclSpecs();
        /*const int VarId = */ ExpectId();
        if (!Accept(TOK_COMMA)) {
            break;
        }
    }
    Expect(TOK_RPAREN);
    ParseCompoundStatement();
}

int main(void)
{
    InBuf = "int foo(int a, int b) { int c = a + b; return c; }\nint main(void) { return foo(42, 2); }\n";
#define DEF_TOKEN(V, N) do { int val = AddId(N); assert(TOK_LAST+1+val == V); } while (0);
    BUILTIN_TOKS(DEF_TOKEN)
#undef DEF_TOKEN

    GetToken();
    while (TokenType != TOK_EOF) {
        ParseExternalDefition();
    }

    return 0;
}

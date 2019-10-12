#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#define TOKEN_MAX 64

#define BUILTIN_TOKS(X)   \
X(TOK_VOID   , "void")    \
X(TOK_CHAR   , "char")    \
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

    printf("%d \"%s\" ", TokenType, TokenText);
    if (TokenType == TOK_NUM) {
        printf("%u", (unsigned)TokenNumVal);
    } else if (TokenType >= TOK_ID) {
        printf("ID");
    }
    puts("");
}


int main(void)
{
    InBuf = "int foo(int a, int b) { int c = a + b; return c; }\nint main(void) { return foo(42, 2); }\n";
#define DEF_TOKEN(V, N) do { int val = AddId(N); assert(TOK_LAST+1+val == V); } while (0);
    BUILTIN_TOKS(DEF_TOKEN)
#undef DEF_TOKEN

    printf("TOK_INT = %d\n", TOK_INT);
    do {
        GetToken();
    } while (TokenType != TOK_EOF);
    for (int id = 0; id < IdsCount; ++id) {
        printf("%d: \"%s\" %s\n", TOK_LAST+1+id, Ids[id], id + TOK_LAST + 1 >= TOK_ID ? "Id" : "Builtin");
    }
    return 0;
}
